#include "NoteData.h"
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
        curr_row = vNotes[i].row;
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
    // TODO: check note duplication (longnote / normalnote)
    // COMMENT: note duplication won't be checked when loading file.
    // COMMENT: checkTrackDuplication does these work -
    // - if LN, then remove all iRow object in iDuration in same x.
    // - if TapNote/BGM, then check iRow/x/y.
}

std::string const NoteData::toString()
{
    return "";
}

void NoteData::ApplyResolutionRatio(float fRatio)
{
    // TODO: apply ratio
}

void NoteData::UpdateBeatData(int iRes)
{
    // calculate note's beat data from resolution
    for (auto &n: m_Track)
    {
        n.fBeat = n.iRow / (float)iRes;
    }
}
