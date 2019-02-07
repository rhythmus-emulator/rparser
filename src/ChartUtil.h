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

}