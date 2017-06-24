#include "TimingData.h"
#include "util.h"
#include <sstream>

namespace rparser {

float scale_f(float v, float s1, float e1, float s2, float e2) {
    return v * (e2-s2) / (e1-s1);
}
double scale_d(double v, double s1, double e1, double s2, double e2) {
    return v * (e2-s2) / (e1-s1);
}

struct ts_less : std::binary_function <TimingObject*, TimingObject*, bool>
{
    bool operator() (const TimingObject *x, const TimingObject *y) const
    {
        return (*x) < (*y);
    }
};

struct ts_less_beat : std::binary_function <TimingObject*, TimingObject*, bool>
{
    bool operator() (const TimingObject *x, const TimingObject *y) const
    {
        return x->GetBeat() < y->GetBeat();
    }
};

struct ts_less_row : std::binary_function <TimingObject*, TimingObject*, bool>
{
    bool operator() (const TimingObject *x, const TimingObject *y) const
    {
        return x->GetRow() < y->GetRow();
    }
};

/* Objects */

std::string TimingObject::toString() const {
    std::stringstream ss;
    ss << "TimingObject Row: " << m_iRow;
    return ss.str();
}

std::string ScrollObject::toString() const {
    std::stringstream ss;
    ss << "ScrollObject Row: " << GetRow() << ", (Unusable)";
    return ss.str();
}

void BpmObject::SetValue(float dBpm) { m_dBpm = dBpm; }
float BpmObject::GetValue() const { return m_dBpm; }
std::string BpmObject::toString() const {
    std::stringstream ss;
    ss << "BpmObject Row: " << GetRow() << ", Value: " << m_dBpm;
    return ss.str();
}

void StopObject::SetValue(float dStopMSec) { m_dStopMSec = dStopMSec; }
float StopObject::GetValue() const { return m_dStopMSec; }
void StopObject::SetDelay(bool bDelay) { m_bDelay = bDelay; }
bool StopObject::GetDelay() const { return m_bDelay; }
std::string StopObject::toString() const {
    std::stringstream ss;
    ss << "StopObject Row: " << GetRow() << ", Stop(Before): " << m_bDelay << "/Stop(After):" << m_dStopMSec;
    return ss.str();
}

void WarpObject::SetValue(int iWarpRows) { m_iLength = iWarpRows; }
int WarpObject::GetValue() const { return m_iLength; }
void WarpObject::SetLength(int iLength) { m_iLength = iLength; }
int WarpObject::GetLength() const { return m_iLength; }
void WarpObject::SetBeatLength(float fLength) { m_fLength = fLength; }
float WarpObject::GetBeatLength() const { return m_fLength; }
std::string WarpObject::toString() const {
    std::stringstream ss;
    ss << "WarpObject Row: " << GetRow() << ", Length: " << m_iLength;
    return ss.str();
}

unsigned long TickObject::GetValue() const { return m_iTick; }
void TickObject::SetValue(unsigned long iTick) { m_iTick = iTick; }
std::string TickObject::toString() {
    std::stringstream ss;
    ss << "TickObject Row: " << GetRow() << ", Value(Tick): " << m_iTick;
    return ss.str();
}

void MeasureObject::SetLength(float fLength) { m_fLength = fLength; }
float MeasureObject::GetLength() const { return m_fLength; }
void MeasureObject::SetMeasure(int iMeasure) { m_iMeasure = iMeasure; }
int MeasureObject::GetMeasure() const { return m_iMeasure; }
std::string MeasureObject::toString() const {
    std::stringstream ss;
    ss << "MeasureObject Row: " << GetRow() << ", Value: " << m_iMeasure;
    return ss.str();
}
bool MeasureObject::IsPositionSame(const MeasureObject &other) const
{ return GetMeasure() == other.GetMeasure(); }
bool MeasureObject::operator<( const TimingObject &other ) const
{ return GetMeasure() < static_cast<const MeasureObject&>(other).GetMeasure(); }
bool MeasureObject::operator==( const TimingObject &other ) const
{ return GetType() == other.GetType()
    && GetMeasure() == static_cast<const MeasureObject&>(other).GetMeasure()
    && GetLength() == static_cast<const MeasureObject&>(other).GetLength(); }
bool MeasureObject::operator!=( const TimingObject &other ) const
{ return !operator==(other); }



/* TimingData */

TimingData::TimingData() {
    AddObject( BpmSegment(0, 0, RPARSER_DEFAULT_BPM) );
    AddObject( ScrollSegment(0, 0) );
    AddObject( MeasureSegment(0, 1.0) );
    AddObject( TickObject(0, 0, 4) );
}

int TimingData::GetBpm()
{
    // first part BPM, mainly used for metadata
    return m_TimingObjs[TYPE_TIMINGOBJ::TYPE_BPM][0].GetValue();
}

BpmObject* GetNextBpmObject(int iStartRow) { return GetNextObject(TYPE_TIMINGOBJ::TYPE_BPM, iStartRow); }
StopObject* GetNextStopObject(int iStartRow) { return GetNextObject(TYPE_TIMINGOBJ::TYPE_STOP, iStartRow); }
WarpObject* GetNextWarpObject(int iStartRow) { return GetNextObject(TYPE_TIMINGOBJ::TYPE_WARP, iStartRow); }
MeasureObject* GetNextMeasureObject(int iStartRow) { return GetNextObject(TYPE_TIMINGOBJ::TYPE_MEASURE, iStartRow); }

TimingObject* TimingData::GetNextObject(TYPE_TIMINGOBJ iType, int iStartRow)
{
    std::vector<TimingObject*>& vObjs = GetTimingObjects(iType);
	auto it = vObjs.lower_bound(iStartRow, ts_less_row);
	if (it == vObjs.end()) return 0;
	else return *it;
}
TimingObject* TimingData::GetObjectAtRow(TYPE_TIMINGOBJ iType, int iRow)
{
    // returns same or less object
    std::vector<TimingObject*>& vObjs = GetTimingObjects(iType);
    if (vObjs.size() == 0) return 0;

    int min = 0, max = vObjs.size() - 1;
    int l = min, r = max;
    while( l <= r )
    {
        int m = ( l + r ) / 2;
        if( ( m == min || vObjs[m]->GetRow() <= iRow ) && ( m == max || iRow < vObjs[m + 1]->GetRow() ) )
        {
            return m;
        }
        else if( vObjs[m]->GetRow() <= iRow )
        {
            l = m + 1;
        }
        else
        {
            r = m - 1;
        }
    }
    // iRow is before the first segment of type tst
    return 0;
}

int TimingData::GetObjectIndexAtRow( TYPE_TIMINGOBJ iType, int iRow )
{
    TimingObject* t = GetObjectAtRow(iType, iRow);
    if (!t) return -1;
    else return t->GetRow();
}

float TimingData::GetBeatFromMeasure(float fMeasure)
{
	std::vector<MeasureObject*> &vObjs = 
		static_cast<std::vector<MeasureObject*>&>(GetTimingObjects(TYPE_TIMINGOBJ::TYPE_MEASURE));
	ASSERT(vObjs.size() > 0);
    int min = 0, max = vObjs.size() - 1;
    int l = min, r = max;
	int idx = 0;
    while( l <= r )
    {
        int m = ( l + r ) / 2;
        if( ( m == min || vObjs[m]->GetMeasure() <= iMeasure ) && ( m == max || iMeasure < vObjs[m + 1]->GetMeasure() ) )
        {
            idx = m;
			break;
        }
        else if( vObjs[m]->GetMeasure() <= iMeasure )
        {
            l = m + 1;
        }
        else
        {
            r = m - 1;
        }
    }
	// find beat from measure_object_index
	return vObjs[idx]->GetBeat() + vObjs[idx]->GetBeatLength() * (fMeasure - vObjs[idx]->GetMeasure())
}

float TimingData::GetMeasureFromBeat(float fBeat)
{
	// we borrow measure infomation from row
	// it may cause wrong result, but mostly it is okay (and this function wont be used)
	MeasureObject* mObj = GetObjectAtRow(TYPE_TIMINGOBJ::TYPE_MEASURE, fBeat * m_iRes);
	return mObj->GetMeasure() + (fBeat - mObj->GetBeat()) / mObj->GetBeatLength();
}

// functions using sequential objects
void TimingData::PrepareLookup()
{
    auto bpms= GetTimingObjects(TYPE_TIMINGOBJ::TYPE_BPM);
    auto stops= GetTimingObjects(TYPE_TIMINGOBJ::TYPE_STOP);
    auto warps= GetTimingObjects(TYPE_TIMINGOBJ::TYPE_WARP);

    // Place an initial bpm segment (dummy object)
    float first_bps= ToBPM(bpms[0])->GetBPS();
    LookupObject next_line = {
        -first_bps, -m_fBeat0MSecOffset-1.f,
        0.f, -m_fBeat0MSecOffset,
        first_bps};
    
    // second per beat / beat per second
    float spb= 1.f / next_line.bps;
    bool finished= false;
    // Placement order (event search order):
    //   warp (greater dest. value is used)
    //   stop
    //   bpm
    // (from stepmania code)
    float const max_time= 16777216.f;
    BpmObject* curr_bpm_segment= bpms[0];
    WarpObject* curr_warp_segment= nullptr;
    int idx_warp=0, idx_stop=0, idx_bpm=0;
    bool is_warping = false;
    while(!finished)
    {
        // search next event row (for WARP, STOP, BPM channel)
        float curr_beat= std::numeric_limits<float>::max();
        int curr_type = TYPE_TIMINGOBJ::TYPE_INVALID;
        float fidx;	// temporary value
        TimingObject *curr_obj = nullptr;
        // COMMENT:
        // we don't need to include last object into warp
        // when warp-ending position and other object's position is duplicated
        // (that's different from stepmania)
        if (idx_bpm < bpms.size() && (fidx=bpms[idx_bpm]->GetBeat()) < curr_beat)
        {
            curr_beat = fidx;
            curr_type = TYPE_TIMINGOBJ::TYPE_BPM;
            curr_obj = bpms[idx_bpm];
        }
        if (idx_stop < stops.size() && (fidx=stops[idx_stop]->GetBeat()) < curr_beat)
        {
            curr_beat = fidx;
            curr_type = TYPE_TIMINGOBJ::TYPE_STOP;
            curr_obj = stops[idx_stop];
        }
        if (idx_warp < warps.size() && (fidx=warps[idx_warp]->GetBeat()) < curr_beat)
        {
            curr_beat = fidx;
            curr_type = TYPE_TIMINGOBJ::TYPE_WARP;
            curr_obj = warps[idx_warp];
        }
		if (is_warping && pos_warp_end < curr_beat)
		{
			// go to END-OF-WARP process
            curr_beat = curr_warp_segment->GetBeat() + curr_warp_segment->GetBeatLength();
            curr_type = TYPE_TIMINGOBJ::TYPE_WARP;
			curr_obj = curr_warp_segment;
		}

        // if no object found - all events had been scanned
        // just add 1 beat for last segment
        if(!curr_obj)
        {
            next_line.end_beat= next_line.start_beat + 1.f;
            float seconds_change= next_line.start_second + spb;
            next_line.end_second= seconds_change;
            
            m_LookupObjs.push_back(next_line);
            finished= true;
            break;
        }

        if(is_warping)
        {
            switch (event_type)
            {
                case TYPE_TIMINGOBJ::TYPE_BPM:
                    // last BPM value will be used.
                    next_line.bps= ToBPM(curr_obj)->GetBPS();
                    spb= 1.f / next_line.bps;
                    break;
                case TYPE_TIMINGOBJ::TYPE_STOP:
                    // all STOP time will be appended during warping time.
                    next_line.start_stop_msec += ToStop(curr_obj)->GetValue();
                    next_line.end_msec += ToStop(curr_obj)->GetValue();
                    break;
                case TYPE_TIMINGOBJ::TYPE_WARP:
                    // END OF WARP, save LookupObj
                    // don't touch end_msec (beat time is actually zero)
                    next_line.end_beat = curr_beat;
                    m_LookupObjs.push_back(next_line);
                    next_line.start_beat = curr_beat;
                    next_line.start_msec = next_line.end_msec;
                    next_line.start_stop_msec = next_line.end_delay_msec = 0;
                    is_warping = false;
					curr_warp_segment = nullptr;
                    break;
            }
        }
        else
        {
            // if object found, then check new object has different row
            // if it is, add previous one to LookupObjs
            if (next_line.start_beat != curr_beat) {
                next_line.end_beat = curr_beat;
                next_line.end_msec+= spb*(next_line.start_beat - next_line.end_beat);
                m_LookupObjs.push_back(next_line);
                next_line.start_beat = curr_beat;
                next_line.start_msec = next_line.end_msec;
                next_line.start_stop_msec = next_line.end_delay_msec = 0;
            }

            switch(event_type)
            {
                case TYPE_TIMINGOBJ::TYPE_BPM:
                    next_line.bps= ToBPM(curr_obj)->GetBPS();
                    spb= 1.f / next_line.bps;
                    ++idx_bpm;
                    break;
                case TYPE_TIMINGOBJ::TYPE_STOP:
                    next_line.start_stop_msec += ToStop(curr_obj)->GetValue();
                    next_line.end_msec += ToStop(curr_obj)->GetValue();
                    // TODO: in case of delay, use end_delay_msec
                    ++idx_stop;
                    break;
                case TYPE_TIMINGOBJ::TYPE_WARP:
                    // WARP start
					curr_warp_segment = static_cast<WarpObject*>(curr_obj);
                    is_warping = true;
                    ++idx_warp;
                    break;
            }
        }
    }

    // build Lookup object (by start_beat, start_msec)
    m_lobjs_time_sorted.clear();
    m_lobjs_beat_sorted.clear();
    for(size_t idx= 0; idx < m_LookupObjs.size(); ++idx)
    {
        // beat won't be overlapped, but time may be overlapped.
        // (in case of WARP without STOP)
        // in that case, WARP object will be ignored.
        // (so time->beat conversion won't be sequential.)
        float curr_time = m_LookupObjs[idx].start_msec;
        float curr_beat = m_LookupObjs[idx].start_beat;
        m_lobjs_time_sorted[curr_time] = &m_LookupObjs[idx];
        m_lobjs_beat_sorted[curr_beat] = &m_LookupObjs[idx];
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
    const vector<TimingObject *> &tSigs = GetTimingObjects(TYPE_TIMINGOBJ::TYPE_MEASURE);
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
            int iNumMeasuresThisSegment = (iNumRowsThisSegment) / iRowsPerMeasureThisSegment;   // don't round up
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
                / iRowsPerMeasureThisSegment;   // round up
            iMeasureIndexOut += iNumMeasuresThisSegment;
        }
    }
    ASSERT("Unexpected error from TimingData::GetBarMeasureFromRow");
}

