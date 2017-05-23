#include "TimingData.h"

namespace rparser {

float scale_f(float v, float s1, float e1, float s2, float e2) {
    return v * (e2-s2) / (e1-s1);
}
double scale_d(double v, double s1, double e1, double s2, double e2) {
    return v * (e2-s2) / (e1-s1);
}

TimingData::TimingData() {
    AddSegments( BpmSegment(0, RPARSER_DEFAULT_BPM) );
    AddSegments( StopSegment(0) );
    AddSegments( ScrollSegment(0) );
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

float TimingData::GetBeatFromMSec(float time)
{
	LineSegment const* segment= FindLineSegment(m_segments_by_second, from);
	if(segment->start_second == segment->end_second)
	{
		return segment->end_beat;
	}
	return scale_f(time, segment->start_second, segment->end_second,
		segment->start_beat, segment->end_beat);
}

float TimingData::GetMSecFromBeat(float beat)
{
	LineSegment const* segment= FindLineSegment(m_segments_by_beat, from);
	if(segment->start_beat == segment->end_beat)
	{
		if(segment->time_segment->GetType() == SEGMENT_DELAY)
		{
			return segment->end_second;
		}
		return segment->start_second;
	}
	return scale_f(beat, segment->start_beat, segment->end_beat,
		segment->start_second, segment->end_second);
}

void TimingData::GetBeatMeasureFromRow(unsigned long iNoteRow, unsigned long &iBeatIndexOut, unsigned long &iMeasureIndexOut)
{
    // TODO Time_Sig?
	iMeasureIndexOut = 0;
	const vector<TimingObject *> &tSigs = GetTimingObjects(SEGMENT_TIME_SIG);
	for (unsigned i = 0; i < tSigs.size(); i++)
	{
        // comment: we don't have timesignature now, at least
        // just make offset from beat0offset
		//TimeSignatureSegment *curSig = ToTimeSignature(tSigs[i]);
		//int iSegmentEndRow = (i + 1 == tSigs.size()) ? std::numeric_limits<int>::max() : curSig->GetRow();
		//int iRowsPerMeasureThisSegment = curSig->GetNoteRowsPerMeasure();

        int iSegmentEndRow = std::numeric_limits<int>::max();
        int iRowsPerMeasureThisSegment = iRes;

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

int TimingData::GetNextMeasureFromMSec(float timeoffset)
{
    return 0;
}

// @description
// MUST be called before we use Sequential objects & midi events.
// (use after all BPM/STOP/WARP/MEASURE objects are loaded)
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