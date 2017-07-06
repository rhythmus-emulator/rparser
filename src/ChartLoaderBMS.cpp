/* supports bms, bme, bml formats. */

#include "ChartLoader.h"
#include "Chart.h"
#include "rutil.h"
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>

using namespace rutil;

namespace rparser {

bool ChartLoaderBMS::TestName(const char *fn)
{
	return endsWith(fn, ".bms", false) ||
		endsWith(fn, ".bme", false) ||
		endsWith(fn, ".bml", false);
}


/* common */
void ParseLine(const std::string& line, std::string& name, std::string& value, char sep=0)
{
    // trim
    std::string line_trim = trim(line);

    size_t space = std::string::npos;
    if (sep == 0) {
        // find any type of space
        space = line_trim.find(' ');
        if (space == std::string::npos) space=line_trim.find('\t');
    } else {
        space = line_trim.find(sep);
    }
	if (space == std::string::npos)
	{
		name.clear();
		value.clear();
		return;
	}
    name = line_trim.substr(0, space);
    value = line_trim.substr(space+1);
}
void ParseLine(const char** p_, std::string& name, std::string& value, char sep=0) {
	const char* p = *p_;
    while (*p != '\n' && *p) p++;
	if (p == *p_) { name.clear(); }  // no new content -> no name
    else {
        std::string line(*p_, p-*p_);
        ParseLine(line, name, value, sep);
    }
	if (*p) p++;
	*p_ = p;
}

// @description
// used tempoarary when parsing/writing Bms file,
// as Bms note position is based on measure, not row.
struct BmsNote {
    int measure;    // measure number
    int den, num;   // denominator/numerator of current measure
    int channel;    // channel number
    int value;      // value of current object
    int value_prev; // for LNTYPE2
    int colidx;     // for bgm channel
    bool operator<( const BmsNote &other ) const
	{
		return measure < other.measure;
	}
};

/* processes bms expand command. */
class BMSExpandProc {
private:
    Chart *c;
    std::vector<int> active_value_stack;    // used in case of #RANDOM~#ENDRANDOM
    int active_value;
    std::string m_sExpand;      // only extracted expand commands
    std::string m_sProcCmd;     // expand-processed commands

    enum COND_RESULT {
        COND_UNKNOWN = 0,
        COND_NEW = 1,
        COND_ACCEPT = 2,
        COND_END = 3
    };
    class Condition {
    public:
        virtual bool Valid() = 0;
        virtual std::string GesubType() = 0;
        virtual void Parse(const std::string& cmd_lower, const std::string& value) = 0;
    };
    class IFCondition : public Condition {
        int target;      // active value
        int activecnt;  // condition is activated?
        int activeidx;  // current condition is active?
        int condidx;    // current condition index

    public:
        IFCondition(int active_value) :
            condidx(0), activecnt(0), activeidx(-1), target(active_value) {}
        static int Test(const std::string& name) {
            if (name == "#endif" || name == "#end") {
                return COND_END;
            } else if (name == "#if") {
                return COND_NEW;
            } else if (name == "#else") {
                return COND_ACCEPT;
            } else if (name == "#elseif") {
                return COND_ACCEPT;
            }
            return COND_UNKNOWN;
        }
        std::string GesubType() { return "if"; }
        void Parse(const std::string& name, const std::string& value) {
            if (name == "#endif" || name == "#end") {
                // #ENDIF cannot be shown first
                ASSERT(condidx > 0);
            } else if (name == "#if") {
                // #IF clause cannot be shown twice or later
                ASSERT(condidx <= 0);
                condidx++;
                if (target == atoi(value.c_str())) {
                    activeidx = condidx;
                    activecnt++;
                }
            } else if (name == "#else") {
                condidx++;
                if (activecnt <= 0) {
                    activeidx = condidx;
                    activecnt++;
                }
            } else if (name == "#elseif") {
                condidx++;
                if (activecnt <= 0 && target == atoi(value.c_str())) {
                    activeidx = condidx;
                    activecnt++;
                }
            }
        }
        bool Valid() {
            return activeidx==condidx;
        }
	};
    class SWITCHCondition : public Condition {
        int target;      // active value
        int stat;       // -1: skipped, 0: not activated, 1~: activated

