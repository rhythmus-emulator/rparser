/* supports bms, bme, bml formats. */

#include "NoteLoader.h"
#include "Chart.h"
#include "util.h"
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>

namespace rparser {
/* common */
void ParseLine(const std::string& line, std::string& name, std::string& value, char sep=0)
{
    // trim
    auto wsfront=std::find_if_not(line.begin(),line.end(),[](int c){return std::isspace(c);});
    auto wsback=std::find_if_not(line.rbegin(),line.rend(),[](int c){return std::isspace(c);}).base();
    std::string line_trim = (wsback<=wsfront ? std::string() : std::string(wsfront,wsback));

    size_t space = std::string::npos;
    if (sep == 0) {
        // find any type of space
        space = line_trim.find(' ');
        if (space == std::string::npos) space=line_trim.find('\t');
    } else {
        space = line_trim.find(sep);
    }
    std::string name = line_trim.substr(0, space); lower(name);
    std::string value = std::string();
    if (space != std::string::npos) value = line_trim.substr(space+1);
}
void ParseLine(const char** p, std::string& name, std::string& value, char sep=0) {
    while (**p != '\t' && **p != ' ' && **p) *p++;
    // start point
    char *s = *p;
    while (**p != '\n' && **p) *p++;
    if (*p == *s) { name.clear(); }
    else {
        std::string line(*s, *p-*s);
        if (*p) *p++;
        ParseLine(line, name, value);
    }
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
        virtual bool Valid() = 0;
        virtual std::string GesubType() = 0;
        virtual void Parse(const std::string& cmd_lower, const std::string& value) = 0;
    };
    class IFCondition : public Condition {
        int target;      // active value
        int activecnt;  // condition is activated?
        int activeidx;  // current condition is active?
        int condidx;    // current condition index
        IFCondition() : 
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
        SWITCHCondition() : 
            stat(0), value(active_value) {}

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
                ASSERT(condidx > 0);
            } else if (name == "#switch" || name == "#setswitch") {
                // initialize random value
                value = atoi(value.c_str()) * rand();
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
    BMSExpandProc(Chart *c, int iSeed=-1, bAllowExpand=true):
        c(c), m_iSeed(iSeed), m_bAllowExpand(bAllowExpand), active_value(0) {}

    bool IsCurrentValidStatement() {
        return (conds.size() == 0 || (m_bAllowExpand && conds.back().Valid()));
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
                curr_cond = new IFCondition()
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
                curr_cond = new SWITCHCondition()
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
            while (p < p_end && *p == '\n') p++;
            // make trimmed string
            const char *s=in, *e=p;
            while (*s == ' ' || *s == '\t') s++;
            while (e > s && (*e == ' ' || *e == '\t')) e--;
            std::string line(s, e-s);
            // prepare for next line
            in = p+1;
            // process line
            ProcessStatement(line);
        }
    }

    std::string& GetProcCmd() { return m_sProcCmd; }
    std::string& GetExpandCmd() { return m_sExpandCmd; }
};


// main file
bool ChartLoaderBMS::Test( const char* p, int iLen )
{
    // As bms file has no file signature,
    // just search for "*--" string for first 100 string.
    // TODO: is there any exception?
    for (int i=0; i<iLen-4 && i<100; i++) {
        if (stricmp(p, "*-----", 6)==0)
            return true;
    }
    return false;
}

bool TestName( const char *fn )
{
    std::string fn_lower = fn;  lower(fn_lower);
    return EndsWith(fn_lower, ".bms") ||
        EndsWith(fn_lower, ".bme") ||
        EndsWith(fn_lower, ".bml") ||
        EndsWith(fn_lower, ".pms");
}

