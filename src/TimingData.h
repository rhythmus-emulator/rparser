/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_TIMINGDATA_H
#define RPARSER_TIMINGDATA_H

namespace rparser {

class TimingObject {
public:
private:
    int m_iRow;
};

class BpmObject: public TimingObject {
public:
private:
};

class StopObject: public TimingObject {
public:
private:
};

class WarpObject: public TimingObject {
public:
private:
};

class ScrollObject: public TimingObject {
public:
private:
};

class MeasureObject: public TimingObject {
public:
private:
};

// @description contains integrated lookup information for Beat-Time conversion
class LookupObject: public TimingObject {
public:
private:
}

// BPM, STOP, WARP, MEASURE
#define NUM_TIMINGOBJ_TYPE 5
enum class TYPE_TIMINGOBJ {
    TYPE_BPM,
    TYPE_STOP,
    TYPE_WARP,
    TYPE_SCROLL,
    TYPE_MEASURE
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
    bool HasBPMChange();
    bool HasSTOP();
    bool HasWarp();
private:
    // @description timing objects per each types
    std::vector<TimingObject *> m_TimingObjs[NUM_TIMINGOBJECT_TYPE];
    // @description
    // compiled timing objects(gathered all), sequenced in line.
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