#include "Song.h"
#include "ChartUtil.h"
#include "MetaData.h"
#include "ChartLoader.h"
#include "ChartWriter.h"
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
  : songtype_(SONGTYPE::NONE), error_(ERROR::NONE), directory_(0), chartlist_(0)
{
}

Song::~Song()
{
	Close();
}

size_t Song::GetChartCount()
{
  if (!chartlist_) return 0;
  return chartlist_->size();
}

Chart* Song::NewChart()
{
  if (!chartlist_) return 0;
  chartlist_->CloseChartData();
  Chart *c = chartlist_->GetChartData(chartlist_->AddNewChart());
  c->SetParent(this);   // Set ownership before returning object
  return c;
}

Chart* Song::GetChart(int i)
{
  if (!chartlist_) return 0;
  chartlist_->CloseChartData();
  Chart *c = chartlist_->GetChartData(i);
  c->SetParent(this);   // Set ownership before returning object
  return c;
}

Chart* Song::GetChart(const std::string& filename)
{
  if (!chartlist_) return 0;
  int idx = chartlist_->GetChartIndexByName(filename);
  if (idx < 0) return 0;
  else return GetChart(idx);
}

void Song::CloseChart()
{
  if (!chartlist_) return;
  chartlist_->CloseChartData();
  // TODO: if file renamed then should rename filename of resource.
}

void Song::DeleteChart(int idx)
{
  if (!chartlist_) return;
  chartlist_->DeleteChart(idx);
}

bool Song::SetSongType(SONGTYPE songtype)
{
  if (songtype == songtype_) return true;
  if (chartlist_) return false; // must close first (XXX: changing function add)

  // create chartlist based on 
  switch (songtype)
  {
  case SONGTYPE::OSU:
  case SONGTYPE::VOS:
    chartlist_ = new ChartNoteList();
    break;
  default:
    chartlist_ = new ChartList();
    break;
  }

  songtype_ = songtype;
  return true;
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
  if (IsSongExtensionIsFiletype(path))
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
  else
  {
    // directory type --> detect songtype in directory
    DirectoryManager::OpenDirectory(path);
    directory_ = DirectoryManager::GetDirectory(path);
    if (!directory_)
      return false;
    if (songtype == SONGTYPE::NONE) SetSongType(DetectSongtype());
    else SetSongType(songtype);
  }

  // Check songtype -- must set by now.
  if (GetSongType() == SONGTYPE::NONE)
    return false;

  // Load binary into chartloader
  // directory type song --> Load using directory
  // file type song --> Load using file
	ChartLoader *cl = ChartLoader::Create(songtype_);
  if (directory_)
    r = cl->LoadFromDirectory(*chartlist_, *directory_);
  else
  {
    FileData fd;
    rutil::ReadFileData(path, fd);
    ASSERT(fd.len > 0);
    Chart *c = chartlist_->GetChartData(chartlist_->AddNewChart());
    //c->SetFilename(filepath_);
    c->SetHash(rutil::md5_str(fd.GetPtr(), fd.GetFileSize()));
    r = cl->Load(*c, fd.p, fd.len);
    chartlist_->CloseChartData();
  }
	delete cl;

	return r;
}

bool Song::Save()
{
  if (!chartlist_) return false;
  if (chartlist_->IsChartOpened()) return false;

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
  CloseChart();

  if (save && !Save())
  {
    // error message already assigned at Save()
    return false;
  }

  // release resources and memory.
  delete chartlist_;
  chartlist_ = 0;
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
  for (auto f : *directory_)
  {
    songtype = GetSongTypeByName(f.filename);
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

bool Song::IsSongExtensionIsFiletype(const std::string& path)
{
  std::string ext = rutil::lower(rutil::GetExtension(path));
  return (ext == "vos");
}

std::string Song::toString(bool detailed) const
{
	std::stringstream ss;
	ss << "Song path: " << GetPath() << std::endl;
	ss << "Song type: " << GetExtensionBySongType(songtype_) << std::endl;
  if (chartlist_)
    ss << "Chart count: " << chartlist_->size() << std::endl << std::endl;
  else
    ss << "(Chart list inavailable)" << std::endl;
  if (detailed && chartlist_)
  {
    chartlist_->CloseChartData();
    for (auto i = 0u; i < chartlist_->size(); i++)
    {
      const Chart *c = chartlist_->GetChartData(i);
      ss << c->toString() << std::endl;
      chartlist_->CloseChartData();
    }
  }
	return ss.str();
}



}
