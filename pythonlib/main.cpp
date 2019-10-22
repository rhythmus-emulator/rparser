#include "pybind11/pybind11.h"
#include "Song.h"

namespace py = pybind11;
using namespace rparser;

template <typename A, typename T>
py::class_<T> pyclass_track(A &m, const char* name)
{
  return py::class_<T>(m, name)
    .def("clear", &T::clear)
    .def("toString", &T::toString)
    .def("size", &T::size)
    .def("front", &T::front)
    .def("end", &T::end)
    .def("set_track_count", &T::set_track_count)
    .def("IsEmpty", &T::is_empty)
    .def("AddObject", &T::AddObject)
    .def("AddObjectDuplicated", &T::AddObjectDuplicated)
    .def("ClearAll", &T::ClearAll)
    .def("ClearRange", &T::ClearRange)
    .def("MoveAll", &T::MoveAll)
    .def("MoveRange", &T::MoveRange)
    .def("RemoveObject", &T::RemoveObject)
    .def("RemoveObjectByBeat", &T::RemoveObjectByBeat)
    .def("RemoveObjectByPos", &T::RemoveObjectByPos)
    .def("swap", &T::swap)
    .def("InsertBlank", &T::InsertBlank)
    .def("IsHoldNoteAt", &T::IsHoldNoteAt)
    .def("IsRangeEmpty", &T::IsRangeEmpty);
}

template <typename A, typename T>
py::class_<T> pyclass_datatype(A &m, const char* name)
{
  return pyclass_track<A, T>(m, name)
    .def("ClearTrack", &T::ClearTrack)
    .def("UpdateTracks", &T::UpdateTracks)
    .def("GetAllTrackIterator", [](T& nd) { return nd.GetAllTrackIterator(); })
    .def("GetRowIterator", [](T& nd) { return nd.GetRowIterator(); })
    .def("get_track", [](const T& d, size_t i) { return d.get_track(i); })
    .def("__getitem__", [](const T& d, size_t i) { return d.get_track(i); });
}

template <typename A, typename T>
py::class_<T> pyclass_objtype(A &m, const char* name)
{
  return py::class_<T>(m, name)
    .def(py::init<>())
    .def_readwrite("time", &T::time_msec)
    .def_readwrite("beat", &T::beat)
    .def_readwrite("measure", &T::measure)
    .def("toString", &T::toString)
    .def("clone", &T::clone)
    .def("SetRowPos", (void (T::*)(uint32_t, RowPos, RowPos)) &T::SetRowPos)
    .def("SetRowPos", (void (T::*)(double)) &T::SetRowPos)
    .def("SetBeatPos", &T::SetBeatPos)
    .def("get_track", &T::get_track)
    .def("set_track", &T::set_track);
}

