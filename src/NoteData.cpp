#include "NoteData.h"
#include "Chart.h"
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
    ss << "Beat / BeatLength: " << fBeat << "," << fBeatLength << std::endl;
    ss << "Row / iDuration: " << iRow << "," << iDuration << std::endl;
    ss << "Time / fDuration: " << fTime << "," << fTimeLength << std::endl;
    ss << "x / y: " << x << "," << y << std::endl;
    return ss.str();
}

bool Note::operator<( const Note &other ) const
{
	// first compare beat, next track(x).
	return (fBeat == other.fBeat) ? x < other.x : fBeat < other.fBeat;
}
bool Note::operator==(const Note &other) const
{
	return
		nType == other.nType &&
		subType == other.subType &&
		iRow == other.iRow &&
		x == other.x &&
		y == other.y;
}

bool Note::IsTappableNote()
{
    return true;
}
int Note::GetPlayerSide()
{
    return y / 10;
}
int Note::GetTrack()
{
    return y;
}

struct n_less_row : std::binary_function <Note, Note, bool>
{
	bool operator() (const Note& x, const Note& y) const
	{
		return x.iRow < y.iRow;
	}
};

struct nptr_less : std::binary_function <Note*, Note*, bool>
{
	bool operator() (const Note* x, const Note* y) const
	{
		return x->fBeat < y->fBeat;
	}
};

struct nptr_less_row : std::binary_function <Note*, Note*, bool>
{
	bool operator() (const Note* x, const Note* y) const
	{
		return x->iRow < y->iRow;
	}
};




// ------ class NoteData ------

Note* NoteData::GetLastNoteAtTrack(int iTrackNum, int iType, int iSubType)
{
    // iterate from last to first
    for (auto it = m_Track.rbegin(); it != m_Track.rend(); ++it)
    {
        if (iTrackNum < 0 || it->GetTrack() == iTrackNum)
            if (iType < 0 || it->nType == iType)
                if (iSubType < 0 || it->subType == iSubType)
                    return &(*it);
    }
    // found no note
    return 0;
}

void FillNoteTimingData(std::vector<Note>& vNotes, const TimingData& td)
{
    float curr_beat= -1;
    double curr_row_second= -1.0;
    Note* curr_note;
    for(int i=0; i<vNotes.size(); ++i)
    {
        curr_note = &vNotes[i];
        // time calculation for current row
        if(curr_note->fBeat != curr_beat)
        {
            curr_beat= curr_note->fBeat;
            curr_row_second = td.LookupMSecFromBeat(curr_beat);
        }
        if(curr_note->nType != NoteType::NOTE_EMPTY)
        {
            curr_note->fTime = curr_row_second;
            // in case of hold note, calculate end time too
            if(curr_note->subType == NoteTapType::TAPNOTE_CHARGE)
            {
                curr_note->fTimeLength = td.LookupMSecFromBeat(curr_beat + curr_note->fBeatLength);
            }
        }
    }
}
// @description
// fill all note's timing data from row(beat) data.
// MUST have correct beatdata first to get correct timing.
void NoteData::FillTimingData(TimingData& td)
{
    // prepare lookup
    td.PrepareLookup();

    std::vector<Note>& vNotes = m_Track;
    
    // now lets track all note for time calculation
    FillNoteTimingData(m_Track, td);

    // finish? TODO: make internal counter
    td.ClearLookup();
}


/*
* metadata utilities
*/
void NoteData::FillSummaryData(ChartSummaryData& cmd)
{
    cmd.iTrackCount = 0;
    cmd.iNoteCount = 0;
    cmd.isBomb = false;
    cmd.isBSS = false;
    cmd.isCharge = false;
    cmd.fLastNoteTime = 0;
    for (auto it = this->begin(); it != this->end(); ++it)
    {
        if (it->IsTappableNote())
        {
            cmd.iNoteCount++;
            if (cmd.fLastNoteTime < it->fTime + it->fTimeLength)
                cmd.fLastNoteTime = it->fTime + it->fTimeLength;
        }
        cmd.isBomb |= it->nType == NoteTapType::TAPNOTE_MINE;
        cmd.isCharge |= it->nType == NoteTapType::TAPNOTE_CHARGE;
        if (it->GetTrack() % 10 == 0)
            cmd.isBSS |= it->nType == NoteTapType::TAPNOTE_CHARGE;
        if (cmd.iTrackCount < it->GetTrack()) cmd.iTrackCount = it->GetTrack();
    }
}

