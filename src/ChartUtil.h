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

/**
 * Export chart data (metadata, notedata, events etc ...) to HTML format.
 * CSS and other necessary things should be made by oneself ...
 */
void ExportToHTML(const Chart &c, std::string& out);

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
  void RRandom(Chart &c, const EffectorParam& param, bool shift_by_timing=false);
  void Mirror(Chart &c, const EffectorParam& param);
  void AllSC(Chart &c, const EffectorParam& param);
  void Flip(Chart &c, const EffectorParam& param);
}

}

#endif