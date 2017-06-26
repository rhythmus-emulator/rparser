/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_TIMINGDATA_H
#define RPARSER_TIMINGDATA_H

#include <string>
#include <map>
#include <vector>

namespace rparser {

class NoteData;
class MetaData;

/*
 * COMMENT:
 * Some metadata in bms (BPM, STP) wants to be stored as metadata,
 * rather than storing as note object. should care about that.
 */

// COMMENT
// fBeat value can be evaluated from iRow, reverse is not suggested.
// fBeat value only can be set directly when file loading.

class TimingObject {
private:
    int m_iRow;         // for editing
    float m_fBeat;      // for playing (to prevent timing mismatch caused by resolution)
    bool m_bSpecial;    // special object? (like BMS object)
public:
    virtual TYPE_TIMINGOBJ GetType() const { return TYPE_TIMINGOBJ::TYPE_INVALID; }
    virtual std::string toString() const;
    int GetRow() const { return m_iRow; }
    int SetRow(int iRow) { m_iRow = iRow; }
    int GetBeat() const { return m_fBeat; }
    int SetBeat(int fBeat) { m_fBeat = fBeat; }
    bool GetIsSpecial() const { return m_bSpecial; }
    void SetIsSpecial(bool bSpecial) { m_bSpecial = bSpecial; }
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

    TimingObject(int iRow, float fBeat) : m_iRow(iRow), m_fBeat(fBeat) {}
};

class BpmObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() const { return TYPE_TIMINGOBJ::TYPE_BPM; }
    std::string toString() const;

    void SetValue(float dBpm);
    float GetValue() const;
	float GetBPS() const;

    BpmObject(int iRow, float fBeat, float dBpm)
        : TimingObject(iRow, fBeat), m_dBpm(dBpm) {}
private:
    float m_dBpm;
};

class StopObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() const { return TYPE_TIMINGOBJ::TYPE_STOP; }
    std::string toString() const;

    void SetValue(float dStopMSec);
    float GetValue() const;
    void SetDelay(bool bDelay);
    bool GetDelay() const;

    StopObject(int iRow, float fBeat, float dStopMSec)
        : TimingObject(iRow, fBeat), m_dStopMSec(dStopMSec), m_bDelay(false) {}
private:
    float m_dStopMSec;
    // @description check is current style is delay.
    // if it is, then pump-style STOP judgement accepted
    //   (timing at the end of the STOP)
    // else, the timing of the row is at the beginning of the STOP.
    bool m_bDelay;
};

class WarpObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() const { return TYPE_TIMINGOBJ::TYPE_WARP; }
    std::string toString() const;

    void SetValue(int iWarpLength);
    int GetValue() const;
    void SetLength(int iLength);
    int GetLength() const;
    void SetBeatLength(float fLength);
    float GetBeatLength() const;

    WarpObject(int iRow, float fBeat, int iLength, bool fLength)
        : TimingObject(iRow, fBeat), m_iLength(iLength), m_fLength(fLength) {}
private:
    int m_iLength;      // warp length in row
    float m_fLength;    // warp length in beat
};

// TODO: currently unusable
class ScrollObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() const { return TYPE_TIMINGOBJ::TYPE_SCROLL; }
    std::string toString() const;
	float GetValue() const;
	void SetValue(float v);
    ScrollObject(int iRow, float fBeat)
        : TimingObject(iRow, fBeat), m_Scroll(1) {}
private:
	float m_Scroll;
};

// default measure signature length: 4/4 -> 1.0
// measure object compares object with Measure_idx, not Row_idx
class MeasureObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() const { return TYPE_TIMINGOBJ::TYPE_MEASURE; }
    std::string toString() const;

    void SetLength(float fLength=1.0);
    float GetLength() const;
    void SetMeasure(int iMeasure);
    int GetMeasure() const;

    bool IsPositionSame(const MeasureObject &other) const;
    bool operator<( const TimingObject &other ) const;
    bool operator==( const TimingObject &other ) const;
    bool operator!=( const TimingObject &other ) const;

    MeasureObject(int iMeasure, float fLength=1.0)
        : TimingObject(0, 0), m_fLength(fLength) {}
private:
    int m_iMeasure;         // measure number
    float m_fLength;        // measure length (in beat)
};