    public:
        SWITCHCondition(int active_value) :
            stat(0), target(active_value) {}
        static int Test(const std::string& name) {
            if (name == "#endsw") {
                return COND_END;
            } else if (name == "#switch" || name == "#setswitch") {
                return COND_NEW;
            } else if (name == "#case" || name == "#skip" || name == "#def") {
                return COND_ACCEPT;
            }
            return COND_UNKNOWN;
        }
        std::string GesubType() { return "switch"; }
        void Parse(const std::string& name, const std::string& value) {
            if (name == "#endsw") {
                // #ENDIF cannot be shown first
                ASSERT(stat > 0);
            } else if (name == "#switch" || name == "#setswitch") {
                // initialize random value
                target = atoi(value.c_str()) * rand();
            } else if (name == "#case") {
                if (atoi(value.c_str()) == target && stat >= 0)
                    stat++;
            } else if (name == "#skip") {
                stat = -1;
            } else if (name == "#def") {
                if (stat >= 0)
                    stat++;
            }
        }
        bool Valid() {
            return stat > 0;
        }
	};

    std::vector<Condition*> conds;
public:
    int m_iSeed;
    bool m_bAllowExpand;
    BMSExpandProc(Chart *c, int iSeed=-1, bool bAllowExpand=true):
        c(c), m_iSeed(iSeed), m_bAllowExpand(bAllowExpand), active_value(0) {}

    bool IsCurrentValidStatement() {
        return (conds.size() == 0 || (m_bAllowExpand && conds.back()->Valid()));
    }

	// process for conditional statement
    void ProcessStatement(const std::string& line) {
        // get command
        std::string name, value;
        ParseLine(line, name, value);
        if (name.empty()) return;

        // match command
        int isCond = 1;
        int cond_test;
        Condition* curr_cond;
        std::vector<Condition*> cond;
        if (name == "#random" || name == "#rondom") {
            active_value_stack.push_back(active_value);
            active_value = atoi(value.c_str()) * rand();
        } else if (name == "#endrandom") {
            ASSERT(active_value_stack.size() > 0);
            active_value = active_value_stack.back();
            active_value_stack.pop_back();
        } else if ((cond_test = IFCondition::Test(name)) != COND_UNKNOWN) {
            switch (cond_test) {
            case COND_NEW:
                curr_cond = new IFCondition(active_value);
                curr_cond->Parse(name, value);
                cond.push_back(curr_cond);
                break;
            case COND_ACCEPT:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GesubType() == "if");
                curr_cond->Parse(name, value);
                break;
            case COND_END:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GesubType() == "if");
                curr_cond->Parse(name, value);
                cond.pop_back();
                delete curr_cond;
                break;
            }
        } else if ((cond_test = SWITCHCondition::Test(name)) != COND_UNKNOWN) {
            switch (cond_test) {
            case COND_NEW:
                curr_cond = new SWITCHCondition(active_value);
                curr_cond->Parse(name, value);
                cond.push_back(curr_cond);
                break;
            case COND_ACCEPT:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GesubType() == "switch");
                curr_cond->Parse(name, value);
                break;
            case COND_END:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GesubType() == "switch");
                curr_cond->Parse(name, value);
                cond.pop_back();
                delete curr_cond;
                break;
            }
        } else {
            isCond = 0;
        }
        // if condition size is over zero before and after,
        // then it's expand command.
        int is_curr_expand = isCond || cond.size();
        if (is_curr_expand) {
            m_sExpand += line + '\n';
        }
        // processed statement
        if (isCond == 0 && IsCurrentValidStatement()) {
            m_sProcCmd += line + '\n';
        }
    }

    void proc( const char* in, int iLen ) {
        // clear tree
        conds.clear();

        // set seed first
        if (m_iSeed >= 0)
            srand(m_iSeed);

        // read each line
        const char* p_end = in+iLen;
        while (in < p_end) {
            // split current line
            const char* p = in;
            while (p < p_end && *p != '\n') p++;
            // make trimmed string
            const char *s=in, *e=p;
            while (*s == ' ' || *s == '\t' || *s == '\r') s++;
            while (e > s && (*e == ' ' || *e == '\t' || *e == '\r')) e--;
            std::string line(s, e-s);
            // prepare for next line
            in = p+1;
            // process line
            ProcessStatement(line);
        }
    }

    std::string& GetProcCmd() { return m_sProcCmd; }
    std::string& GetExpandCmd() { return m_sExpand; }
};