PYBIND11_MODULE(rparser_py, m)
{
  py::class_<Song>(m, "Song")
    .def(py::init<>())
    .def("Open", [](Song& s, const std::string& fn) { return s.Open(fn); })
    .def("Close", [](Song &s) { s.Close(); })
    .def("Close", &Song::Close)
    .def("GetChartCount", &Song::GetChartCount)
    .def("GetPath", &Song::GetPath)
    .def("GetChart", (Chart* (Song::*)(size_t)) &Song::GetChart)
    .def("GetChart", (Chart* (Song::*)(const std::string&)) &Song::GetChart)
    .def("NewChart", &Song::NewChart)
    .def("DeleteChart", &Song::DeleteChart)
    ;

  py::class_<Chart>(m, "Chart")
    .def(py::init<>())
    .def("GetBgaData", (BgaData& (Chart::*)()) &Chart::GetBgaData)
    .def("GetBgmData", (BgmData& (Chart::*)()) &Chart::GetBgmData)
    .def("GetMetaData", (MetaData& (Chart::*)()) &Chart::GetMetaData)
    .def("GetNoteData", (NoteData& (Chart::*)()) &Chart::GetNoteData)
    .def("GetTimingData", (TimingData& (Chart::*)()) &Chart::GetTimingData)
    .def("GetTimingSegmentData", (TimingSegmentData& (Chart::*)()) &Chart::GetTimingSegmentData)
    .def("GetEffectData", (EffectData& (Chart::*)()) &Chart::GetEffectData)

    .def("GetNoteCount", &Chart::GetScoreableNoteCount)
    .def("GetLastObjectTime", &Chart::GetSongLastObjectTime)
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

  py::class_<TimingSegmentData>(m, "TimingSegmentData")
    .def("GetTimeFromBeat", &TimingSegmentData::GetTimeFromBeat)
    .def("GetBeatFromTime", &TimingSegmentData::GetBeatFromTime)
    .def("GetBeatFromBar", &TimingSegmentData::GetBeatFromBar)
    .def("GetBarFromBeat", &TimingSegmentData::GetBarFromBeat)
    .def("GetMaxBpm", &TimingSegmentData::GetMaxBpm)
    .def("GetMinBpm", &TimingSegmentData::GetMinBpm)
    .def("HasBpmChange", &TimingSegmentData::HasBpmChange)
    .def("HasStop", &TimingSegmentData::HasStop)
    .def("HasWarp", &TimingSegmentData::HasWarp)
    .def("clear", &TimingSegmentData::clear)
    .def("toString", &TimingSegmentData::toString)
    ;

  pyclass_datatype<decltype(m), BgaData>(m, "BgaData");

  pyclass_datatype<decltype(m), BgmData>(m, "BgmData");

  pyclass_datatype<decltype(m), NoteData>(m, "NoteData");

  pyclass_datatype<decltype(m), EffectData>(m, "EffectData");

  pyclass_datatype<decltype(m), TimingData>(m, "TimingData");

  pyclass_datatype<decltype(m), TrackData>(m, "TrackData");

  pyclass_track<decltype(m), Track>(m, "Track");

  pyclass_objtype<decltype(m), NotePos>(m, "NotePos");

  pyclass_objtype<decltype(m), Note>(m, "Note")
    .def("chainsize", &Note::chainsize)
    .def("IsLongnote", [](Note& n) { return n.chainsize() > 1; })
    .def("get_player", &Note::get_player)
    .def("set_player", &Note::set_player)
    ;

  pyclass_objtype<decltype(m), EffectObject>(m, "EffectObject")
    .def("SetMidiCommand", &EffectObject::SetMidiCommand)
    .def("SetBmsARGBCommand", &EffectObject::SetBmsARGBCommand)
    .def("GetBga", &EffectObject::GetBga)
    .def("GetMidiCommand", &EffectObject::GetMidiCommand);

  pyclass_objtype<decltype(m), BgmObject>(m, "BgmObject")
    .def("set_bgm_type", &BgmObject::set_bgm_type)
    .def("get_bgm_type", &BgmObject::get_bgm_type)
    .def("set_column", &BgmObject::set_column)
    .def("get_column", &BgmObject::get_column);

  pyclass_objtype<decltype(m), BgaObject>(m, "BgaObject")
    .def("layer", &BgaObject::layer)
    .def("set_layer", &BgaObject::set_layer)
    .def("channel", &BgaObject::channel)
    .def("set_channel", &BgaObject::set_channel);

  pyclass_objtype<decltype(m), TimingObject>(m, "TimingObject")
    .def("SetBpm", &TimingObject::SetBpm)
    .def("SetBmsBpm", &TimingObject::SetBmsBpm)
    .def("SetStop", &TimingObject::SetStop)
    .def("SetBmsStop", &TimingObject::SetBmsStop)
    .def("SetMeasure", &TimingObject::SetMeasure)
    .def("SetScroll", &TimingObject::SetScroll)
    .def("SetTick", &TimingObject::SetTick)
    .def("SetWarp", &TimingObject::SetWarp)
    .def("GetFloatValue", &TimingObject::GetFloatValue)
    .def("GetIntValue", &TimingObject::GetIntValue);

  // TODO: add iterator for TrackData
  // TODO: add ChartUtil functions
  // TODO: add definition for adding note with smart pointer
}