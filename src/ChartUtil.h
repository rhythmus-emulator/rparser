#ifndef RPARSER_CHARTUTIL_H
#define RPARSER_CHARTUTIL_H

#include "Chart.h"

namespace rparser
{

constexpr int kMaxSizeLane = 20;

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

void RemoveNotes(Chart &c, const NoteSelection& vNotes);
void CopyNotes(Chart &c, const NoteSelection& vNotes, double beat_delta);
void MoveNotes(Chart &c, const NoteSelection& vNotes, double beat_delta);
void ClipRange(Chart &c, double beat_start, double beat_end);
void ShiftLane(Chart &c, const NoteSelection& vNotes, unsigned int newLane);
void ShiftType(Chart &c, const NoteSelection& vNotes, int notetype, int notesubtype);
void InsertBlank(RowPos rowFromBegin, RowPos rowFromLength);
void AddNote(const Note& n, bool overwrite = true);
void AddTapNote(RowPos iRow, unsigned int lane);
void AddLongNote(RowPos iRow, unsigned int lane, RowPos iLength);

struct SortedNoteObjects
{
  std::vector<Note*> nobj_by_beat;
  std::vector<Note*> nobj_by_tempo;
  std::vector<Note*> nobj_by_row;
};
struct ConstSortedNoteObjects
{
  std::vector<const Note*> nobj_by_beat;
  std::vector<const Note*> nobj_by_tempo;
  std::vector<const Note*> nobj_by_row;
};
// XXX: restrict note type to kTempo, really?
template <typename A, typename B>
void SortNoteObjectsByType(A& notedata, B& out)
{
  for (auto& nobj : notedata)
  {
    if (nobj.GetNotePosType() == NotePosTypes::Beat)
      out.nobj_by_beat.push_back(&nobj);
    else if (nobj.GetNotePosType() == NotePosTypes::Time)
      out.nobj_by_tempo.push_back(&nobj);
    else if (nobj.GetNotePosType() == NotePosTypes::Row)
      out.nobj_by_row.push_back(&nobj);
  }
  std::sort(out.nobj_by_beat.begin(), out.nobj_by_beat.end(), [] (const Note* lhs, const Note* rhs)
  { return lhs->GetNotePos().beat < rhs->GetNotePos().beat; });
  std::sort(out.nobj_by_tempo.begin(), out.nobj_by_tempo.end(), [] (const Note* lhs, const Note* rhs)
  { return lhs->GetNotePos().beat < rhs->GetNotePos().beat; });
  std::sort(out.nobj_by_row.begin(), out.nobj_by_row.end(), [] (const Note* lhs, const Note* rhs)
  { return lhs->GetNotePos().row.measure <= rhs->GetNotePos().row.measure &&
           (lhs->GetNotePos().row.num / (double)lhs->GetNotePos().row.deno)
           < 
           (rhs->GetNotePos().row.num / (double)rhs->GetNotePos().row.deno); });
}

// fix invalid note to correct one after chart effector
void FixInvalidNote(Chart &c, SONGTYPE songtype, bool delete_invalid_note);
namespace effector
{
  enum LaneTypes
  {
    LOCKED = 0,
    NOTE,
    SC,
    PEDAL
  };

  struct EffectorParam
  {
    int player;                     // default is 0 (1P)
    int lanesize;                   // default is 7
    int lockedlane[kMaxSizeLane];   // flag for locked lanes for changing (MUST sorted); ex: SC, pedal, ...
    int sc_lane;                    // scratch lane idx (for AllSC option)
  };

  void SetLanefor7Key(EffectorParam& param);
  void SetLanefor9Key(EffectorParam& param);
  void SetLaneforBMS1P(EffectorParam& param);
  void SetLaneforBMS2P(EffectorParam& param);
  void SetLaneforBMSDP1P(EffectorParam& param);
  void SetLaneforBMSDP2P(EffectorParam& param);

  /**
   * @detail Generate random column mapping. 
   * @params
   * cols           integer array going to be written lane mapping.
   * EffectorParam  contains locked lane info and lanesize to generate random mapping.
   */
  void GenerateRandomColumn(int *cols, const EffectorParam& param);
  void Random(Chart &c, const EffectorParam& param);
  void SRandom(Chart &c, const EffectorParam& param);
  void HRandom(Chart &c, const EffectorParam& param);
  void RRandom(Chart &c, const EffectorParam& param);
  void Mirror(Chart &c, const EffectorParam& param);
  void AllSC(Chart &c, const EffectorParam& param);
  void Flip(Chart &c, const EffectorParam& param);
}

bool IsRangeEmpty(double beat_start, double beat_end);
bool IsHoldNoteAt(double beat);
bool IsHoldNoteAt(double beat, NoteTrack track);
bool IsTypeEmpty(int type, int subtype = -1);
bool IsTrackEmpty(NoteTrack track);
}

#endif