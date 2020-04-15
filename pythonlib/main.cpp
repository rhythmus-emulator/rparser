#include "pybind11/pybind11.h"
#include "Song.h"
#include "ChartUtil.h"

namespace py = pybind11;
using namespace rparser;

template <typename A, typename T, typename NT>
py::class_<T> pyclass_track(A &m, const char* name)
{
  return py::class_<T>(m, name)
    .def("get", &T::get)
    .def("clear", &T::clear)
    .def("size", &T::size)
    .def("front", [](T &t) { return t.front(); })
    .def("back", [](T &t) { return t.back(); })
    .def("is_empty", &T::is_empty)
    .def("AddNoteElement", &T::AddNoteElement)
    .def("RemoveNoteElement", &T::RemoveNoteElement)
    .def("RemoveObjectByBeat", &T::RemoveObjectByBeat)
    .def("RemoveObjectByPos", &T::RemoveObjectByPos)
    .def("ClearAll", &T::ClearAll)
    .def("ClearRange", &T::ClearRange)
    .def("MoveAll", &T::MoveAll)
    .def("MoveRange", &T::MoveRange)
    .def("InsertBlank", &T::InsertBlank)
    .def("IsHoldNoteAt", &T::IsHoldNoteAt)
    .def("IsRangeEmpty", [](T &t, double m_start, double m_end) { return t.IsRangeEmpty(m_start, m_end); })
    ;
}

template <typename A, typename T, typename NT>
py::class_<T> pyclass_track_datatype(A &m, const char* name)
{
  return py::class_<T>(m, name)
    .def("clear", &T::clear)
    .def("size", &T::size)
    .def("front", [](T &t) { return t.front(); })
    .def("back", [](T &t) { return t.back(); })
    .def("is_empty", &T::is_empty)
    .def("AddNoteElement", &T::AddNoteElement)
    .def("RemoveNoteElement", &T::RemoveNoteElement)
    .def("RemoveObjectByBeat", &T::RemoveObjectByBeat)
    .def("RemoveObjectByPos", &T::RemoveObjectByPos)
    .def("ClearAll", &T::ClearAll)
    .def("ClearRange", &T::ClearRange)
    .def("MoveAll", &T::MoveAll)
    .def("MoveRange", &T::MoveRange)
    .def("InsertBlank", &T::InsertBlank)
    .def("IsHoldNoteAt", &T::IsHoldNoteAt)
    .def("IsRangeEmpty", [](T &t, double m_start, double m_end) { return t.IsRangeEmpty(m_start, m_end); })
    .def("toString", &T::toString)
    .def("GetAllTrackIterator", [](T& nd) { return nd.GetAllTrackIterator(); })
    .def("GetRowIterator", [](T& nd) { return nd.GetRowIterator(); })
    .def("get_track", [](const T& d, size_t i) { return d.get_track(i); })
    .def("__getitem__", [](const T& d, size_t i) { return d.get_track(i); });
}

template <typename A, typename T>
py::class_<T> pyclass_objtype(A &m, const char* name)
{
  // TODO: get_value, set_value, get_point, set_point
  return py::class_<T>(m, name)
    .def(py::init<>())
    .def("time", &T::time_msec)
    .def("beat", &T::beat)
    .def("measure", &T::measure)
    .def("set_time", &T::set_time_msec)
    .def("set_beat", &T::set_beat)
    .def("set_measure", &T::set_measure)
    .def("toString", &T::toString);
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
    .def("GetMetaData", (MetaData& (Chart::*)()) &Chart::GetMetaData)
    .def("GetBgmData", (TrackData& (Chart::*)()) &Chart::GetBgmData)
    .def("GetNoteData", (TrackData& (Chart::*)()) &Chart::GetNoteData)
    .def("GetCommandData", (TrackData& (Chart::*)()) &Chart::GetCommandData)
    .def("GetTimingData", (TrackData& (Chart::*)()) &Chart::GetTimingData)
    .def("GetTimingSegmentData", (TimingSegmentData& (Chart::*)()) &Chart::GetTimingSegmentData)

    .def("GetNoteCount", &Chart::GetScoreableNoteCount)
    .def("GetLastObjectTime", &Chart::GetSongLastObjectTime)
    .def("HasLongNote", &Chart::HasLongnote)
    .def("GetPlayLaneCount", &Chart::GetPlayLaneCount)
    .def("IsEmpty", &Chart::IsEmpty)
    .def("GetHash", &Chart::GetHash)
    .def("GetFilename", &Chart::GetFilename)

    .def("Update", &Chart::Update)
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
    .def("GetBeatFromMeasure", &TimingSegmentData::GetBeatFromMeasure)
    .def("GetMeasureFromBeat", &TimingSegmentData::GetMeasureFromBeat)
    .def("GetMeasureFromTime", &TimingSegmentData::GetMeasureFromTime)
    .def("GetMaxBpm", &TimingSegmentData::GetMaxBpm)
    .def("GetMinBpm", &TimingSegmentData::GetMinBpm)
    .def("HasBpmChange", &TimingSegmentData::HasBpmChange)
    .def("HasStop", &TimingSegmentData::HasStop)
    .def("HasWarp", &TimingSegmentData::HasWarp)
    .def("clear", &TimingSegmentData::clear)
    .def("toString", &TimingSegmentData::toString)
    ;

  pyclass_track_datatype<decltype(m), TrackData, NoteElement>(m, "TrackData");

  pyclass_track<decltype(m), Track, NoteElement>(m, "Track");

  pyclass_objtype<decltype(m), NoteElement>(m, "NotePos");

  py::class_<TrackData::all_track_iterator>(m, "AllTrackIterator")
    .def("get", &TrackData::all_track_iterator::get)
    .def("next", &TrackData::all_track_iterator::next)
    .def("get_track", &TrackData::all_track_iterator::get_track)
    .def("is_end", &TrackData::all_track_iterator::is_end);

  py::class_<TrackData::row_iterator>(m, "RowIterator")
    .def("get", [](const TrackData::row_iterator &riter, size_t i) { return riter[i]; })
    .def("get_measure", &TrackData::row_iterator::get_measure)
    .def("next", &TrackData::row_iterator::next)
    .def("is_end", &TrackData::row_iterator::is_end);

#if 0
  pyclass_objtype<decltype(m), Note>(m, "Note")
    .def("chainsize", &Note::chainsize)
    .def("IsLongnote", [](Note& n) { return n.chainsize() > 1; })
    .def("get_player", &Note::get_player)
    .def("set_player", &Note::set_player)
    ;
#endif

  // ChartUtil functions
  m.def("ExportToHTML", ExportToHTML);
  py::class_<effector::EffectorParam>(m, "EffectorParam");
  m.def("SetLanefor7Key", effector::SetLanefor7Key);
  m.def("SetLanefor9Key", effector::SetLanefor9Key);
  m.def("SetLaneforBMS1P", effector::SetLaneforBMS1P);
  m.def("SetLaneforBMS2P", effector::SetLaneforBMS2P);
  m.def("SetLaneforBMSDP1P", effector::SetLaneforBMSDP1P);
  m.def("SetLaneforBMSDP2P", effector::SetLaneforBMSDP2P);
  m.def("GenerateRandomColumn", effector::GenerateRandomColumn);
  m.def("Random", effector::Random);
  m.def("SRandom", effector::SRandom);
  m.def("HRandom", effector::HRandom);
  m.def("RRandom", effector::RRandom);
  m.def("Mirror", effector::Mirror);
  m.def("AllSC", effector::AllSC);
  m.def("Flip", effector::Flip);

  // TODO: add definition for adding note with smart pointer
}