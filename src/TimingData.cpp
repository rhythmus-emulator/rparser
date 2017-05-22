#include "TimingData.h"

namespace rparser {

TimingData::TimingData() {
    AddSegments( BpmSegment(0, RPARSER_DEFAULT_BPM) );
    AddSegments( StopSegment(0) );
    AddSegments( ScrollSegment(0) );
}

BpmObject* TimingData::GetNextBpmObject(int iStartRow)
{
    m_TimingObjs[TYPE_TIMINGOBJ::TYPE_BPM]
    return 0;
}

StopObject* TimingData::GetNextStopObject(int iStartRow)
{
    return 0;
}

WarpObject* TimingData::GetNextWarpObject(int iStartRow)
{
    return 0;
}

MeasureObject* TimingData::GetNextMeasureObject(int iStartRow)
{
    return 0;
}

void TimingData::GetBpm()
{
    // first part BPM, mainly used for metadata
    return 0;
}

// measure related
void TimingData::SetBarLengthAtRow(int row, float length=4.0f)
{
    return 0;
}

float TimingData::GetBarLengthAtRow(int row)
{
    return 0;
}

float TimingData::GetBarBeat(int barnumber)
{
    return 0;
}

// functions using sequential objects
float TimingData::GetBeatFromTime(float time)
{
    return 0;
}

float TimingData::GetTimeFromBeat(float beat)
{
    return 0;
}

void TimingData::GetBeatMeasureFromRow(unsigned long iNoteRow, unsigned long &iBeatIndexOut, unsigned long &iMeasureIndexOut)
{
	iMeasureIndexOut = 0;
	const vector<TimingSegment *> &tSigs = GetTimingSegments(SEGMENT_TIME_SIG);
	for (unsigned i = 0; i < tSigs.size(); i++)
	{
		TimeSignatureSegment *curSig = ToTimeSignature(tSigs[i]);
		int iSegmentEndRow = (i + 1 == tSigs.size()) ? std::numeric_limits<int>::max() : curSig->GetRow();

		int iRowsPerMeasureThisSegment = curSig->GetNoteRowsPerMeasure();

		if( iNoteRow >= curSig->GetRow() )
		{
			// iNoteRow lands in this segment
			int iNumRowsThisSegment = iNoteRow - curSig->GetRow();
			int iNumMeasuresThisSegment = (iNumRowsThisSegment) / iRowsPerMeasureThisSegment;	// don't round up
			iMeasureIndexOut += iNumMeasuresThisSegment;
			iBeatIndexOut = iNumRowsThisSegment / iRowsPerMeasureThisSegment;
			iRowsRemainder = iNumRowsThisSegment % iRowsPerMeasureThisSegment;
			return;
		}
		else
		{
			// iNoteRow lands after this segment
			int iNumRowsThisSegment = iSegmentEndRow - curSig->GetRow();
			int iNumMeasuresThisSegment = (iNumRowsThisSegment + iRowsPerMeasureThisSegment - 1)
				/ iRowsPerMeasureThisSegment;	// round up
			iMeasureIndexOut += iNumMeasuresThisSegment;
		}
	}
    ASSERT("Unexpected error from TimingData::GEtBarMeasureFromRow");
}

int TimingData::GetNextMeasureFromTime(float timeoffset)
{
    return 0;
}

// @description
// MUST be called before we use Sequential objects & midi events.
// (use after all BPM/STOP/WARP/MEASURE objects are loaded)
void TimingData::SortSegments(TYPE_TIMINGOBJ type)
{
    std::vector<TimingObject*> &vobj = GetTimingSegments(type);
    std::sort(vobj.begin(), vobj.end());
}

void TimingData::SortAllSegments() {
    SortSegments(TYPE_TIMINGOBJ::TYPE_BPM);
    SortSegments(TYPE_TIMINGOBJ::TYPE_STOP);
    SortSegments(TYPE_TIMINGOBJ::TYPE_WARP);
    SortSegments(TYPE_TIMINGOBJ::TYPE_SCROLL);
    SortSegments(TYPE_TIMINGOBJ::TYPE_MEASURE);
}

std::vector<TimingObject *>& TimingData::GetTimingSegments(TYPE_TIMINGOBJ type)
{
    return m_TimingObjs[type];
}

const std::vector<TimingObject *>& TimingData::GetTimingSegments(TYPE_TIMINGOBJ type)
{
    return m_TimingObjs[type];
}

bool TimingData::HasScrollChange()
{
    return false;
}

bool TimingData::HasBPMChange()
{
    return false;
}

bool TimingData::HasSTOP()
{
    return false;
}

bool TimingData::HasWrap()
{
    return false;
}


void TimingData::AddSegments(BpmObject* obj)
{
}

void TimingData::AddSegments(StopObject* obj)
{
}

void TimingData::AddSegments(WrapObject* obj)
{
}

void TimingData::AddSegments(ScrollObject* obj)
{
}

void TimingData::AddSegments(MeasureObject* obj)
{
}


}