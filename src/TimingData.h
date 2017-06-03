/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_TIMINGDATA_H
#define RPARSER_TIMINGDATA_H

#include <string>

namespace rparser {

/*
 * COMMENT:
 * Some metadata in bms (BPM, STP) wants to be stored as metadata,
 * rather than storing as note object. should care about that.
 */

class TimingObject {
public:
    virtual TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_INVALID; }
    virtual std::string toString();
    int GetRow const { return m_iRow; }
    int SetRow(int iRow) { m_iRow = iRow; }
    virtual bool IsPositionSame(const TimingObject &other) const
    {
        return GetRow() == other.GetRow();
    }

    virtual bool operator<( const TimingObject &other ) const
	{
		return GetRow() < other.GetRow();
	}
	// overloads should not call this base version; derived classes
	// should only compare contents, and this compares position.
	virtual bool operator==( const TimingObject &other ) const
	{
		return GetRow() == other.GetRow();
	}

	virtual bool operator!=( const TimingObject &other ) const
	{
		return !this->operator==(other);
	}

    TimingObject(int iRow) : m_iRow(iRow) {}
private:
    int m_iRow;
};

class BpmObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_BPM; }
    std::string toString();

    void SetValue(double dBpm);
    double GetValue();

    BpmObject(int iRow, double dBpm)
        : TimingObject(iRow), m_dBpm(dBpm) {}
private:
    double m_dBpm;
};

class StopObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_STOP; }
    std::string toString();

    void SetValue(double dStopMSec);
    double GetValue();
    void SetDelay(bool bDelay);
    bool GetDelay();

    StopObject(int iRow, double dStopMSec)
        : TimingObject(iRow), m_dStopMSec(dStopMSec), m_bDelay(false) {}
private:
    double m_dStopMSec;
    // @description check is current style is delay.
    // if it is, then pump-style STOP judgement accepted
    //   (timing at the end of the STOP)
    // else, the timing of the row is at the beginning of the STOP.
    bool m_bDelay;
};

class WarpObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_WARP; }
    std::string toString();

    void SetValue(int iWarpRows);
    int GetValue();
    void SetBpm(float fWarpBpm);
    float GetBpm();
    void SetIsWarpEnds(bool bWarpEnds);
    bool GetIsWarpEnds();

    WarpObject(int iRow, int iWarpRows, bool bWarpEnds=false)
        : TimingObject(iRow), m_iWarpRows(iWarpRows), m_bWarpEnds(bWarpEnds) {}
private:
    int m_iWarpRows;
    bool m_bWarpEnds;
    // @description used in format that unsupporting WARP object
    // (ex: BMS)
    float m_fWarpBpm;
};

// TODO: currently unusable
class ScrollObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_SCROLL; }
    std::string toString();
    ScrollObject(int iRow)
        : TimingObject(iRow) {}
private:
};

// default measure signature length: 4/4 -> 1.0
// measure object compares object with Measure_idx, not Row_idx
class MeasureObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_MEASURE; }
    std::string toString();

    void SetLength(float dMeasure=1.0);
    float GetLength();
    void SetMeasure(int iMeasure);
    float GetMeasure();

    bool IsPositionSame(const TimingObject &other) const;
    bool operator<( const TimingObject &other ) const;
	bool operator==( const TimingObject &other ) const;
	bool operator!=( const TimingObject &other ) const;

    MeasureObject(int iMeasure, float dMeasure=1.0)
        : TimingObject(0),  m_dMeasure(dMeasure) {}
private:
    int m_iMeasure;     // measure position
    float m_dMeasure;  // measure length
};

class TickObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_TICK; }
    std::string toString();
    static const unsigned long DEFAULT_TICK_COUNT = 4;

    unsigned long GetValue();
    void SetValue(unsigned long iTick=4);

    TickObject(int iRow, unsigned long iTick=4)
        : TimingObject(iRow), m_iTick(iTick) {}
    
private:
    // @description tick per beat
    unsigned long m_iTick;
};