// main file
bool ChartLoaderBMS::Test( const void* p, int iLen )
{
    // As bms file has no file signature,
    // just search for "*--" string for first 100 string.
    // TODO: is there any exception?
    for (int i=0; i<iLen-4 && i<100; i++) {
        if (strncmp((const char*)p, "*-----", 6)==0)
            return true;
    }
    return false;
}

bool TestName( const char *fn )
{
    std::string fn_lower = fn;  lower(fn_lower);
    return endsWith(fn_lower, ".bms") ||
        endsWith(fn_lower, ".bme") ||
        endsWith(fn_lower, ".bml") ||
        endsWith(fn_lower, ".pms");
}

bool ChartLoaderBMS::Load( const void* p, int iLen )
{
    // parse expand, and process it first
    // (expand parse effects to Metadata / Channels)
    BMSExpandProc bExProc(c, m_iSeed, procExpansion);
	bExProc.ProcessStatement((const char*)p);
    std::string& proc_res = bExProc.GetProcCmd();
    c->GetMetaData()->sExpand = bExProc.GetExpandCmd();

    // parse metadata(include expanded command) first
    ReadHeader(proc_res.c_str(), proc_res.size());

    // parse BGA & notedata channel second
    ReadObjects(proc_res.c_str(), proc_res.size());
    notes.clear();

    // set encoding from EUC-JP to UTF8
    c->GetMetaData()->SetEncoding(R_CP_932, R_CP_UTF8);

    // fill hash file
    c->UpdateHash(p, iLen);

    // we don't call EvaluateBeat in here,
    // as fBeat is automatically calculated 
    // while reading objects. (in good resolution)

    return true;
}

