#include "Song.h"
#include "ChartUtil.h"
#include "MetaData.h"
#include "ChartLoader.h"
#include "ChartWriter.h"
#include "common.h"
#include <list>

#define BMSTYPES  \
  BMS( SONGTYPE::BMS, "bms" ),     \
  BMS( SONGTYPE::BMS, "bme" ),     \
  BMS( SONGTYPE::BMS, "bml" ),     \
  BMS( SONGTYPE::BMSON, "bmson" ), \
  BMS( SONGTYPE::OSU, "osu" ),     \
  BMS( SONGTYPE::SM, "sm" ),       \
  BMS( SONGTYPE::OJM, "ojm" ),     \
  BMS( SONGTYPE::VOS, "vos" ),     \
  BMS( SONGTYPE::DTX, "dtx" )
//BMS( SONGTYPE::BMS, "zip" ),     \ -- is directory.

using namespace rutil;

namespace rparser
{

SONGTYPE GetSongTypeByName(const std::string& filename)
{
#define BMS(a,b) {b,a}
  static const std::map<std::string, SONGTYPE> ext_2_type_ = {
    BMSTYPES
  };
#undef BMS

  const std::string ext(std::move(rutil::GetExtension(filename)));
  auto ii = ext_2_type_.find(ext.c_str());
  if (ii == ext_2_type_.end()) return SONGTYPE::NONE;
  else return ii->second;
}

const char* GetExtensionBySongType(SONGTYPE iType)
{
  if (iType == SONGTYPE::NONE)
    return "none";

#define BMS(a,b) {a,b}
  static const std::map<SONGTYPE, const char*> type_2_str_ = {
    BMSTYPES
  };
#undef BMS

  // this function should ensure no-exception
  // by supplying string for all types of SONGTYPE.
  return type_2_str_.find(iType)->second;
}

const char* ChartTypeToString(CHARTTYPE type)
{
  static const char* type_2_str[] = {
    0,
    "4Key",
    "5Key",
    "6Key",
    "7Key",
    "8Key",
    "9Key",
    "10Key",
    "IIDX5Key",
    "IIDX10Key",
    "IIDXSP",
    "IIDXDP",
    "Popn",
    "Drummania",
    "Guitarfreaks",
    "jubeat",
    "DDR",
    "DDR_DP",
    "Pump",
    "Pump_DP",
    "SDVX",
  };
  return type_2_str[(size_t)type];
}

const char** GetSongExtensions()
{
#define BMS(a,b) b
  static const char *kChartExtensions[] = {
    BMSTYPES, 0
  };
#undef BMS
  return kChartExtensions;
}

Song::Song()
  : songtype_(SONGTYPE::NONE), error_(ERROR::NONE), directory_(0)
{
}

Song::~Song()
{
	Close();
}

size_t Song::GetChartCount() const
{
  return charts_.size();
}

Chart* Song::NewChart()
{
  Chart *c = new Chart();

  // set shared segments
  switch (songtype_)
  {
  case SONGTYPE::BMS:
    c->GetNoteData().set_track_count(8);
    break;
  case SONGTYPE::VOS:
    c->GetNoteData().set_track_count(7);
    break;
  case SONGTYPE::SM:
    c->GetNoteData().set_track_count(4);
    c->shared_data_.trackdata[TrackTypes::kTrackCommand] = &chart_shared_.trackdata_[TrackTypes::kTrackCommand];
    c->shared_data_.trackdata[TrackTypes::kTrackTiming] = &chart_shared_.trackdata_[TrackTypes::kTrackTiming];
    c->shared_data_.timingsegmentdata = &chart_shared_.timingsegmentdata_;
    break;
  default:
    ASSERT(0);
  }

  c->SetParent(this);
  charts_.push_back(c);
  return c;
}

Chart* Song::GetChart(size_t idx)
{
  if (idx >= charts_.size()) return nullptr;
  return charts_[idx];
}

Chart* Song::GetChart(const std::string& filename)
{
  for (auto *c : charts_)
  {
    if (c->GetFilename() == filename)
      return c;
  }
  return nullptr;
}

void Song::DeleteChart(size_t idx)
{
  if (idx >= charts_.size()) return;
  delete charts_[idx];
  charts_.erase(charts_.begin() + idx);
}

void Song::SetSongType(SONGTYPE songtype)
{
  songtype_ = songtype;

  // Change all chart types if possible.
  for (auto* c : charts_)
  {
    c->UpdateCharttype();
  }
}

SONGTYPE Song::GetSongType() const
{
  return songtype_;
}

bool Song::Open(const std::string &path, SONGTYPE songtype)
{
  bool r = false;
	Close();
  filepath_ = path;

  // Check method to open song file -- binary or directory.
  if (DirectoryManager::OpenDirectory(path))
  {
    // directory type --> detect songtype in directory
    DirectoryManager::OpenDirectory(path);
    directory_ = DirectoryManager::GetDirectory(path);
    if (!directory_)
      return false;
    if (songtype == SONGTYPE::NONE) SetSongType(DetectSongtype());
    else SetSongType(songtype);
  }
  else
  {
    // check at least file is existing ...
    // if not, raise error
    if (!rutil::IsFile(path))
    {
      error_ = ERROR::OPEN_NO_FILE;
      songtype_ = SONGTYPE::NONE;
      return false;
    }

    // file type --> detect songtype in file
    if (songtype == SONGTYPE::NONE) SetSongType(GetSongTypeByName(path));
    else SetSongType(songtype);
    filepath_ = path;
  }

  // Check songtype -- must set by now.
  if (GetSongType() == SONGTYPE::NONE)
    return false;

  // Load binary into chartloader
  // directory type song --> Load using directory
  // file type song --> Load using file
	ChartLoader *cl = ChartLoader::Create(this);
  if (directory_)
    r = cl->LoadFromDirectory();
  else
  {
    FileData fd;
    rutil::ReadFileData(path, fd);
    ASSERT(fd.len > 0);
    Chart *c = NewChart();
    c->SetFilename(filepath_);
    r = cl->Load(*c, fd.p, fd.len);
    if (!r)
      DeleteChart(0);
  }
	delete cl;

	return r;
}

bool Song::Save()
{
  ChartWriter* writer = CreateChartWriter(songtype_);
  if (!writer)
  {
    error_ = ERROR::WRITE_SERIALIZE_CHART;
    return false;
  }

  // Put charts and metadata to writer
  writer->Write(this);

	// save directory and cleanup
  if (directory_)
    directory_->Save();
  delete writer;
	return true;
}

// only save single chart
bool Song::SaveChart(Chart *chart)
{
  if (!chart) return false;

  ChartWriter* writer = CreateChartWriter(songtype_);
  if (!writer)
  {
    error_ = ERROR::WRITE_SERIALIZE_CHART;
    return false;
  }

  writer->WriteChart(chart);

  // save directory and cleanup
  if (directory_)
    directory_->Save();
  delete writer;
  return true;
}

bool Song::Close(bool save)
{
  if (save && !Save())
  {
    // error message already assigned at Save()
    return false;
  }

  // release resources and memory.
  for (auto *c : charts_)
    delete c;
  charts_.clear();
  if (directory_)
  {
    directory_.reset();
    DirectoryManager::CloseDirectory(filepath_);
  }
  error_ = ERROR::NONE;
  songtype_ = SONGTYPE::NONE;
  filepath_.clear();

  return true;
}

SONGTYPE Song::DetectSongtype()
{
  SONGTYPE songtype;

  // Try to detect SONGTYPE by name of resource file.
  songtype = GetSongTypeByName(directory_->GetPath());
  if (songtype != SONGTYPE::NONE) return songtype;

  // If not detected, then try to detect SONGTYPE by file lists.
  for (auto *f : *directory_)
  {
    songtype = GetSongTypeByName(f->filename);
    if (songtype != SONGTYPE::NONE)
      return songtype;
  }
  return SONGTYPE::NONE;
}

bool Song::SaveAs(const std::string & newpath)
{
  return directory_->SaveAs(newpath);
}

const std::string Song::GetPath() const
{
	return directory_->GetPath();
}

const char* Song::GetErrorStr() const
{
	return get_error_msg(error_);
}

Directory * Song::GetDirectory()
{
	return directory_.get();
}

std::string Song::GetHash() const
{
  /* use default hash with bit 1
     to avoid creating same hash with chart. */
  std::string hash = "11111111"
    "11111111"
    "11111111"
    "11111111";
  for (auto *c : charts_)
    hash = md5_sum(hash, c->GetHash());
  return hash;
}

std::string Song::toString(bool detailed) const
{
	std::stringstream ss;
	ss << "Song path: " << GetPath() << std::endl;
	ss << "Song type: " << GetExtensionBySongType(songtype_) << std::endl;
  ss << "Chart count: " << charts_.size() << std::endl << std::endl;
  if (detailed)
  {
    for (auto *c : charts_)
    {
      ss << c->toString() << std::endl;
    }
  }
	return ss.str();
}



}