void NoteData::CalculateNoteBeat()
{
    for (auto it = begin(); it != end(); ++it)
    {
        it->fBeat = it->iRow / (float)m_iRes;
        it->fBeatLength = it->iDuration / (float)m_iRes;
    }
}
bool NoteData::IsHoldNoteAtRow(int row, int track)
{
    // start from origin with selected track
    NoteSelection vSelected;
    GetNotesAtRow(vSelected, -1, track);
    for (auto it = vSelected.begin(); it != vSelected.end(); ++it)
    {
        Note *n = (*it);
        if (n->iRow <= row && n->iRow + n->iDuration >= row)
            return true;
    }
    return false;
}

void NoteData::GetNotesWithType(NoteSelection &vNotes, int nType, int subType)
{
    // iterate from first to end
    for (auto &n : m_Track)
    {

    }
}

void NoteData::GetNotesAtRow(NoteSelection &vNotes, int row, int track)
{
    // iterate from first to end
    for (auto &n : m_Track)
    {

    }
}

NoteType NoteData::GetNoteTypeAtRow(int row, int track)
{
    Note *n = GetNoteAtRow(row, track);
    if (!n) return NoteType::NOTE_EMPTY;
    else return n->nType;
}

Note* NoteData::GetNoteAtRow(int iRow, int track)
{
    int idx = GetNoteIndexAtRow(iRow, track);
    if (idx < 0) return 0;
    else return &m_Track[idx];
}

int NoteData::GetNoteIndexAtRow(int iRow, int track)
{
    // returns same or less object
    std::vector<Note>& vObjs = m_Track;
    if (vObjs.size() == 0) return -1;

    int min = 0, max = vObjs.size() - 1;
    int l = min, r = max;
    int curr_idx = 0;
    while (l <= r)
    {
        int m = (l + r) / 2;
        if ((m == min || vObjs[m].iRow <= iRow) && (m == max || iRow < vObjs[m + 1].iRow))
        {
            curr_idx = m;
            break;
        }
        else if (vObjs[m].iRow <= iRow)
        {
            l = m + 1;
        }
        else
        {
            r = m - 1;
        }
    }
    // check track
    while (curr_idx > 0 && vObjs[curr_idx - 1].iRow == iRow) curr_idx--;
    if (track < 0) return curr_idx;     // track==-1 -> return note with any track
    while (curr_idx < vObjs.size())
    {
        if (vObjs[curr_idx].iRow != iRow) break;
        if (vObjs[curr_idx].GetTrack() == track) return curr_idx;
        curr_idx++;
    }
    // no track found
    return -1;
}

NoteData::trackiter NoteData::lower_bound(int row)
{
    Note n; n.iRow = row;
    return std::lower_bound(m_Track.begin(), m_Track.end(), n, n_less_row());
}

NoteData::trackiter NoteData::upper_bound(int row)
{
    Note n; n.iRow = row;
    return std::upper_bound(m_Track.begin(), m_Track.end(), n, n_less_row());
}

bool NoteData::IsRangeEmpty(int iStartRow, int iEndRow)
{
    auto it = lower_bound(iStartRow);
    return (it->iRow > iEndRow);
}

bool NoteData::IsRowEmpty(int row)
{
    auto it = lower_bound(row);
    return (it->iRow != row);
}

bool NoteData::IsTrackEmpty(int track)
{
    for (auto it = begin(); it != end(); ++it)
    {
        if (it->GetTrack() == track)
            return false;
    }
    return true;
}

bool NoteData::IsEmpty()
{
    return m_Track.empty();
}

void NoteData::CopyRange(int rowFromBegin, int rowFromLength, int rowToBegin)
{
    return;
}

void NoteData::SetNoteDuplicatable(int bNoteDuplicatable)
{
    m_bNoteDuplicatable = bNoteDuplicatable;
}