class TickObject: public TimingObject {
public:
    TYPE_TIMINGOBJ GetType() const { return TYPE_TIMINGOBJ::TYPE_TICK; }
    std::string toString();
    static const unsigned long DEFAULT_TICK_COUNT = 4;

    unsigned long GetValue() const;
    void SetValue(unsigned long iTick=4);

    TickObject(int iRow, float fBeat, unsigned long iTick=4)
        : TimingObject(iRow, fBeat), m_iTick(iTick) {}
    
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
};

// BPM, STOP, WARP, MEASURE
#define NUM_TIMINGOBJECT_TYPE 6
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
class ChartSummaryData;

/*
 * @description
 * contains special channel objects, like BPM/STOP.
 * and calculates real-time position by these information.
 * Also contains MIDI instrument specific effects (time-sequential, not-sounding special objects).
 */
class TimingData {
public:
    TimingData();

    // measure related
    void GetBeatMeasureFromRow(unsigned long row, unsigned long &beatidx, unsigned long &beat);
    float GetBeatFromMeasure(float fMeasure);
    float GetMeasureFromBeat(float fBeat);

    // search without lookup
    BpmObject* GetNextBpmObject(int iStartRow);
    StopObject* GetNextStopObject(int iStartRow);
    WarpObject* GetNextWarpObject(int iStartRow);
    MeasureObject* GetNextMeasureObject(int iStartRow);
    TimingObject* GetNextObject(TYPE_TIMINGOBJ iType, int iStartRow);
    TimingObject* GetObjectAtRow(TYPE_TIMINGOBJ iType, int iRow);
    int GetObjectIndexAtRow( TYPE_TIMINGOBJ iType, int iRow );          // returns -1 if obj not exists

    // search using lookup objects
    void PrepareLookup();
    void ClearLookup();
    LookupObject* const FindLookupObject(std::map<float, LookupObject*> const& sorted_objs, float v) const;
    float LookupBeatFromMSec(float msec) const;
    float LookupMSecFromBeat(float beat) const;

    // functions related in editing
    void DeleteRows(int iStartRow, int iRowsToDelete);
    void Clear();

    // @description
    // Sort objs
    void SortObjects(TYPE_TIMINGOBJ type);
    void SortAllObjects();
    std::vector<TimingObject *>& GetTimingObjects(TYPE_TIMINGOBJ type);
    const std::vector<TimingObject *>& GetTimingObjects(TYPE_TIMINGOBJ type) const;

    // @description
    // kind of metadata
    bool HasScrollChange() const;
    bool HasBpmChange() const;
    bool HasStop() const;
    bool HasWarp() const;
    float GetBpm() const;
    float GetMaxBpm() const;
    float GetMinBpm() const;

    void FillSummaryData(ChartSummaryData &csd) const;
	std::string toString() const;

    // don't call these methods directly
    void SetResolution(int iRes);
    int GetResolution();
    void UpdateBeatData();

    TimingData();
    ~TimingData();
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
    int m_iRes;
    // @description
    // start timing of the song
    float m_fBeat0MSecOffset;

    // utility functions
    void AddObject(const BpmObject& obj);
    void AddObject(const StopObject& obj);
    void AddObject(const WarpObject& obj);
    void AddObject(const ScrollObject& obj);
    void AddObject(const MeasureObject& obj);
	void AddObject(const TickObject& obj);
    // @description
    // automatically add/modify/sort object.
    void AddObject(TimingObject *obj);
    BpmObject* ToBpm(TimingObject* obj);
    StopObject* ToStop(TimingObject* obj);
    WarpObject* ToWarp(TimingObject* obj);
    ScrollObject* ToScroll(TimingObject* obj);
    MeasureObject* ToMeasure(TimingObject* obj);
    const BpmObject* ToBpm(TimingObject* obj) const;
    const StopObject* ToStop(TimingObject* obj) const;
    const WarpObject* ToWarp(TimingObject* obj) const;
    const ScrollObject* ToScroll(TimingObject* obj) const;
    const MeasureObject* ToMeasure(TimingObject* obj) const;

    // @description
    // loads special timing data from note/metadata object
    // (mostly for BMS object)
    friend class Chart;
    void ClearExternObject();
    void LoadExternObject(const MetaData &metadata, const NoteData &notedata);
};

}

#endif
