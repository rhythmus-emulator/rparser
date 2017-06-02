#include "NoteData.h"
#include <algorithm>

void NoteData::SearchAllNotes(std::vector<trackiter>& notelist);
void NoteData::SearchAllNotes(std::vector<trackiter>& notelist, int iStartRow, int iEndRow, bool bInclusive);

struct notedata_with_rows
{
	int row;
	Note* n;
}

// @description fill all note's timing data from row(beat) data.
void NoteData::FillTimingData(const TimingData& td)
{
	// prepare lookup
    td.PrepareLookup();

	// arrange all note for iteration
	std::vector<notedata_with_rows> vNotes;
	for (int i=0; i<GetTrackCount(); ++i)
	{
		for (auto iter=begin(i); iter!=end(i); ++iter)
		{
			vNotes.push_back({iter->first, &iter->second});
		}
	}
	std::sort(vNotes::begin(), vNotes::end());
    
	// now lets track all note for time calculation
	int curr_row= -1;
	double curr_row_second= -1.0;
	Note* curr_note;
	for(int i=0; i<vNotes.size(); ++i)
	{
		curr_row = vNotes[i].row;
		curr_note = vNotes[i].n;
		// time calculation for current row
		if(curr_note.Row() != curr_row)
		{
			curr_row= curr_note.Row();
			curr_row_second= timing_data->GetElapsedTimeFromBeat(NoteRowToBeat(curr_row));
			++curr_row_id;
		}
		if(curr_note->type != TapNoteType_Empty)
		{
			curr_note->occurs_at_second= curr_row_second;
			curr_note->id_in_chart= static_cast<float>(curr_note_id);
			// in case of hold note, calculate end time too
			if(curr_note->type == TapNoteType_HoldHead)
			{
				curr_note->end_second= timing_data->GetElapsedTimeFromBeat(NoteRowToBeat(curr_row + curr_note->iDuration));
			}
		}
	}

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

void NoteData::AddNote(const Note& n)
{
    // TODO: check note duplication in case of longnote
}

std::string const NoteData::toString()
{
    return "";
}

void NoteData::ApplyResolutionRatio(float fRatio)
{
    // TODO: apply ratio
}