// @description contains integrated lookup information for Beat-Time conversion
// we don't store measure info here; measure conversion uses only TYPE_MEASURE.
// by delay-time, the real judgement time is just after STOP
struct LookupObject {
    float start_beat;
    float start_msec;
    float end_beat;
    float end_msec;             // beat+STOP+DELAY time
    float start_stop_msec;      // STOP time
    float end_delay_msec;       // DELAY time

    float bps;                  // beat per second
}

// BPM, STOP, WARP, MEASURE
#define NUM_TIMINGOBJ_TYPE 6
enum TYPE_TIMINGOBJ {
    TYPE_BPM=0,
    TYPE_STOP=1,
    TYPE_WARP=2,
    TYPE_SCROLL=3,
    TYPE_MEASURE=4,
    TYPE_TICK=5,
    TYPE_INVALID=6
};

enum class LOOKUP_TYPE {
    LOOKUP_NONE,    // just clear lookup data
    LOOKUP_BPM,
    LOOKUP_BEAT
};

#define RPARSER_DEFAULT_BPM 120

/*
 * @description
 * contains special channel objects, like BPM/STOP.
 * and calculates real-time position by these information.
 * Also contains MIDI instrument specific effects (time-sequential, not-sounding special objects).
 */
class TimingData {
public:
    TimingData();

    BpmObject* GetNextBpmObject(int iStartRow);
    StopObject* GetNextStopObject(int iStartRow);
    WarpObject* GetNextWarpObject(int iStartRow);
    MeasureObject* GetNextMeasureObject(int iStartRow);
    TimingObject* GetObjectAtRow(int iRow, TYPE_TIMINGOBJ iType);
    void GetBpm();

    // measure related
    void SetBarLengthAtRow(int row, float length=4.0f);
    float GetBarLengthAtRow(int row);
    float GetBarBeat(int barnumber);

    // functions using lookup objects
    void PrepareLookup();
    LookupObject* const FindLookupObject(std::vector<float, LookupObject*> const& sorted_objs, float v);
    float LookupBeatFromMSec(float msec);
    float LookupMSecFromBeat(float beat);
    void GetBeatMeasureFromRow(unsigned long row, unsigned long &beatidx, unsigned long &beat);
    int GetNextMeasureFromMSec(float msec);

    // functions related in editing
    void DeleteRows(int iStartRow, int iRowsToDelete);

    // @description
    // Sort objs
    void SortObjects(TYPE_TIMINGOBJ type);
    void SortAllObjects();
    std::vector<TimingObject *>& GetTimingObjects(TYPE_TIMINGOBJ type);
    const std::vector<TimingObject *>& GetTimingObjects(TYPE_TIMINGOBJ type);

    // @description
    // kind of metadata
    bool HasScrollChange();
    bool HasBpmChange();
    bool HasStop();
    bool HasWarp();

    void SetResolution(int iRes);
    int GetResolution();
private:
    // @description timing objects per each types
    std::vector<TimingObject *> m_TimingObjs[NUM_TIMINGOBJECT_TYPE];
    // @description
    // compiled timing objects(gathered all), sequenced in line.
    // Timing objects: Bpm, Stop, Warp
    // need to call UpdateSequentialObjs(); to use this array.
    std::vector<LookupObject> m_LookupObjs;
    std::map<float, LookupObject*> m_lobjs_time_sorted; // msec time
    std::map<float, LookupObject*> m_lobjs_beat_sorted;
    // @description
    // bar resolution of current song
    int iRes;
    // @description
    // start timing of the song
    float fBeat0MSecOffset;

    // utility functions
    void AddObject(const BpmObject& obj);
    void AddObject(const StopObject& obj);
    void AddObject(const WarpObject& obj);
    void AddObject(const ScrollObject& obj);
    void AddObject(const MeasureObject& obj);
    // not should be called directly
    void AddObject(TimingObject *obj);
    BpmObject* ToBpm(TimingObject* obj);
    StopObject* ToStop(TimingObject* obj);
    WarpObject* ToWarp(TimingObject* obj);
    ScrollObject* ToScroll(TimingObject* obj);
    MeasureObject* ToMeasure(TimingObject* obj);
};

}

#endif