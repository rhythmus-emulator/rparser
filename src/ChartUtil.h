#pragma once

#include "Chart.h"
#include "Resource.h"

namespace rparser
{



/*
 * @description
 * generated mixing object from Chart class.
 */
struct MixingData
{
	// sorted in time
	std::vector<MixingNote> vMixingNotes;

	int iNoteCount;
	int iTrackCount;
	float fLastNoteTime_ms;    // msec

								// general bpm
								// (from metadata or bpm channel)
								// (bpm channel may overwrite metadata bpm info)
	int iBPM;
	// MAX BPM
	int iMaxBPM;
	// MIN BPM
	int iMinBPM;
	// is bpm changes? (maxbpm != minbpm)
	bool isBPMChanges;
	// is backspin object exists? (bms specific attr.)
	bool isBSS;
	// is charge note exists? (bms type)
	bool isCN_bms;
	// is charge note exists?
	bool isCN;
	// is hellcharge note exists?
	bool isHCN;
	// is Invisible note exists?
	bool isInvisible;
	// is fake note exists?
	bool isFake;
	// is bomb/shock object exists?
	bool isBomb;
	// is warp object exists? (stepmania specific attr.)
	bool isWarp;
	// is stop object exists?
	bool isStop;
	// is command exists/processed? (bms specific attr.)
	bool isCommand;
};

static bool GenerateMixingDataFromChart(const Chart& c, MixingData& md);
static bool SerializeChart(const Chart& c, char **out, int &iLen);
static bool SerializeChart(const Chart& c, Resource::BinaryData &res);




class NoteSelection
{
private:
	// COMMENT: this vector should be in always sorted state
	std::vector<Note*> m_vNotes;
	friend class ChartData;      // only chartdata class can directly access to this object
public:
	void SelectNote(Note* n);
	void UnSelectNote(const Note* n);
	std::vector<Note*>& GetSelection();
	std::vector<Note*>::iterator begin();
	std::vector<Note*>::iterator end();
	void Clear();
};

// insert / delete / update
void RemoveNotes(rowid iStartRow, rowid iEndRow, bool bInclusive);
void RemoveNotes(const NoteSelection& vNotes);
void CopyRange(rowid rowFromBegin, rowid rowFromLength, rowid rowToBegin, bool overwrite = true);
void CopyRange(const NoteSelection& vNotes, rowid rowToBegin, bool overwrite = true);
void MoveRange(rowid rowFromBegin, rowid rowFromLength, rowid rowToBegin);
void MoveRange(const NoteSelection& vNotes, rowid rowToBegin);
void ShiftLane(rowid rowFromBegin, rowid rowFromLength, unsigned int newLane);
void ShiftLane(const NoteSelection& vNotes, unsigned int newLane);
void ShiftType(rowid rowFromBegin, rowid rowFromLength, TRACKTYPE newType, unsigned int newSubType);
void ShiftType(const NoteSelection& vNotes, TRACKTYPE newType, unsigned int newSubType);
void InsertBlank(rowid rowFromBegin, rowid rowFromLength);
void AddNote(const Note& n, bool overwrite = true);
void AddTapNote(rowid iRow, unsigned int lane);
void AddLongNote(rowid iRow, unsigned int lane, rowid iLength);


void AddTimingObject(const TimingObject& tobj);
void RemoveTimingObject(rowid start, rowid end);
void AddAction(const TimingObject& tobj);
void RemoveAction(rowid start, rowid end);


// modification(option) utilities
void LaneMapping(int *trackmap, int s, int e);
// @description useful for iidx(DP) style
void LaneRandom(int side, int key);
void LaneSRandom(int side, int key, bool bHrandom = false);
void LaneRRandom(int side, int key);
void LaneMirror(int side, int key);
void LaneAllSC(int side);
void LaneFlip();


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