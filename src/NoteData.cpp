#include "NoteData.h"
#include "util.h"
#include <algorithm>

void NoteData::SearchAllNotes(std::vector<trackiter>& notelist);
void NoteData::SearchAllNotes(std::vector<trackiter>& notelist, int iStartRow, int iEndRow, bool bInclusive);

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

void NoteData::AddNote(const Note& n, bool checkTrackDuplication)
{
    // COMMENT: note duplication won't be checked when loading file.
    // COMMENT: checkTrackDuplication does these work -
    // - if LN, then remove all iRow object in iDuration in same x.
    // - if TapNote/BGM, then check iRow/x/y.

    // search is same type of note exists in same row
    int idx = vNotes.begin() - vNotes.lower_bound(n);
    if (checkTrackDuplication)
    {
        // if found note occupies same type & row & position, then erase it.
        while (idx < vNotes.size() && vNotes[idx].iRow == n.iRow)
        {
            if (vNotes[idx].x == n.x &&
                vNotes[idx].y == n.y &&
                it->nType == n.nType && it->iRow == n.iRow)
            {
                vNotes.erase(vNotes.begin()+idx);
            }
            else
            {
                idx ++;
            }
        }
        if (it != vNotes.end() && it->nType == n.nType && it->iRow == n.iRow)
            vNotes.erase(it);
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

std::string const NoteData::toString()
{
    return "";
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
}

NoteData::~NoteData()
{

}