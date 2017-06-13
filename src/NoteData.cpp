#include "NoteData.h"
#include "util.h"
#include <sstream>
#include <algorithm>

namespace rparser
{

char *n_type[] = {
    "NOTE_EMPTY",
    "NOTE_TAP",
    "NOTE_TOUCH",
    "NOTE_BGM",
    "NOTE_BGA",
    "NOTE_MIDI",
    "NOTE_BMS",
    "NOTE_REST",
    "NOTE_SHOCK",
};
char *n_subtype_tap[] = {
    "TAPNOTE_EMPTY",
    "TAPNOTE_TAP",
    "TAPNOTE_INVISIBLE",
    "TAPNOTE_CHARGE",
    "TAPNOTE_HCHARGE",
    "TAPNOTE_TCHARGE",
    "TAPNOTE_MINE",
    "TAPNOTE_AUTOPLAY",
    "TAPNOTE_FAKE",
    "TAPNOTE_EXTRA",
    "TAPNOTE_FREE",
};
char *n_subtype_bga[] = {
    "NOTEBGA_EMPTY",
    "NOTEBGA_BGA",
    "NOTEBGA_MISS",
    "NOTEBGA_LAYER1",
    "NOTEBGA_LAYER2",
};
char *n_subtype_bms[] = {
    "NOTEBMS_EMPTY",
    "NOTEBMS_BPM",
    "NOTEBMS_STOP",
    "NOTEBMS_BGAOPAC",
    "NOTEBMS_INVISIBLE",
};

std::string Note::toString()
{
    std::stringstream ss;
    std::string sType, sSubtype;
    sType = n_type[nType];
    switch (nType)
    {
        case NOTE_TAP:
        case NOTE_TOUCH:
            sSubtype = n_subtype_tap[subType];
            break;
        case NOTE_BGA:
            sSubtype = n_subtype_bga[subType];
            break;
        case NOTE_BGM:
            sSubtype = "(COL NUMBER)";
            break;
        case NOTE_BMS:
            sSubtype = n_subtype_bms[subType];
            break;
        default:
            sSubtype = "(UNSUPPORTED)";
            break;
    }
    ss << "[Object Note]\n" ;
    ss << "type/subtype: " << sType << "," << sSubtype << 
        " (" << nType << "," << subType << ")\n";
    ss << "Value / EndValue: " << iValue << "," << iEndValue << std::endl;
    ss << "Beat: " << fBeat << std::endl;
    ss << "Row / iDuration: " << iRow << "," << iDuration << std::endl;
    ss << "Time / fDuration: " << fTime << "," << fDuration << std::endl;
    ss << "x / y: " << x << "," << y << std::endl;
    return ss.str();
}

Note* NoteData::GetLastNoteAtTrack(int iTrackNum=-1, int iType=-1, int iSubType=-1)
{
    // iterate from last to first
    for (auto it=vNotes.rbegin(); it != vNotes.rend(); ++it)
    {
        if (iTrackNum < 0 || it->iTrack == iTrackNum)
        if (iType < 0 || it->iType == iType)
        if (iSubType < 0 || it->iSubType == iSubType)
            return *it;
    }
    // found no note
    return 0;
}

bool NoteData::operator<( const NoteData &other ) const
{
	// first compare beat, next track(x).
	return (fBeat == other.fBeat) ? x < other.x : fBeat < other.fBeat;
}

void FillNoteTimingData(std::vector<Note>& vNotes, const TimingData& td)
{
    float curr_beat= -1;
    double curr_row_second= -1.0;
    Note* curr_note;
    for(int i=0; i<vNotes.size(); ++i)
    {
        curr_beat = vNotes[i].fBeat;
        curr_note = vNotes[i].n;
        // time calculation for current row
        if(curr_note.fBeat != curr_beat)
        {
            curr_beat= curr_note.fBeat();
            curr_row_second= timing_data->GetElapsedTimeFromBeat(curr_beat);
            ++curr_row_id;
        }
        if(curr_note->nType != NoteType::NOTE_EMPTY)
        {
            curr_note->occurs_at_second= curr_row_second;
            curr_note->id_in_chart= static_cast<float>(curr_note_id);
            // in case of hold note, calculate end time too
            if(curr_note->type == TapNoteType::TAPNOTE_CHARGE || 
               curr_note->type == TapNoteType::TAPNOTE_HCHARGE || 
               curr_note->type == TapNoteType::TAPNOTE_TCHARGE)
            {
                curr_note->end_second= timing_data->GetElapsedTimeFromBeat(NoteRowToBeat(curr_row + curr_note->iDuration));
            }
        }
    }
}
// @description
// fill all note's timing data from row(beat) data.
// MUST have correct beatdata first to get correct timing.
void NoteData::FillTimingData(const TimingData& td)
{
    // prepare lookup
    td.PrepareLookup();

    std::vector<Note>& vNotes = m_Track;
    
    // now lets track all note for time calculation
    FillNoteTimingData(m_Track, td);

    // finish? TODO: make internal counter
    td->ReleaseLookup();
}


/*
* metadata utilities
*/
int NoteData::GetTrackCount()
{
    return m_Tracks.size();
}

void NoteData::SetTrackCount(int iTrackCnt)
{
    ASSERT(iTrackCnt > 0);
    m_Tracks.resize( iTrackCnt );
}

int NoteData::GetNoteCount()
{
    int i=0;
    int iNoteCount=0;
    for (i=0; i<GetTrackCount(); i++)
        iNoteCount += GetNoteCount(i);
    return iNoteCount;
}

int NoteData::GetNoteCount(int iTrackNum)
{
    return m_Tracks[iTrackNum].size();
}

// @description 
int NoteData::GetLastNoteRow()
{
    return 0;
}

int NoteData::GetLastNoteTime()
{
    return 0;
}

bool NoteData::IsHoldNoteAtRow(int track, int row)
{
    return 0;
}

NoteType NoteData::GetNoteTypeAtRow(int track, int row)
{
    return 0;
}

Note* NoteData::GetNoteAtRow(int track, int row)
{
    return 0;
}

bool NoteData::IsRangeEmpty(int track, int iStartRow, int iEndRow)
{
    return 0;
}

bool NoteData::IsRowEmpty(int row)
{
    return 0;
}

bool NoteData::IsTrackEmpty(int track)
{
    return 0;
}

bool NoteData::IsEmpty()
{
    return 0;
}

void NoteData::CopyRange(int rowFromBegin, int rowFromLength, int rowToBegin)
{
    return 0;
}

void NoteData::SetNoteDuplicatable(int bNoteDuplicatable)
{
    m_bNoteDuplicatable = bNoteDuplicatable;
}

void NoteData::AddNote(const Note& n)
{
    // COMMENT: note duplication won't be checked when loading file.
    // COMMENT: m_bNoteDuplicatable does these work -
    // - if LN, then remove all iRow object in iDuration in same x.
    // - if TapNote/BGM, then check iRow/x/y.

    // search is same type of note exists in same row
    int idx = vNotes.begin() - vNotes.lower_bound(n);
    if (m_bNoteDuplicatable)
    {
        // only check for object in same row
        while (idx < vNotes.size() && vNotes[idx].iRow == n.iRow)
        {
            // if found note occupies same type & row & position, then erase it.
            if (vNotes[idx].nType == n.nType && 
                vNotes[idx].x == n.x && vNotes[idx].y == n.y)
            {
                vNotes.erase(vNotes.begin()+idx);
            }
            // if found note is row-occupying-object (ex: SHOCK), erase it
            else if (vNotes[idx].nType == NoteType::NOTE_SHOCK ||
                    vNotes[idx].nType == NoteType::NOTE_REST)
            {
                vNotes.erase(vNotes.begin()+idx);
            }
            else
            {
                idx ++;
            }
        }
        // iterate from here to first to check LN
        int idx_copy = idx;
        while (idx_copy => 0)
        {
            if (vNotes[idx_copy].tType == TapNoteType::TAPNOTE_CHARGE ||
                vNotes[idx_copy].tType == TapNoteType::TAPNOTE_HCHARGE ||
                vNotes[idx_copy].tType == TapNoteType::TAPNOTE_TCHARGE ||
                vNotes[idx_copy].tType == TapNoteType::TAPNOTE_FREE)
                {
                    // check for range for ranged-note
                    // (assume only 1 longnote can be overlapped,
                    //  to prevent performance decreasing by all-range-search)
                    if (vNotes[idx_copy].x == n.x &&
                        vNotes[idx_copy].y == n.y &&
                        vNotes[idx_copy].iRow+vNotes[idx_copy].iDuration > n.iRow)
                    {
                        vNotes.erase(vNotes.begin()+idx_copy);
                        break;
                    }
                }
            --idx_copy;
        }
    }
    vNotes.insert(vNotes.begin()+idx, n);
}

void NoteData::TrackMapping(int tracknum, int *trackmap, int s, int e)
{
    // only map for TAPNOTE
    for (auto &n: vNotes)
    {
        if (n.nType != NoteType::NOTE_TAP) continue;
		if (n.x < s || n.x >= e) continue;
        n.x = trackmap[n.x-s]+s;
    }
}

void NoteData::TrackSRandom(int side, int key, bool bHrandom)
{
	ASSERT(key<10 && key > 0 && side >= 0 && side < 2);
	int vShuffle[10] = {
		0,1,2,3,4,5,6,7,8,9
	};
	float curr_LN_length[10];
	for (int i=0; i<10; i++) curr_LN_length[i]=0;

	float curr_beat = -100;		// current lane's beat
	int curr_side = 0;			// current note's lane side
	int curr_shuffle_idx = 0;	// shuffling index
	int curr_note_cnt = 0;		// note count of current beat
	for (auto &n: vNotes) {
		if (n.nType != NoteType::NOTE_TAP) continue;
		curr_side = n.x / 10;
		if (curr_side != side) continue;
		if (curr_beat != n.fBeat)
		{
			// new beat? then shuffle value.
			// (no HRANDOM)
			if (!bHrandom)
			{
				std::random_shuffle(vShuffle, vShuffle+key);
				curr_shuffle_idx = 0;
			}
			float delta_beat = n.fBeat - curr_beat;
			curr_beat = n.fBeat;
			// check longnote ends
			for (int i=0; i<key; i++)
			{
				curr_LN_length[i] -= delta_beat;
				if (curr_LN_length[i] < 0) curr_LN_length[i] = 0;
			}
			curr_note_cnt = 0;
		}
		// prevent duplication with LN
		while (curr_LN_length[ vShuffle[curr_shuffle_idx] ] > 0
			&& curr_shuffle_idx < key)
		{
			curr_shuffle_idx++;
			curr_note_cnt++;
		}
		if (curr_shuffle_idx >= key)
		{
			// this part will work in case of H-RANDOM
			// prevent note jackhammering by minimizing note shuffling
			// COMMENT:
			// If current lane contains 2 Notes (in 9 keys),
			// Then shuffle 7 note backward and 2 note for forward
			// to minimize note hammering.
			std::random_shuffle(vShuffle, vShuffle+curr_note_cnt);
			std::random_shuffle(vShuffle+curr_note_cnt, vShuffle+key-curr_note_cnt);
			curr_shuffle_idx=0;
		}
		n.x = vShuffle[curr_shuffle_idx] + curr_side*10;
		curr_shuffle_idx++;
		curr_note_cnt++;
		// if current note is LN, then remember
		if (n.subType == NoteTapType::TAPNOTE_CHARGE)
		{
			curr_LN_length[ vShuffle[curr_shuffle_idx] ] = n.fDuration;
		}
	}
}

void NoteData::TrackRandom(int side, int key)
{
	ASSERT(side >= 0 && key >= 0 && key < 10);
	int vShuffle[10] = {0,1,2,3,4,5,6,7,8,9};
	std::random_shuffle(vShuffle, vShuffle+key);
	TrackMapping(key, vShuffle, side*10, side*10+key);
}

void NoteData::TrackRRandom(int side, int key)
{
	ASSERT(side >= 0 && key >= 0 && key < 10);
	int vShuffle[10] = {0,1,2,3,4,5,6,7,8,9};
	int iMoveVal = rand()*key;
	for (int i=0; i<key; i++)
	{
		vShuffle[i] -= iMoveVal;
		if (vShuffle[i] < 0) vShuffle[i] += key;
	}
	TrackMapping(key, vTrackmap, side*10, side*10+key);
}

void NoteData::TrackMirror(int side, int key)
{
	ASSERT(side >= 0 && key >= 0 && key < 10);
	int vTrackmap[10];
	for (int i=0; i<key; i++)
	{
		vTrackmap[i] = key-i-1;
	}
	TrackMapping(key, vTrackmap, side*10, side*10+key);
}

void NoteData::TrackAllSC(int side)
{
	float curr_beat = -100;		// current beat (to detect beat changing)
	int curr_sc = 0;			// current row has SC?
	std::vector<Note*> vNotes_sc_candidate;
	for (auto &n: vNotes)
	{
		if (n.fBeat != curr_beat)
		{
			// select one note and make it SC
			if (curr_sc == 0)
			{
				Note* n_sc_candidate = vNotes_sc_candidate[ vNotes_sc_candidate.size() * rand() ];
				n_sc_candidate->x = 0;
			}
			// prepare for next
			curr_sc = 0;
			curr_beat = n.fBeat;
			vNotes_sc_candidate.clear();
		} else if (curr_sc) {
			continue;
		}
		// SC channel : 0
		if (n.x == 0)
		{
			curr_sc = 1;
			continue;
		}
		vNotes_sc_candidate.push_back(&n);
	}
}

void NoteData::TrackFlip()
{
	int vTrackmap[20] = {
		10,11,12,13,14,15,16,17,18,19,
		0,1,2,3,4,5,6,7,8,9,
	};
	TrackMapping(key, vTrackmap, 0, 20);
}

std::string const NoteData::toString()
{
    std::stringstream ss;
    ss << "Total Note count: " << vNotes.size() << std::endl;
    ss << "Last note Information\n" << vNotes.back().toString();
    return ss.str();
}

void NoteData::SetResolution(int iRes)
{
    float fRatio = (float)iRes / m_iRes;
        for (auto n: m_Track)
        {
            n.iRow *= fRatio;
            n.iDuration *= fRatio;
        }
    }
    m_iRes = iRes;
}

void NoteData::UpdateBeatData()
{
    // calculate note's beat data from resolution
    for (auto &n: m_Track)
    {
        n.fBeat = n.iRow / (float)m_iRes;
    }
}

NoteData::NoteData()
{
	m_iRes = DEFAULT_RESOLUTION_SIZE;
    m_bNoteDuplicatable = 0;
}

NoteData::~NoteData()
{

}

} /* namespace rparser */