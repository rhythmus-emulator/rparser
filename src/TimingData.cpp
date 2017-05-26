#include "TimingData.h"
#include <sstream>

namespace rparser {

float scale_f(float v, float s1, float e1, float s2, float e2) {
    return v * (e2-s2) / (e1-s1);
}
double scale_d(double v, double s1, double e1, double s2, double e2) {
    return v * (e2-s2) / (e1-s1);
}

/* Objects */

std::string TimingObject::toString() {
	std::stringstream ss;
	ss << "TimingObject Row: " << iRow;
	return ss.str();
}

std::string ScrollObject::toString() {
	std::stringstream ss;
	ss << "ScrollObject Row: " << iRow << ", (Unusable)";
	return ss.str();
}

void BpmObject::SetValue(double dBpm) { m_dBpm = Bpm; }
double BpmObject::GetValue() { return m_dBpm; }
std::string BpmObject::toString() {
	std::stringstream ss;
	ss << "BpmObject Row: " << iRow << ", Value: " << m_dBpm;
	return ss.str();
}

void StopObject::SetValue(double dStopMSec) { m_dStopMSec = dStopMSec; }
double StopObject::GetValue() { return m_dStopMSec; }
void StopObject::SetDelay(bool bDelay) { m_bDelay = bDelay; }
bool StopObject::GetDelay() { return m_bDelay }
std::string StopObject::toString() {
	std::stringstream ss;
	ss << "StopObject Row: " << iRow << ", Value: " << m_dMeasure;
	return ss.str();
}

void WarpObject::SetValue(int iWarpRows) { m_iWarpRows = iWarpRows; }
int WarpObject::GetValue() { return m_iWarpRows; }
std::string WarpObject::toString() {
	std::stringstream ss;
	ss << "WarpObject Row: " << iRow << ", Value: " << m_dMeasure;
	return ss.str();
}

unsigned long TickObject::GetValue() { return m_iTick; }
void TickObject::SetValue(unsigned long iTick) { m_iTick = iTick; }
std::string TickObject::toString() {
	std::stringstream ss;
	ss << "TickObject Row: " << iRow << ", Value: " << m_dMeasure;
	return ss.str();
}

void MeasureObject::SetValue(double dMeasure) { m_dMeasure = dMeasure; }
double MeasureObject::GetValue() { return m_dMeasure; }
std::string MeasureObject::toString() {
	std::stringstream ss;
	ss << "MeasureObject Row: " << iRow << ", Value: " << m_dMeasure;
	return ss.str();
}



/* TimingData */

TimingData::TimingData() {
    AddSegments( BpmSegment(0, RPARSER_DEFAULT_BPM) );
    AddSegments( ScrollSegment(0) );
    AddSegments( MeasureSegment(1.0) );
	AddSegments( TickObject(4) );
}

BpmObject* TimingData::GetNextBpmObject(int iStartRow)
{
	for( unsigned i=0; i<bpms.size(); i++ )
	{
		BPMObject *bs = ToBPM(bpms[i]);
    }
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
void TimingData::PrepareLookup(LOOKUP_TYPE type)
{
	auto bpms= GetTimingObjects(TYPE_TIMINGOBJ::TYPE_BPM);
	auto stops= GetTimingObjects(TYPE_TIMINGOBJ::TYPE_STOP);
	auto warps= GetTimingObjects(TYPE_TIMINGOBJ::TYPE_WARP);
	if(search_mode == SEARCH_NONE)
	{
		m_LookupObjs.reserve(bpms.size() + stops.size() + warps.size());
	}

	FindEventStatus status;
    // make first line
#define SEARCH_NONE_CASE \
	case SEARCH_NONE: m_line_segments.push_back(next_line); break;
#define RETURN_IF_BEAT_COMPARE \
	if(next_line.end_beat > search_time) \
	{ \
		(*search_ret)= next_line; \
		return; \
	}
#define RETURN_IF_SECOND_COMPARE \
	if(next_line.end_second > search_time) \
	{ \
		(*search_ret)= next_line; \
		return; \
	}
	// Place an initial bpm segment in negative time before the song begins.
	// Without this, if there is a stop at beat 0, arrows will not move until
	// after beat 0 passes. -Kyz
	float first_bps= ToBPM(bpms[0])->GetBPS();
	LineSegment next_line= {
		-first_bps, -m_fBeat0OffsetInSeconds-1.f, 0.f, -m_fBeat0OffsetInSeconds,
		-m_fBeat0OffsetInSeconds-1.f, -m_fBeat0OffsetInSeconds,
		first_bps, bpms[0]};
	switch(search_mode)
	{
		SEARCH_NONE_CASE;
		case SEARCH_BEAT:
			RETURN_IF_BEAT_COMPARE;
			break;
		case SEARCH_SECOND:
			RETURN_IF_SECOND_COMPARE;
			break;
		default:
			break;
	}
	next_line.set_for_next();


    // second per beat / beat per second
	float spb= 1.f / next_line.bps;
	bool finished= false;
	// Placement order:
	//   warp (greater dest. value is used)
	//   delay
	//   stop
	//   bpm
    // (from stepmania code)
	// Stop and delay segments can be placed as complete lines.
	// A warp needs to be broken into parts at every stop or delay that occurs
	// inside it.
	// When a warp occurs inside a warp, whichever has the greater destination
	// is used.
	// A bpm segment is placed when between two other segments.
	// -Kyz
	float const max_time= 16777216.f;
	TimingSegment* curr_bpm_segment= bpms[0];
	TimingSegment* curr_warp_segment= nullptr;
	while(!finished)
	{
		int event_row= std::numeric_limits<int>::max();;
		int event_type= NOT_FOUND;
		FindEvent(event_row, event_type, status, max_time, false,
			bpms, warps, stops, delays);
		if(event_type == NOT_FOUND)
		{
			next_line.end_beat= next_line.start_beat + 1.f;
			float seconds_change= next_line.start_second + spb;
			next_line.end_second= seconds_change;
			next_line.end_expand_second= next_line.start_expand_second + seconds_change;
			next_line.bps= ToBPM(curr_bpm_segment)->GetBPS();
			next_line.time_segment= curr_bpm_segment;
			switch(search_mode)
			{
				SEARCH_NONE_CASE;
				case SEARCH_BEAT:
				case SEARCH_SECOND:
					(*search_ret)= next_line;
					return;
					break;
				default:
					break;
			}
			finished= true;
			break;
		}
		if(status.is_warping)
		{
			// Don't place a line when encountering a warp inside a warp because
			// it should go straight to the end.
			if(event_type != FOUND_WARP)
			{
				next_line.end_beat= NoteRowToBeat(event_row);
				next_line.end_second= next_line.start_second;
				next_line.end_expand_second= next_line.start_expand_second;
				next_line.time_segment= curr_warp_segment;
				switch(search_mode)
				{
					SEARCH_NONE_CASE;
					case SEARCH_BEAT:
						RETURN_IF_BEAT_COMPARE;
						break;
					case SEARCH_SECOND:
						// A search for a second can't end inside a warp.
						break;
					default:
						break;
				}
				next_line.set_for_next();
			}
		}
		else
		{
			if(event_row > 0)
			{
				next_line.end_beat= NoteRowToBeat(event_row);
				float beats= next_line.end_beat - next_line.start_beat;
				float seconds= beats * spb;
				next_line.end_second= next_line.start_second + seconds;
				next_line.end_expand_second= next_line.start_expand_second + seconds;
				next_line.time_segment= curr_bpm_segment;
				switch(search_mode)
				{
					SEARCH_NONE_CASE;
					case SEARCH_BEAT:
						RETURN_IF_BEAT_COMPARE;
						break;
					case SEARCH_SECOND:
						RETURN_IF_SECOND_COMPARE;
						break;
					default:
						break;
				}
				next_line.set_for_next();
			}
		}
		switch(event_type)
		{
			case FOUND_WARP_DESTINATION:
				// Already handled in the is_warping condition above.
				status.is_warping= false;
				curr_warp_segment= nullptr;
				break;
			case FOUND_BPM_CHANGE:
				curr_bpm_segment= bpms[status.bpm];
				next_line.bps= ToBPM(curr_bpm_segment)->GetBPS();
				spb= 1.f / next_line.bps;
				++status.bpm;
				break;
			case FOUND_STOP:
				next_line.end_beat= next_line.start_beat;
				next_line.end_second= next_line.start_second +
					ToStop(stops[status.stop])->GetPause();
				next_line.end_expand_second= next_line.start_expand_second;
				next_line.time_segment= stops[status.stop];
				switch(search_mode)
				{
					SEARCH_NONE_CASE;
					case SEARCH_BEAT:
						// Stop occurs after the beat, so use this segment if the beat is
						// equal.
						if(next_line.end_beat == search_time)
						{
							(*search_ret)= next_line;
							return;
						}
						break;
					case SEARCH_SECOND:
						RETURN_IF_SECOND_COMPARE;
						break;
					default:
						break;
				}

				next_line.set_for_next();
				++status.stop;
				break;
			case FOUND_WARP:
				{
					status.is_warping= true;
					curr_warp_segment= warps[status.warp];
					WarpSegment* ws= ToWarp(warps[status.warp]);
					float warp_dest= ws->GetLength() + ws->GetBeat();
					if(warp_dest > status.warp_destination)
					{
						status.warp_destination= warp_dest;
					}
					++status.warp;
				}
				break;
			default:
				break;
		}
		status.last_row= event_row;
	}
#undef SEARCH_NONE_CASE
#undef RETURN_IF_BEAT_COMPARE
#undef RETURN_IF_SECOND_COMPARE
	ASSERT_M(search_mode == SEARCH_NONE, "PrepareLineLookup made it to the end while not in search_mode none.");
	// m_segments_by_beat and m_segments_by_second cannot be built in the
	// traversal above that builds m_line_segments because the vector
	// reallocates as it grows. -Kyz
	vector<LineSegment*>* curr_segments_by_beat= &(m_segments_by_beat[0.f]);
	vector<LineSegment*>* curr_segments_by_second= &(m_segments_by_second[-m_fBeat0OffsetInSeconds]);
	float curr_beat= 0.f;
	float curr_second= 0.f;
	for(size_t seg= 0; seg < m_line_segments.size(); ++seg)
	{
#define ADD_SEG(bors) \
		if(m_line_segments[seg].start_##bors > curr_##bors) \
		{ \
			curr_segments_by_##bors= &(m_segments_by_##bors[m_line_segments[seg].start_##bors]); \
			curr_##bors= m_line_segments[seg].start_##bors; \
		} \
		curr_segments_by_##bors->push_back(&m_line_segments[seg]);

		ADD_SEG(beat);
		ADD_SEG(second);
#undef ADD_SEG
	}
}

LookupObject* const TimingData::FindLookupObject(std::vector<float, LookupObject*> const& sorted_objs, float v)
{
	// lower_bound returns the first element not less than the given key. (same or over)
	auto liter = sorted_objs.lower_bound(v);
	// so object value can be over designated one. in that case, we have to reduce it.
	if (sorted_objs.begin() != liter && (sorted_objs.end() == liter || liter->first > v))
		--liter;
	return liter->second;
}

float TimingData::LookupBeatFromMSec(float time)
{
	LookupObject const* lobj= FindLookupObject(m_lobjs_time_sorted, time);
	// in case of warp (over end second), consider it as end of warp beat.
	if(lobj->start_msec == lobj->end_msec)
	{
		return lobj->end_beat;
	}
	return scale_f(time, lobj->start_msec, lobj->end_msec,
		lobj->start_beat, lobj->end_beat);
}

float TimingData::LookupMSecFromBeat(float beat)
{
	// time is always sequenced object, so there's no case like warp in LookupBeatFromMSec().
	LookupObject const* lobj= FindLookupObject(m_lobjs_beat_sorted, beat);
	if(lobj->start_beat == lobj->end_beat)
	{
		// if delay exists, then we should return SEGMENT_DELAY
		if(!lobj->judgetime_use_first)
		{
			return lobj->end_msec;
		}
		return lobj->start_msec;
	}
	return scale_f(beat, lobj->start_beat, lobj->end_beat,
		lobj->start_msec, lobj->end_msec);
}

void TimingData::GetBeatMeasureFromRow(unsigned long iNoteRow, unsigned long &iBeatIndexOut, unsigned long &iMeasureIndexOut)
{
   	// use MeasureObject to get measure
	iMeasureIndexOut = 0;
	const vector<TimingObject *> &tSigs = GetTimingObjects(SEGMENT_TIME_SIG);
	// all objects are sorted in descending
	for (unsigned i = 0; i < tSigs.size(); i++)
	{
		// obtain measureobject
		MeasureObject *curSig = ToMeasure(tSigs[i]);
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
    ASSERT("Unexpected error from TimingData::GetBarMeasureFromRow");
}

int TimingData::GetNextMeasureFromMSec(float timeoffset)
{
    return 0;
}

void TimingData::DeleteRows(int iStartRow, int iRowsToDelete)
{
	// iterate each time signatures
	for (int tst=0; tst<NUM_TIMINGOBJ_TYPE; tst++)
	{
		// Don't delete the indefinite segments that are still in effect
		// at the end row; rather, shift them so they start there.
		TimingObject *tsEnd = GetSegmentAtRow(iStartRow + iRowsToDelete, tst);
		if (tsEnd != nullptr && 
			iStartRow <= tsEnd->GetRow() &&
			tsEnd->GetRow() < iStartRow + iRowsToDelete)
		{
			// The iRowsToDelete will eventually be subtracted out
			printf("[TimingData::DeleteRows] Segment at row %d shifted to %d", tsEnd->GetRow(), iStartRow + iRowsToDelete);
			tsEnd->SetRow(iStartRow + iRowsToDelete);
		}

		// Now delete and shift up
		vector<TimingObject *> &objs = m_TimingObjs[tst];
		for (unsigned j = 0; j < objs.size(); j++)
		{
			TimingObject *obj = objs[j];
			// Before deleted region:
			if (obj->GetRow() < iStartRow)
				continue;
			// Inside deleted region:
			if (obj->GetRow() < iStartRow + iRowsToDelete)
			{
				objs.erase(obj.begin()+j, obj.begin()+j+1);
				--j;
				continue;
			}
			// After deleted regions:
			obj->SetRow(obj->GetRow() - iRowsToDelete);
		}
	}
}

// @description
// MUST be called before we use Sequential objects & midi events.
// (use after all BPM/STOP/WARP/MEASURE objects are loaded)
// NOTE: all object is sorted in Row-Ascending
void TimingData::SortObjects(TYPE_TIMINGOBJ type)
{
    std::vector<TimingObject*> &vobj = GetTimingObjects(type);
    std::sort(vobj.begin(), vobj.end());
}

void TimingData::SortAllObjects() {
    SortObjects(TYPE_TIMINGOBJ::TYPE_BPM);
    SortObjects(TYPE_TIMINGOBJ::TYPE_STOP);
    SortObjects(TYPE_TIMINGOBJ::TYPE_WARP);
    SortObjects(TYPE_TIMINGOBJ::TYPE_SCROLL);
    SortObjects(TYPE_TIMINGOBJ::TYPE_MEASURE);
}

std::vector<TimingObject *>& TimingData::GetTimingObjects(TYPE_TIMINGOBJ type)
{
    return m_TimingObjs[type];
}

const std::vector<TimingObject *>& TimingData::GetTimingObjects(TYPE_TIMINGOBJ type)
{
    return m_TimingObjs[type];
}

bool TimingData::HasScrollChange()
{
	const vector<TimingObject*> &tobjs = GetTimingObjects(TYPE_TIMING::TYPE_SCROLL);
	return (tobjs.size()>1 || ToScroll(tobjs[0])->GetValue() != 1);
}
bool TimingData::HasBpmChange() { return !GetTimingObjects(TYPE_TIMINGOBJ::TYPE_STOP).size() > 1; }
bool TimingData::HasStop() { return !GetTimingObjects(TYPE_TIMINGOBJ::TYPE_STOP).empty(); }
bool TimingData::HasWrap() {  { return !GetTimingObjects(TYPE_TIMINGOBJ::TYPE_WARP).empty(); }

void TimingData::AddObjects(BpmObject* obj) { GetTimingObjects(TYPE_TIMINGOBJ::TYPE_BPM).push_back(obj); }
void TimingData::AddObjects(StopObject* obj) { GetTimingObjects(TYPE_TIMINGOBJ::TYPE_STOP).push_back(obj); }
void TimingData::AddObjects(WrapObject* obj) { GetTimingObjects(TYPE_TIMINGOBJ::TYPE_WARP).push_back(obj); }
void TimingData::AddObjects(ScrollObject* obj) { GetTimingObjects(TYPE_TIMINGOBJ::TYPE_SCROLL).push_back(obj); }
void TimingData::AddObjects(MeasureObject* obj) { GetTimingObjects(TYPE_TIMINGOBJ::TYPE_MEASURE).push_back(obj); }

BpmObject* ToBpm(TimingObject* obj) { return static_cast<BpmObject*>(obj); }
StopObject* ToStop(TimingObject* obj) { return static_cast<StopObject*>(obj); }
WarpObject* ToWarp(TimingObject* obj) { return static_cast<WarpObject*>(obj); }
ScrollObject* ToScroll(TimingObject* obj) { return static_cast<ScrollObject*>(obj); }
MeasureObject* ToMeasure(TimingObject* obj) { return static_cast<MeasureObject*>(obj); }


}