void NoteData::AddNote(const Note& n_)
{
    // little trick: if fBeat == 0, then update fBeat value.
    Note n = n_;
    if (n.fBeat == 0)
    {
        n.fBeat = n.iRow / (float)m_iRes;
    }

    // COMMENT: note duplication won't be checked when loading file.
    // COMMENT: m_bNoteDuplicatable does these work -
    // - if LN, then remove all iRow object in iDuration in same x.
    // - if TapNote/BGM, then check iRow/x/y.

    // search is same type of note exists in same row
    int idx = m_Track.begin() - lower_bound(n);
    if (m_bNoteDuplicatable)
    {
        // only check for object in same row
        while (idx < m_Track.size() && m_Track[idx].iRow == n.iRow)
        {
            // if found note occupies same type & row & position, then erase it.
            if (m_Track[idx].nType == n.nType &&
                m_Track[idx].x == n.x && m_Track[idx].y == n.y)
            {
                m_Track.erase(m_Track.begin()+idx);
            }
            // if found note is row-occupying-object (ex: SHOCK), erase it
            else if (m_Track[idx].nType == NoteType::NOTE_SHOCK ||
                     m_Track[idx].nType == NoteType::NOTE_REST)
            {
                m_Track.erase(m_Track.begin()+idx);
            }
            else
            {
                idx ++;
            }
        }
        // iterate from here to first to check LN
        int idx_copy = idx;
        while (idx_copy >= 0)
        {
            if (m_Track[idx_copy].subType == NoteTapType::TAPNOTE_CHARGE ||
                m_Track[idx_copy].subType == NoteTapType::TAPNOTE_FREE)
                {
                    // check for range for ranged-note
                    // (assume only 1 longnote can be overlapped,
                    //  to prevent performance decreasing by all-range-search)
                    if (m_Track[idx_copy].x == n.x &&
                        m_Track[idx_copy].y == n.y &&
                        m_Track[idx_copy].iRow + m_Track[idx_copy].iDuration > n.iRow)
                    {
                        m_Track.erase(m_Track.begin()+idx_copy);
                        break;
                    }
                }
            --idx_copy;
        }
    }
    m_Track.insert(m_Track.begin()+idx, n);
}

void NoteData::TrackMapping(int *trackmap, int s, int e)
{
    // only map for TAPNOTE
    for (auto &n: m_Track)
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
	float curr_LN_length[10];   // LN length in beat
	for (int i=0; i<10; i++) curr_LN_length[i]=0;

	float curr_beat = -100;		// current lane's beat
	int curr_side = 0;			// current note's lane side
	int curr_shuffle_idx = 0;	// shuffling index
	int curr_note_cnt = 0;		// note count of current beat
	for (auto &n: m_Track) {
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
			curr_LN_length[ vShuffle[curr_shuffle_idx] ] = n.fBeatLength;
		}
	}
}

void NoteData::TrackRandom(int side, int key)
{
	ASSERT(side >= 0 && key >= 0 && key < 10);
	int vShuffle[10] = {0,1,2,3,4,5,6,7,8,9};
	std::random_shuffle(vShuffle, vShuffle+key);
	TrackMapping(vShuffle, side*10, side*10+key);
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
	TrackMapping(vShuffle, side*10, side*10+key);
}

void NoteData::TrackMirror(int side, int key)
{
	ASSERT(side >= 0 && key >= 0 && key < 10);
	int vTrackmap[10];
	for (int i=0; i<key; i++)
	{
		vTrackmap[i] = key-i-1;
	}
	TrackMapping(vTrackmap, side*10, side*10+key);
}

void NoteData::TrackAllSC(int side)
{
	float curr_beat = -100;		// current beat (to detect beat changing)
	int curr_sc = 0;			// current row has SC?
	std::vector<Note*> vNotes_sc_candidate;
	for (auto &n: m_Track)
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
	TrackMapping(vTrackmap, 0, 20);
}

std::string const NoteData::toString()
{
    std::stringstream ss;
    ss << "Total Note count: " << m_Track.size() << std::endl;
    ss << "Last note Information\n" << m_Track.back().toString();
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




// ------ class NoteSelection ------

void NoteSelection::SelectNote(Note* n)
{
	auto it = std::lower_bound(m_vNotes.begin(), m_vNotes.end(), n, nptr_less_row());
	m_vNotes.insert(it, n);
}
void NoteSelection::UnSelectNote(const Note* n)
{
	for (int i = 0; i < m_vNotes.size(); i++)
	{
		if (*m_vNotes[i] == *n)
		{
			m_vNotes.erase(i + m_vNotes.begin());
			break;
		}
	}
}
std::vector<Note*>& NoteSelection::GetSelection()
{
	return m_vNotes;
}
std::vector<Note*>::iterator NoteSelection::begin()
{
	return m_vNotes.begin();
}
std::vector<Note*>::iterator NoteSelection::end()
{
	return m_vNotes.end();
}
void NoteSelection::Clear()
{
	m_vNotes.clear();
}

} /* namespace rparser */