void TimingData::DeleteRows(int iStartRow, int iRowsToDelete)
{
    // iterate each time signatures
    for (int tst=0; tst<NUM_TIMINGOBJ_TYPE; tst++)
    {
        // measure object is iterated by 'measure', so skip here.
        if (tst == TYPE_TIMINGOBJ::TYPE_MEASURE) continue;

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
        std::vector<TimingObject *> &objs = m_TimingObjs[tst];
        for (unsigned j = 0; j < objs.size(); j++)
        {
            TimingObject *obj = objs[j];
            // Before deleted region:
            if (obj->GetRow() < iStartRow)
                continue;
            // Inside deleted region:
            if (obj->GetRow() < iStartRow + iRowsToDelete)
            {
                // TODO: delete object for memory leak prevention
                objs.erase(obj.begin()+j, obj.begin()+j+1);
                --j;
                continue;
            }
            // After deleted regions:
            obj->SetRow(obj->GetRow() - iRowsToDelete);
        }
    }
}

void TimingData::Clear()
{
    for (int tst=0; tst<NUM_TIMINGOBJ_TYPE; tst++)
    {
        std::vector<TimingObject *> &objs = m_TimingObjs[tst];
        for (unsigned j = 0; j < objs.size(); j++)
        {
            delete objs[j];
        }
        objs.clear();
    }
}

