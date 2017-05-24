/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_TIMINGDATA_H
#define RPARSER_TIMINGDATA_H

#include <string>

namespace rparser {

class TimingObject {
public:
    virtual TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_INVALID; }
    virtual std::string toString();
    int GetRow const { return m_iRow; }
    int SetRow(int iRow) { m_iRow = iRow; }

    bool operator<( const TimingObject &other ) const
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

    StopObject(int iRow, double dStopMSec)
        : TimingObject(iRow), m_dStopMSec(dStopMSec) {}
private:
    double m_dStopMSec;
};

class WarpObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_WARP; }
    std::string toString();

    void SetValue(int iWarpRows);
    int GetValue();

    WarpObject(int iRow, int iWarpRows)
        : TimingObject(iRow), m_iWarpRows(iWarpRows) {}
private:
    int m_iWarpRows;
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

class MeasureObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() { return TYPE_TIMINGOBJ::TYPE_MEASURE; }
    std::string toString();

    void SetValue(double dMeasure=1.0);
    double GetValue();

    MeasureObject(int iRow, double dMeasure=1.0)
        : TimingObject(iRow),  m_dMeasure(dMeasure) {}
private:
    double m_dMeasure;
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
class LookupObject: public TimingObject {
public:
private:
}

// BPM, STOP, WARP, MEASURE
#define NUM_TIMINGOBJ_TYPE 6
enum class TYPE_TIMINGOBJ {
    TYPE_BPM,
    TYPE_STOP,
    TYPE_WARP,
    TYPE_SCROLL,
    TYPE_MEASURE,
    TYPE_TICK,
    TYPE_INVALID
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
    void GetBpm();

    // measure related
    void SetBarLengthAtRow(int row, float length=4.0f);
    float GetBarLengthAtRow(int row);
    float GetBarBeat(int barnumber);

    // functions using lookup objects
    void PrepareLookup(LOOKUP_TYPE lookup_type);
    float LookupBeatFromMSec(float msec);
    float LookupMSecFromBeat(float beat);
    void GetBeatMeasureFromRow(unsigned long row, unsigned long &beatidx, unsigned long &beat);
    int GetNextMeasureFromMSec(float msec);

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
private:
    // @description timing objects per each types
    std::vector<TimingObject *> m_TimingObjs[NUM_TIMINGOBJECT_TYPE];
    // @description
    // compiled timing objects(gathered all), sequenced in line.
    // Timing objects: Bpm, Stop, Warp
    // need to call UpdateSequentialObjs(); to use this array.
    std::vector<LookupObject *> m_LookupObjs;
    // @description
    // bar resolution of current song
    int iRes;
    // @description
    // start timing of the song
    float fBeat0MSecOffset;

    // utility functions
    void AddSegments(BpmObject* obj);
    void AddSegments(StopObject* obj);
    void AddSegments(WarpObject* obj);
    void AddSegments(ScrollObject* obj);
    void AddSegments(MeasureObject* obj);
    BpmObject* ToBpm(TimingObject* obj);
    StopObject* ToStop(TimingObject* obj);
    WarpObject* ToWarp(TimingObject* obj);
    ScrollObject* ToScroll(TimingObject* obj);
    MeasureObject* ToMeasure(TimingObject* obj);
};

}

#endif