bool ChartLoaderBMS::Load( const char* p, int iLen )
{
    // parse expand, and process it first
    // (expand parse effects to Metadata / Channels)
    BMSExpandProc bExProc(c, m_iSeed, procExpansion);
    bExProc.proc(p, iLen);
    std::string& proc_res = bExProc.GetProcCmd();
    c->GetMetaData()->m_sExpand = bExProc.GetExpandCmd();

    // parse metadata(include expanded command) first
    ReadHeader(proc_res.c_str(), proc_res.size());

    // parse BGA & notedata channel second
    ReadObjects(proc_res.c_str(), proc_res.size(), notes);
    ProcObjects(notes);
    notes.clear();

    // set encoding from EUC-JP to UTF8
    c->GetMetaData()->SetEncoding("EUC-JP", "UTF8");

    // fill hash file
    m_ChartSummaryData.FillHash(p, iLen);

    // we don't call EvaluateBeat in here,
    // as fBeat is automatically calculated 
    // while reading objects. (in good resolution)

    return true;
}

void ChartLoaderBMS::ReadHeader(const char* p, int iLen)
{
    const char **pp = *p;
    std::string name, value;
    while (1) {
        if (**pp == 0) break;
        const char *pp_save = *pp;
        ParseLine(*pp, name, value);
        if (!name.empty()) {
            if (name == "#title") {
                c->GetMetaData()->sTitle = value;
            }
            else if (name == "#subtitle") {
                c->GetMetaData()->sSubTitle = value;
            }
            else if (name == "#artist") {
                c->GetMetaData()->sArtist = value;
            }
            else if (name == "#subartist") {
                c->GetMetaData()->sSubArtist = value;
            }
            else if (name == "#genre") {
                c->GetMetaData()->sGenre = value;
            }
            else if (name == "#player") {
                c->GetMetaData()->iPlayer = atoi(value.c_str());
            }
            else if (name == "#playlevel") {
                c->GetMetaData()->iLevel = atoi(value.c_str());
            }
            else if (name == "#difficulty") {
                c->GetMetaData()->iDifficulty = atoi(value.c_str());
            }
            else if (name == "#rank") {
                // convert from 4 to 100.0
                c->GetMetaData()->fJudge = atof(value.c_str()) / 4.0 * 100;
            }
            else if (name == "#total") {
                c->GetMetaData()->fTotal = atof(value.c_str());
            }
            else if (name == "#banner") {
                c->GetMetaData()->sBannerImage = value;
            }
            else if (name == "#backbmp") {
                c->GetMetaData()->sBackImage = value;
            }
            else if (name == "#stagefile") {
                c->GetMetaData()->sStageImage = value;
            }
            else if (name == "#bpm") {
                c->GetMetaData()->fBPM = atof(value.c_str());
            }
            else if (name == "#lntype") {
                c->GetMetaData()->iLNType = atoi(value.c_str());
            }
            else if (name == "#lnobj") {
                c->GetMetaData()->sLNObj = value;
            }
            else if (name == "#music") {
                c->GetMetaData()->sMusic = value;
            }
            else if (name == "#preview") {
                c->GetMetaData()->sPreviewMusic = value;
            }
            else if (name == "#offset") {
                // append it to TimingData
                c->GetTimingData()->fBeat0MSecOffset = atof(value.c_str());
            }

            /*
             * Resource part
             */
            #define CHKCMD(s, cmd, len) (stricmp(s.c_str(), cmd, len) == 0)
            else if (CHKCMD(name,"#bmp",4)) {
            }
            else if (CHKCMD(name,"#wav"),4) {
            }
            else if (CHKCMD(name,"#exbpm",6) || CHKCMD(name,"#bpm",4)) {
            }
            else if (CHKCMD(name,"#stop",5)) {
                // COMMENT
                // We don't fill STOP data here,
                // we call LoadFromMetaData() instead.
            }
            else if (CHKCMD(name,"#stp",4)) {
            }
            // TODO: WAVCMD, EXWAV
            // TODO: MIDIFILE
            #undef CHKCMD
        }
    }
}