void TimingData::SetResolution(int iRes)
{
    float fRatio = (float)iRes / m_iRes;
    for (int i=0; i<NUM_TIMINGOBJ_TYPE; i++)
    {
        if (i == TYPE_TIMINGOBJ::TYPE_MEASURE)
        {
            // TYPE_MEASURE's Beat value is always correct,
            // So it's rather better to use that value.
            tobj->SetRow( tobj->GetBeat() * iRes );
            continue;
        }
        for (auto tobj: GetTimingObjects(i))
        {
            tobj->SetRow( tobj->GetRow() * fRatio );
			if (i == TYPE_TIMINGOBJ::TYPE_WARP)
			{
				WarpObject *wobj = ToWarp(tobj);
				wobj->SetBeatLength( wobj->GetLength() * fRatio );
			}
        }
    }
	m_iRes = iRes;
}

void TimingData::UpdateBeatData(int iRes)
{
    // calculate beat data from iRow
    for (int i=0; i<NUM_TIMINGOBJ_TYPE; i++)
    {
        // MEASURE channel's beat data is always correct, so don't update.
        if (tst == TYPE_TIMINGOBJ::TYPE_MEASURE) continue;

        for (auto tobj: GetTimingObjects(i))
        {
            tobj->SetRow( tobj->GetRow() / (float)iRes );
        }
    }

    // also calculate length in case of warp object
    for (auto tobj: GetTimingObjects(TYPE_TIMINGOBJ::TYPE_WARP))
    {
        WarpObject *wobj = ToWarp(tobj);
        wobj->SetBeatLength( wobj->GetLength() / (float)iRes );
    }
}

