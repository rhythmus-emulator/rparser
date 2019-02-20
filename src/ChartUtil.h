#pragma once

#include "Chart.h"
#include "Resource.h"

namespace rparser
{

class NoteSelection
{
private:
	std::vector<Note*> m_vNotes;
	double beat_start;
	double beat_end;
public:
	void SelectNote(Note* n);
	void UnSelectNote(const Note* n);
	std::vector<Note*>& GetSelection();
	std::vector<Note*>::iterator begin();
	std::vector<Note*>::iterator end();
	void Clear();
};

// insert / delete / update
void RemoveNotes(RowPos iStartRow, RowPos iEndRow, bool bInclusive);
void RemoveNotes(const NoteSelection& vNotes);
void CopyRange(RowPos rowFromBegin, RowPos rowFromLength, RowPos rowToBegin, bool overwrite = true);
void CopyRange(const NoteSelection& vNotes, RowPos rowToBegin, bool overwrite = true);
void MoveRange(RowPos rowFromBegin, RowPos rowFromLength, RowPos rowToBegin);
void MoveRange(const NoteSelection& vNotes, RowPos rowToBegin);
void ShiftLane(RowPos rowFromBegin, RowPos rowFromLength, unsigned int newLane);
void ShiftLane(const NoteSelection& vNotes, unsigned int newLane);
void ShiftType(RowPos rowFromBegin, RowPos rowFromLength, TRACKTYPE newType, unsigned int newSubType);
void ShiftType(const NoteSelection& vNotes, TRACKTYPE newType, unsigned int newSubType);
void InsertBlank(RowPos rowFromBegin, RowPos rowFromLength);
void AddNote(const Note& n, bool overwrite = true);
void AddTapNote(RowPos iRow, unsigned int lane);
void AddLongNote(RowPos iRow, unsigned int lane, RowPos iLength);


// modification(option) utilities
void LaneMapping(int *trackmap, int s, int e);
// @description useful for iidx(DP) style
void LaneRandom(NoteSelection& nsel, int side, int key);
void LaneSRandom(int side, int key, bool bHrandom = false);
void LaneRRandom(int side, int key);
void LaneMirror(int side, int key);
void LaneAllSC(int side);
void LaneFlip();
// fix impossible note to correct one after chart effector
void FixImpossibleNote();

void GetNotesWithType(NoteSelection &vNotes, int nType = -1, int subType = -1);
Note* GetNoteAtRow(int row, int track = -1);
void GetNotesAtRow(NoteSelection &vNotes, int row = -1, int track = -1);
void GetNotesAtRange(NoteSelection &vNotes, int row = -1, int track = -1);

// row scanning utilities (fast-scan)
bool IsRangeEmpty(unsigned long long startrow, unsigned long long endrow);
bool IsRowEmpty(unsigned long long row, TRACKTYPE type = TRACKTYPE::TRACK_EMPTY, unsigned int subtype = 0);
bool IsTrackEmpty(trackinfo track);
bool IsHoldNoteAtRow(int row, int track = -1);

// full scan utilities
bool IsTypeEmpty(TRACKTYPE type, unsigned int subtype = 0);
bool IsLaneEmpty(unsigned int lane);


}