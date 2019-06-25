#include "pybind11/pybind11.h"
#include "Song.h"

namespace py = pybind11;
using namespace rparser;

PYBIND11_MODULE(rparser_py, m)
{
  py::class_<Song>(m, "Song")
    .def(py::init<>())
    .def("Open", [](Song& s, const std::string& fn) { return s.Open(fn); })
    .def("Close", [](Song &s) { s.Close(); })
    .def("Close", &Song::Close)
    .def("GetChartCount", &Song::GetChartCount)
    .def("GetPath", &Song::GetPath)
    .def("GetChart", (Chart* (Song::*)(int)) &Song::GetChart)
    .def("GetChart", (Chart* (Song::*)(const std::string&)) &Song::GetChart)
    .def("CloseChart", &Song::CloseChart)
    .def("NewChart", &Song::NewChart)
    ;

  py::class_<Chart>(m, "Chart")
    .def(py::init<>())
    .def("GetMetaData", (MetaData& (Chart::*)()) &Chart::GetMetaData)
    .def("GetNoteData", (NoteData<SoundNote>& (Chart::*)()) &Chart::GetNoteData)
    .def("GetTempoData", (TempoData& (Chart::*)()) &Chart::GetTempoData)
    .def("GetEventNoteData", (NoteData<EventNote>& (Chart::*)()) &Chart::GetEventNoteData)

    .def("GetNoteCount", &Chart::GetScoreableNoteCount)
    .def("GetLastObjectTime", &Chart::GetSongLastObjectTime)
    .def("GetLastScoreableObjectTime", &Chart::GetSongLastScorableObjectTime)
    .def("HasLongNote", &Chart::HasLongnote)
    .def("GetPlayLaneCount", &Chart::GetPlayLaneCount)
    .def("IsEmpty", &Chart::IsEmpty)
    .def("GetHash", &Chart::GetHash)
    .def("GetFilename", &Chart::GetFilename)

    .def("Invalidate", &Chart::Invalidate)
    ;

  py::class_<MetaData>(m, "MetaData")
    .def("SetUtf8Encoding", &MetaData::SetUtf8Encoding)
    .def("GetAttribute", (int (MetaData::*)(const std::string&) const) &MetaData::GetAttribute)
    .def("GetAttribute", (double (MetaData::*)(const std::string&) const) &MetaData::GetAttribute)
    //.def("GetAttribute", (std::string(MetaData::*)(const std::string&) const) &MetaData::GetAttribute)
    .def("SetAttribute", (void (MetaData::*)(const std::string&, int)) &MetaData::SetAttribute)
    .def("SetAttribute", (void (MetaData::*)(const std::string&, const std::string&)) &MetaData::SetAttribute)
    .def("SetAttribute", (void (MetaData::*)(const std::string&, double)) &MetaData::SetAttribute)
    .def("SetMetaFromAttribute", &MetaData::SetMetaFromAttribute)

    .def_readwrite("title", &MetaData::title)
    .def_readwrite("artist", &MetaData::artist)
    .def_readwrite("subtitle", &MetaData::subtitle)
    .def_readwrite("subartist", &MetaData::subartist)
    .def_readwrite("genre", &MetaData::genre)
    .def_readwrite("charttype", &MetaData::charttype)
    .def_readwrite("chartmaker", &MetaData::chartmaker)
    .def_readwrite("player_count", &MetaData::player_count)
    .def_readwrite("player_side", &MetaData::player_side)
    .def_readwrite("difficulty", &MetaData::difficulty)
    .def_readwrite("level", &MetaData::level)
    .def_readwrite("bpm", &MetaData::bpm)
    .def_readwrite("total", &MetaData::gauge_total)
    .def_readwrite("LNType", &MetaData::bms_longnote_type)
    .def_readwrite("LNOBJ", &MetaData::bms_longnote_object)
    .def_readwrite("script", &MetaData::script)

    .def("GetSoundChannel", (SoundMetaData* (MetaData::*)()) &MetaData::GetSoundChannel)
    .def("GetBGAChannel", (BgaMetaData* (MetaData::*)()) &MetaData::GetBGAChannel)
    .def("GetBPMChannel", (BmsBpmMetaData* (MetaData::*)()) &MetaData::GetBPMChannel)
    .def("GetSTOPChannel", (BmsStopMetaData* (MetaData::*)()) &MetaData::GetSTOPChannel)

    .def("toString", &MetaData::toString)
    ;

  py::class_<TempoData>(m, "TempoData")
    .def("GetTimeFromBeat", &TempoData::GetTimeFromBeat)
    .def("GetBeatFromTime", &TempoData::GetBeatFromTime)
    .def("GetBeatFromRow", &TempoData::GetBeatFromRow)
    .def("GetMeasureFromBeat", &TempoData::GetMeasureFromBeat)
    .def("GetMaxBpm", &TempoData::GetMaxBpm)
    .def("GetMinBpm", &TempoData::GetMinBpm)
    .def("HasBpmChange", &TempoData::HasBpmChange)
    .def("HasStop", &TempoData::HasStop)
    .def("HasWarp", &TempoData::HasWarp)
    .def("clear", &TempoData::clear)
    .def("toString", &TempoData::toString)
    ;

  py::class_< NoteData<SoundNote> >(m, "NoteData")
    .def("clear", &NoteData<SoundNote>::clear)
    .def("toString", &NoteData<SoundNote>::toString)
    .def("size", &NoteData<SoundNote>::size)
    .def("IsEmpty", &NoteData<SoundNote>::IsEmpty)
    .def("front", (SoundNote& (NoteData<SoundNote>::*)()) &NoteData<SoundNote>::front)
    .def("back", (SoundNote& (NoteData<SoundNote>::*)()) &NoteData<SoundNote>::back)
    .def("get", (SoundNote& (NoteData<SoundNote>::*)(size_t)) &NoteData<SoundNote>::get)
    .def("__getitem__", [](const NoteData<SoundNote>& d, size_t i) { return d.get(i); })
    ;

  py::class_< NoteData<EventNote> >(m, "EventNoteData")
    .def("clear", &NoteData<EventNote>::clear)
    .def("toString", &NoteData<EventNote>::toString)
    .def("size", &NoteData<EventNote>::size)
    .def("IsEmpty", &NoteData<EventNote>::IsEmpty)
    .def("front", (EventNote& (NoteData<EventNote>::*)()) &NoteData<EventNote>::front)
    .def("back", (EventNote& (NoteData<EventNote>::*)()) &NoteData<EventNote>::back)
    .def("get", (EventNote& (NoteData<EventNote>::*)(size_t)) &NoteData<EventNote>::get)
    .def("__getitem__", [](const NoteData<EventNote>& d, size_t i) { return d.get(i); })
    ;

  py::enum_<NotePosTypes>(m, "NotePosTypes")
    .value("NullPos", NotePosTypes::NullPos)
    .value("Time", NotePosTypes::Time)
    .value("Beat", NotePosTypes::Beat)
    .value("Bar", NotePosTypes::Bar)
    ;

  py::class_<NotePos>(m, "NotePos")
    .def(py::init<>())
    .def_readwrite("type", &NotePos::type)
    .def_readwrite("time", &NotePos::time_msec)
    .def_readwrite("beat", &NotePos::beat)
    .def_readwrite("measure", &NotePos::measure)
    .def("toString", &NotePos::toString)

    .def("SetRowPos", (void (NotePos::*)(uint32_t, RowPos, RowPos)) &NotePos::SetRowPos)
    .def("SetRowPos", (void (NotePos::*)(double)) &NotePos::SetRowPos)
    .def("SetBeatPos", &NotePos::SetBeatPos)
    .def("SetTimePos", &NotePos::SetTimePos)
    ;

  py::class_<SoundNote>(m, "SoundNote")
    .def(py::init<>())
    .def("type", &SoundNote::type)
    .def_readwrite("time", &SoundNote::time_msec)
    .def_readwrite("beat", &SoundNote::beat)
    .def_readwrite("measure", &SoundNote::measure)
    .def("toString", &SoundNote::toString)

    .def("SetRowPos", (void (SoundNote::*)(uint32_t, RowPos, RowPos)) &SoundNote::SetRowPos)
    .def("SetRowPos", (void (SoundNote::*)(double)) &SoundNote::SetRowPos)
    .def("SetBeatPos", &SoundNote::SetBeatPos)
    .def("SetTimePos", &SoundNote::SetTimePos)

    .def("SetAsBGM", &SoundNote::SetAsBGM)
    .def("SetAsTapNote", &SoundNote::SetAsTapNote)
    .def("SetMidiNote", &SoundNote::SetMidiNote)
    .def("SetLongnoteLength", &SoundNote::SetLongnoteLength)

    .def("IsLongnote", &SoundNote::IsLongnote)
    .def("IsScoreable", &SoundNote::IsScoreable)
    .def("GetPlayer", &SoundNote::GetPlayer)
    .def("GetLane", &SoundNote::GetLane)

    .def_readwrite("value", &SoundNote::value)
    ;
}