void TimingData::LoadBpmStopObject(const NoteData& nd, const MetaData& md)
{
    // note's row/beat position should be filled before calling this function.
	// start of the warp
	bool is_warping=false;
	float warp_fBeat=0;
	int warp_iRow=0;
	for (auto it=nd.begin(); it != nd.end(); ++it)
	{
		// BPM / WARP
		if (it->nType == NoteType::NOTE_BPM)
		{
			if (it->iValue > 999999)
			{
				// prepare warp-starting
				warp_iRow = it->iRow;
				warp_fBeat = it->fBeat;
				is_warping = true;
				continue;
			}
			if (is_warping)
			{
				// append new warp object
				int iDuration = it->iRow - warp_iRow;
				float fDuration = it->fBeat - warp_fBeat;
				AddObject(new WarpObject(warp_iRow, warp_fBeat, iDuration, fDuration));
				is_warping = false;
				continue;
			}
			// from here it's general BPM object ...
			float fBpm;
			if (!md.GetBPMChannel()->GetBpm(it->iValue, fBpm))
			{
				printf("[BpmObject] Value %d is not on channel value(invalid), ignored" % it->iValue);
				continue;
			}
			AddObject(new BpmObject(it->iRow, it->fBeat, fBpm));
		}
		// STOP
		else if (it->nType == NoteType::NOTE_STOP)
		{
			float fStop;
			if (!md.GetSTOPChannel()->GetStop(it->iValue, fStop))
			{
				printf("[StopObject] Value %d is not on channel value(invalid), ignored" % it->iValue);
				continue;
			}
			AddObject(new StopObject(it->iRow, it->fBeat, fStop));
		}
	}

    // STOP from metadata #STPXX
	for (auto it=md.GetSTPChannel()->begin(); it != md.GetSTPChannel()->end(); ++it)
	{
		float fBeat = GetBeatFromMeasure(it->measure);
		float fStop = it->stop;
		AddObject(new StopObject(fBeat * m_iRes, fBeat, fStop));
	}
}