void ChartLoaderBMS::ReadHeader(const char* p, int iLen)
{
    MetaData* md = c->GetMetaData();
    md->iLNType = 1;    // default LN type
    md->iLNObj = 0;

    const char *pp = p;
    std::string name, value;
    while (1) {
        if (*pp == 0) break;
        ParseLine(&pp, name, value);
		name = lower(name);

        if (!name.empty()) {
            if (name == "#title") {
                md->sTitle = value;
            }
            else if (name == "#subtitle") {
                md->sSubTitle = value;
            }
            else if (name == "#artist") {
                md->sArtist = value;
            }
            else if (name == "#subartist") {
                md->sSubArtist = value;
            }
            else if (name == "#genre") {
                md->sGenre = value;
            }
            else if (name == "#player") {
                md->iPlayer = atoi(value.c_str());
            }
            else if (name == "#playlevel") {
                md->iLevel = atoi(value.c_str());
            }
            else if (name == "#difficulty") {
                md->iDifficulty = atoi(value.c_str());
            }
            else if (name == "#rank") {
                // convert from 4 to 100.0
                md->fJudge = atof(value.c_str()) / 4.0 * 100;
            }
            else if (name == "#total") {
                md->fTotal = atof(value.c_str());
            }
            else if (name == "#banner") {
                md->sBannerImage = value;
            }
            else if (name == "#backbmp") {
                md->sBackImage = value;
            }
            else if (name == "#stagefile") {
                md->sStageImage = value;
            }
            else if (name == "#bpm") {
                md->fBPM = atof(value.c_str());
            }
            else if (name == "#lntype") {
                md->iLNType = atoi(value.c_str());
            }
            else if (name == "#lnobj") {
                md->iLNObj = atoi_bms(value.c_str());
            }
            else if (name == "#music") {
                md->sMusic = value;
            }
            else if (name == "#preview") {
                md->sPreviewMusic = value;
            }
            else if (name == "#offset") {
                // append it to TimingData
                c->GetTimingData()->SetBeat0Offset( atof(value.c_str()) );
            }

            /*
             * Resource part
             */
            #define CHKCMD(s, cmd, len) (strncmp(s.c_str(), cmd, len) == 0)
            else if (CHKCMD(name,"#bmp",4)) {
                int key = atoi_bms(name.c_str() + 4);
                md->GetBGAChannel()->bga[key] = { value, 0,0,0,0, 0,0,0,0 };
            }
            else if (CHKCMD(name,"#wav",4)) {
                int key = atoi_bms(name.c_str() + 4);
                md->GetSoundChannel()->fn[key] = value;
            }
            else if (CHKCMD(name,"#exbpm",6) || CHKCMD(name,"#bpm",4)) {
                int key;
                if (name.c_str()[1] == 'b')
                    key = atoi_bms(name.c_str() + 4);
                else
                    key = atoi_bms(name.c_str() + 6);
                md->GetBPMChannel()->bpm[key] = atof( value.c_str() );
            }
            else if (CHKCMD(name,"#stop",5)) {
                int key = atoi_bms(name.c_str() + 5);
                md->GetSTOPChannel()->stop[key] = atoi(value.c_str());  // 1/192nd
            }
            else if (name == "#stp") {
                std::string sMeasure, sTime;
                if (!split(value, ' ', sMeasure, sTime))
                {
                    printf("invalid #STP found, ignore. (%s)\n", value.c_str());
                    continue;
                }
                float fMeasure = atof(sMeasure.c_str());
                md->GetSTOPChannel()->STP[fMeasure] = atoi(sTime.c_str());
            }
            // TODO: WAVCMD, EXWAV
            // TODO: MIDIFILE
            #undef CHKCMD
        }
    }
}

