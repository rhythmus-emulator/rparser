#pragma once

#include "Chart.h"
#include "Resource.h"

namespace rparser
{

class NoteSelection
{
private:
  // selected range
  int col_start_;
  int col_end_;
  int player_;
  // metadata referred in effector.
  const MetaData* metadata_;
  // selected notes finally
  std::vector<Note*> m_vNotes;
public:
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

// fix invalid note to correct one after chart effector
void FixInvalidNote(Chart &c, SONGTYPE songtype, bool delete_invalid_note);
namespace effector
{
  void Random(Chart &c, int player);
  void SRandom(int side, int key, bool bHrandom = false);
  void RRandom(int side, int key);
  void Mirror(int side, int key);
  void AllSC(int side);
  void Flip();
}

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