// @description
// MUST be called before we use Sequential objects & midi events.
// (use after all BPM/STOP/WARP/MEASURE objects are loaded)
// NOTE: all object is sorted in Row-Ascending
void TimingData::SortObjects(TYPE_TIMINGOBJ type)
{
    std::vector<TimingObject*> &vobj = GetTimingObjects(type);
    std::sort(vobj.begin(), vobj.end(), tobj_less() );
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

void TimingData::AddObject(TimingObject *obj)
{
    std::vector<TimingObject*> vObjs = GetTimingObjects(obj->GetType());
    std::vector<TimingObject*>::iterator it;
    // get object and check its position is same
    // if same or duplicated, then remove previous one.
    int idx = GetObjectIndexAtRow(obj->GetRow());
    if (vObjs[idx]->GetRow() == obj->GetRow()) {
        if (vObjs[idx] == *obj) {
            // don't need to replace same object
            return;
        }
        delete vObjs[idx];
        vObjs.erase(vObjs.begin()+idx);
    }
    // if position not same, then insert
    else {
        // in case of warp object, we have to check length.
        if (vObj[idx]->GetType == TYPE_TIMINGOBJ::TYPE_WARP) {
            WarpObject* wObj = ToWarp(*it);
            if (wObj->GetBeat()+wObj->GetLength() >= obj->GetBeat())
            {
                delete wObj;
                vObjs.erase(vObjs.begin()+idx);
            }
        }
    }
    vObjs.insert(vObjs.begin()+idx, obj);


    // if type is MEASURE, then row position should be fully calculated again
    // to be always validated.
    if (obj->GetType() == TYPE_TIMINGOBJ::TYPE_MEASURE)
    {
        int curr_measure = ToMeasure(obj)->GetMeasure();
        double curr_beat = obj->GetBeat();
        for (it = vObj.upper_bound(obj); it != vObj.end(); ++it)
        {
            // first measure section always at zero position,
            // so we won't need to care about default value.
            MeasureObject* mObj = ToMeasure(*it);
            curr_beat += mObj->GetLength() * (mObj->GetMeasure() - curr_measure);
            mObj->SetBeat(curr_beat);
            mObj->SetRow(curr_beat * iRes);
            curr_measure = mObj->GetMeasure();
        }
    }
}

BpmObject* ToBpm(TimingObject* obj) { return static_cast<BpmObject*>(obj); }
StopObject* ToStop(TimingObject* obj) { return static_cast<StopObject*>(obj); }
WarpObject* ToWarp(TimingObject* obj) { return static_cast<WarpObject*>(obj); }
ScrollObject* ToScroll(TimingObject* obj) { return static_cast<ScrollObject*>(obj); }
MeasureObject* ToMeasure(TimingObject* obj) { return static_cast<MeasureObject*>(obj); }

TimingData::TimingData()
{
	m_iRes = DEFAULT_RESOLUTION_SIZE;
}

TimingData::~TimingData()
{
    Clear();
}

}