void ChartLoaderBMS::ReadObjects(const char* p, int iLen)
{
    // clear objects and set resolution here
    c->SetResolution(DEFAULT_RESOLUTION_SIZE);

    // don't accept measure size at first, just record it in array first.
    // (support up to 10000 internally)
    float measurelen[10000];
    for (int i=0;i<10000; i++)
        measurelen[i] = 1.0f;
    std::vector<BmsNote> vNotes;
    int value_prev[1000];
    memset(value_prev,0,sizeof(int)*1000);

    // Obtain Bms objects or set measure length
    *pp = *p;
    std::map<int, int> bgm_channel_idx;
    while (1) {
        if (**pp == 0) break;
        ParseLine(*pp, name, value, ':');
        if (name.empty()) continue;
        int measure = atoi(name.substr(1, 3).c_str());
        int channel = atoi(name.substr(4, 2).c_str());
        if (channel == 2) {
            // record measure size change
            measurelen[measure] = atof(value.c_str());
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
                n.measure = measure;
                n.channel = channel;
                n.den = cur_res;
                n.num = i;
                n.value = atoi_bms(value.c_str() + i*2);
                n.value_prev = value_prev[channel]; value_prev[channel]=n.value;
                n.colidx = colidx;
                vNotes.append(n);
            }
        }
    }
    std::sort(vNotes);


    // record measure-length change (after resolution is set)
    int prev_ml=4, cur_ml=4;
    for (int i=0; i<10000; i++)
    {
        cur_ml = measurelen[i];
        if (prev_ml != cur_ml)
            c->GetTimingData()->AddObject(
                new MeasureObject(measure, int(atof(value.c_str()) + 0.001))
                );
        prev_ml = cur_ml;
    }


    /* 
     * this section we place objects,
     * So row resolution MUST be decided before we enter this section.
     */
    TimingData* td = c->GetTimingData();
    NoteData* nd = c->GetNoteData();
    // allow note duplication to reduce load time and resolution loss
    nd->SetNoteDuplicatable(1);

    // measure position will be summarized
    int measure_prev = 0;
    double measure_len_sum = 0;
    Note n;
    for (auto &bn: vNotes)
    {
        while (bn.measure > measure_prev)
        {
            measure_prev++;
            measure_len_sum += measurelen(measure_prev);
        }
        n.iValue = bn.value;
        n.iRow = measure_len_sum * td->GetResolution() + td->GetResolution() * bn.num / bn.den;
        n.fBeat = measure_len_sum + bn.num / (float)bn.den;
        n.nType = NoteType::NOTE_EMPTY;
        n.iValue = bn.value;
        n.iEndValue = 0;
        n.iDuration = n.fDuration = 0;
        n.restart = true;
        int channel = bn.channel;
        if (channel == 1) {
            // BGM
            n.nType = NoteType::NOTE_BGM;
            n.subType = NoteTapType::TAPNOTE_EMPTY;
            n.x = bn.colidx;
        }
        else if (channel == 3) {
            // BPM change
            // COMMENT: we don't fill timingdata directly in here,
            // we call LoadFromNoteData() instead.
            n.nType = NoteType::NOTE_BPM;
            n.subType = NoteTapType::TAPNOTE_EMPTY;
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
            int track = channel % 10 + channel / 0xE1;
            if (n.iValue == md->iLNObj)
            {
                // LNOBJ process
                // change previous note to LN
                Note* n_ln = nd->GetLastNoteAtTrack(track);
                ASSERT(n_ln);   // previous note should be existed
                n_ln->subType = NoteTapType::TAPNOTE_CHARGE;
                n_ln->fDuration = n.fBeat - n_ln->fBeat;
                n_ln->iDuration = n.iBeat - n_ln->iBeat;
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
            int track = channel % 10 + channel / 0xE1;
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
                    n_ln->fDuration = n.fBeat - n_ln->fBeat;
                    n_ln->iDuration = n.iBeat - n_ln->iBeat;
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
                    n_ln->fDuration = n.fBeat - n_ln->fBeat;
                    n_ln->iDuration = n.iBeat - n_ln->iBeat;
                    n_ln->iEndValue = n.iValue;
                }
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

        if (n.ntype != NoteType::NOTE_EMPTY) 
            nd->AddNote(n);
    }
    nd->SetNoteDuplicatable(0);

    // now fill timingdata from metadata/notedata.
    c->UpdateTimingData();
}


} /* rparser */