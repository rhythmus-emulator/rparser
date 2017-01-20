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

class MeasureObject: public TimingObject {
public:
private:
};

// @description contains integrated time-sequential information for Beat-Time conversion
class SequentialObject: public TimingObject {
public:
private:
}

// BPM, STOP, WARP, MEASURE
#define NUM_TIMINGOBJ_TYPE 4
enum TYPE_TIMINGOBJ {
    TYPE_BPM,
    TYPE_STOP,
    TYPE_WARP,
    TYPE_MEASURE
}

struct MidiEvent {
    // @description tick difference relative to previous object.
    // used for load/saving, so edit this value instead editing beat attribute.
    int tickoffset;
    // @description channel which event occurs.
    int channel;
    // @description absolute beat for rendering/playing. calculated by Invalidate() function.
    float beat;
    // @description need to be calculated by Invalidate() function.
    float time;
    // @description mostly means command.
    short status;
    short data1;
    short data2;
}

/*
 * @description
 * contains special channel objects, like BPM/STOP.
 * and calculates real-time position by these information.
 * Also contains MIDI instrument specific effects (time-sequential, not-sounding special objects).
 */
class TimingData {
public:
    BpmObject* GetNextBpmObject(int iStartRow);
    StopObject* GetNextBpmObject(int iStartRow);
    WarpObject* GetNextBpmObject(int iStartRow);
    MeasureObject* GetNextBpmObject(int iStartRow);
    void GetBpm();

    // measure related
    void SetBarLengthAtRow(int row, float length=4.0f);
    float GetBarLengthAtRow(int row);
    float GetBarBeat(int barnumber);

    // functions using sequential objects
    float GetBeatFromTime(float time);
    float GetTimeFromBeat(float beat);
    int GetNextMeasureFromTime(float timeoffset);

    // @description
    // MUST be called before we use Sequential objects & midi events.
    // (use after all BPM/STOP/WARP/MEASURE objects are loaded)
    void Invalidate();
private:
    // @description timing objects per each types
    std::vector<TimingObject *> m_TimingObjs[NUM_TIMINGOBJECT_TYPE];
    // @description
    // compiled timing objects(gathered all), sequenced in line.
    // need to call UpdateSequentialObjs(); to use this array.
    std::vector<SequentialObject *> m_SequentialObjs;
    // @description
    // contains special MIDI events.
    std::vector<MidiEvent> m_MidiEvents;
};

}

#endif