void ChartLoaderBMS::ReadObjects(const char* p, int iLen)
{
    TimingData* td = c->GetTimingData();
    NoteData* nd = c->GetNoteData();
    MetaData* md = c->GetMetaData();

    // clear objects and set resolution here
    c->ChangeResolution(DEFAULT_RESOLUTION_SIZE);

	// before start to read objects, obtain BPM from metadata
	td->AddObject(BpmObject(0, 0, md->fBPM));

    // measure length caching array <measure index, measure length>
    std::map<int, float> measurelen;
    measurelen[0] = 1.0f;
    std::vector<BmsNote> vNotes;

    // Obtain Bms objects or set measure length
    const char *pp = p;
	std::map<int, int> vPrevVal;   // previous value of each channel
    std::map<int, int> bgm_channel_idx;
    std::string name, value;
    while (1) {
        if (*pp == 0) break;
        ParseLine(&pp, name, value, ':');
        if (name.empty()) continue;
        int measure = atoi(name.substr(1, 3).c_str());
        int channel = atoi_bms16(name.substr(4, 2).c_str());
		if (channel == 0)
		{
			// invalid channel; case like 'title: ~~~'
			continue;
		}
        if (channel == 2) {
            // record measure size change
            // but recover original length at next measure
            measurelen[measure] = atof(value.c_str());
            measurelen[measure + 1] = 1.0f;
        } else {
            // just append bmsnote data
            // in case of bgm data, need to set colidx value
            BmsNote n;
            int colidx = 0;
            if (channel == 1) {
                colidx = bgm_channel_idx[measure]++;
            }
            int cur_res = value.size()/2;
            for (int i=0; i<cur_res; i++) {
				// less than note channel(0x10), then use atoi_bms16
				if (channel < 0x10)
					n.value = atoi_bms16(value.c_str() + i * 2, 2);
				else
					n.value = atoi_bms(value.c_str() + i*2, 2);
                n.value_prev = vPrevVal[channel];
				vPrevVal[channel]=n.value;
                if (n.value == 0) { continue; }
                n.measure = measure;
                n.channel = channel;
                n.den = cur_res;
                n.num = i;
                n.colidx = colidx;
                vNotes.push_back(n);
            }
        }
    }
	bgm_channel_idx.clear();
	vPrevVal.clear();
    std::sort(vNotes.begin(), vNotes.end());


    // record measure-length change (after resolution is set)
    float prev_ml=1.0f, cur_ml=1.0f;
    for (auto &m : measurelen)
    {
        int cur_midx = m.first;
        cur_ml = m.second;
        if (prev_ml != cur_ml)
        {
            td->AddObject(MeasureObject(cur_midx, cur_ml));
        }
        prev_ml = cur_ml;
    }


    /* 
     * this section we place objects,
     * So row resolution MUST be decided before we enter this section.
     */
    // allow note duplication to reduce load time and resolution loss
    nd->SetNoteDuplicatable(1);

    // measure position will be summarized
    int measure_prev = 0;
    double measure_len_sum = 0;
    Note n;
    // TODO: set vector size before appending?
    for (auto &bn: vNotes)
    {
        while (bn.measure > measure_prev)
        {
            measure_prev++;
            if (IN_MAP(measurelen, measure_prev))
                measure_len_sum += measurelen[measure_prev];
            else
                measure_len_sum += 1.0f;
        }
        n.iRow = measure_len_sum * td->GetResolution() + td->GetResolution() * bn.num / bn.den;
        n.fBeat = measure_len_sum + bn.num / (float)bn.den;
        n.nType = NoteType::NOTE_EMPTY;
        n.iValue = bn.value;        n.iEndValue = 0;
        n.iDuration = n.fBeatLength = 0;
        n.fTime = n.fTimeLength = 0;
		n.x = n.y = 0;
        n.restart = true;
        int channel = bn.channel;
        int track = bn.channel % 16;
        if (channel == 1) {
            // BGM
            n.nType = NoteType::NOTE_BGM;
            n.subType = NoteTapType::TAPNOTE_EMPTY;
            n.x = bn.colidx;
        }
        else if (channel == 3) {
            // BPM change
			// fill TimingData directly.
			td->AddObject(BpmObject(n.iRow, n.fBeat, n.iValue));
            n.nType = NoteType::NOTE_EMPTY;
        }
        else if (channel == 4) {
            // BGA
            n.nType = NoteType::NOTE_BGA;
            n.subType = NoteBgaType::NOTEBGA_BGA;
        }
        else if (channel == 6) {
            // BGA POOR
            n.nType = NoteType::NOTE_BGA;
            n.subType = NoteBgaType::NOTEBGA_MISS;
        }
        else if (channel == 7) {
            // BGA LAYER
            n.nType = NoteType::NOTE_BGA;
            n.subType = NoteBgaType::NOTEBGA_LAYER1;
        }
        else if (channel == 8) {
            // BPM change (exBPM)
            n.nType = NoteType::NOTE_BMS;
            n.subType = NoteBmsType::NOTEBMS_BPM;
        }
        else if (channel == 9) {
            // STOP
            n.nType = NoteType::NOTE_BMS;
            n.subType = NoteBmsType::NOTEBMS_STOP;
        }
        else if (channel == 10) {
            // BGA LAYER2
            n.nType = NoteType::NOTE_BGA;
            n.subType = NoteBgaType::NOTEBGA_LAYER2;
        }
        else if (channel == 11) {
            // opacity of BGA
            n.nType = NoteType::NOTE_BMS;
            n.subType = NoteBmsType::NOTEBMS_BGAOPAC;
            n.x = 0;
        }
        else if (channel == 12) {
            // opacity of BGA LAYER
            n.nType = NoteType::NOTE_BMS;
            n.subType = NoteBmsType::NOTEBMS_BGAOPAC;
            n.x = 1;
        }
        else if (channel == 13) {
            // opacity of BGA LAYER2
            n.nType = NoteType::NOTE_BMS;
            n.subType = NoteBmsType::NOTEBMS_BGAOPAC;
            n.x = 2;
        }
        else if (channel == 14) {
            // opacity of BGA POOR
            n.nType = NoteType::NOTE_BMS;
            n.subType = NoteBmsType::NOTEBMS_BGAOPAC;
            n.x = 3;
        }
        else if (channel >= 0x11 && channel <= 0x19 ||
                channel >= 0x21 && channel <= 0x29) {
            // 1P/2P visible note
            int track = channel % 16 + channel / 0xE0 * 10;
            if (n.iValue == md->iLNObj)
            {
                // LNOBJ process
                // change previous note to LN
                Note* n_ln = nd->GetLastNoteAtTrack(track);
                ASSERT(n_ln);   // previous note should be existed
                n_ln->subType = NoteTapType::TAPNOTE_CHARGE;
                n_ln->fBeatLength = n.fBeat - n_ln->fBeat;
                n_ln->iDuration = n.iRow - n_ln->iRow;
            }
            else {
                n.nType = NoteType::NOTE_TAP;
                n.subType = NoteTapType::TAPNOTE_TAP;
                n.x = track;
            }
        }
        else if (channel >= 0x31 && channel <= 0x39 ||
                channel >= 0x41 && channel <= 0x49) {
            // 1P/2P invisible note
            int track = channel % 16 + channel / 0xE0 * 10;
            n.nType = NoteType::NOTE_BMS;
            n.subType = NoteBmsType::NOTEBMS_INVISIBLE;
            n.x = track;
        }
        else if (channel >= 0x51 && channel <= 0x59 ||
                channel >= 0x61 && channel <= 0x69) {
            // 1P/2P LN
            // LNTYPE 1
            if (md->iLNType == 1)
            {
                Note* n_ln = nd->GetLastNoteAtTrack(track, NoteType::NOTE_TAP, NoteTapType::TAPNOTE_CHARGE);
                if (n_ln == 0 || n_ln->iDuration) {
                    // append new LN
                    n.nType = NoteType::NOTE_TAP;
                    n.subType = NoteTapType::TAPNOTE_CHARGE;
                    n.x = track;
                }
                else
                {
                    // add length information to previous LN
                    // COMMENT: if end of TAPNOTE not found then?
                    // COMMENT: add option to GetLastNoteAtTrack with subtype?
                    n_ln->fBeatLength = n.fBeat - n_ln->fBeat;
                    n_ln->iDuration = n.iRow - n_ln->iRow;
                    n_ln->iEndValue = n.iValue;
                }
            }
            // LNTYPE 2
            else if (md->iLNType == 2)
            {
                // append new LN if value_prev == 0
                if (bn.value_prev == 0)
                {
                    n.nType = NoteType::NOTE_TAP;
                    n.subType = NoteTapType::TAPNOTE_CHARGE;
                    n.x = track;
                }
                else
                {
                    // add length information to previous LN
                    Note* n_ln = nd->GetLastNoteAtTrack(track);
                    ASSERT(n_ln);
                    n_ln->fBeatLength = n.fBeat - n_ln->fBeat;
                    n_ln->iDuration = n.iRow- n_ln->iRow;
                    n_ln->iEndValue = n.iValue;
                }
            }
            else
            {
                printf("[Warning] Invalid LNType %d, but found LN note.\n", md->iLNType);
            }
        }
        else if (channel >= 0xD1 && channel <= 0xD9 ||
                channel >= 0xE1 && channel <= 0xE9) {
            // 1P/2P mine
            int track = channel % 10 + channel / 0xE1;
            n.nType = NoteType::NOTE_TAP;
            n.subType = NoteTapType::TAPNOTE_MINE;
            n.x = track;
        }

        if (n.nType != NoteType::NOTE_EMPTY) 
            nd->AddNote(n);
    }
    nd->SetNoteDuplicatable(0);

    // now fill timingdata from metadata/notedata.
	c->LoadExternObject();
    c->UpdateTimingData();
}


} /* rparser */
