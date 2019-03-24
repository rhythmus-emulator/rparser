#include "Song.h"
#include "ChartUtil.h"
#include "MetaData.h"
#include "ChartLoader.h"
#include "ChartWriter.h"
#include <list>

#define BMSTYPES  \
  BMS( SONGTYPE::BMS, "zip" ),     \
  BMS( SONGTYPE::BMS, "bms" ),     \
  BMS( SONGTYPE::BMSON, "bmson" ), \
  BMS( SONGTYPE::OSU, "osu" ),     \
  BMS( SONGTYPE::SM, "sm" ),       \
  BMS( SONGTYPE::OJM, "ojm" ),     \
  BMS( SONGTYPE::VOS, "vos" ),     \
  BMS( SONGTYPE::DTX, "dtx" )

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
  : songtype_(SONGTYPE::NONE), error_(ERROR::NONE), resource_(0), chartlist_(0)
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
  return chartlist_->GetChartData(chartlist_->AddNewChart());
}

Chart* Song::GetChart(int i)
{
  if (!chartlist_) return 0;
  chartlist_->CloseChartData();
  return chartlist_->GetChartData(i);
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

bool Song::Open(const std::string & path, bool fastread, SONGTYPE songtype)
{
	Close();

	// Read file binaries.
  resource_ = ResourceFactory::Open(path.c_str(), fastread ? GetSongExtensions() : 0);
	if (!resource_)
	{
		error_ = ERROR::OPEN_NO_FILE;
		songtype_ = SONGTYPE::NONE;
		return false;
	}

  // Detect song type and prepare chartlist.
  if (songtype == SONGTYPE::NONE) SetSongType(DetectSongtype());
  else SetSongType(songtype_);

  // Load binary into chartloader
	ChartLoader *cl = CreateChartLoader(songtype);
  cl->SetChartList(chartlist_);
  for (auto ii = resource_->data_begin(); ii != resource_->data_end(); ++ii)
  {
    if (!cl->Load(ii->second.p, ii->second.len))
    {
      // Error might be occured during chart loading,
      // But won't stop loading as there *might* be 
      // So, just skip the wrong chart file and make log.
      error_ = ERROR::OPEN_INVALID_CHART;
      continue;
    }
  }
  cl->LoadCommonData(*chartlist_, *resource_);
	delete cl;

	return true;
}

bool Song::Save()
{
  if (!resource_) return false;
  if (!chartlist_) return false;
  if (chartlist_->IsChartOpened()) return false;

  ChartWriter* writer = CreateChartWriter(songtype_);
  if (!writer)
  {
    error_ = ERROR::WRITE_SERIALIZE_CHART;
    return false;
  }

  // Put charts and metadata to writer
  for (auto i = 0; i < chartlist_->size(); ++i)
  {
    const Chart* c = chartlist_->GetChartData(i);
    if (!writer->Serialize( *c ))
    {
      chartlist_->CloseChartData();
      error_ = ERROR::WRITE_SERIALIZE_CHART;
      return false;
    }
    if (writer->IsWritable())
      resource_->AddBinary(writer->GetFilename(), writer->GetData());
    chartlist_->CloseChartData();
  }
  writer->SerializeCommonData(*chartlist_);
  if (writer->IsWritable())
    resource_->AddBinary(writer->GetFilename(), writer->GetData());

	// flush (save to real file) and cleanup
	resource_->Flush();
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
  delete chartlist_;
  delete resource_;
  chartlist_ = 0;
  resource_ = 0;
	error_ = ERROR::NONE;
	songtype_ = SONGTYPE::NONE;

  return true;
}

SONGTYPE Song::DetectSongtype()
{
  SONGTYPE songtype;

  // Try to detect SONGTYPE by name of resource file.
  songtype = GetSongTypeByName(resource_->GetPath());
  if (songtype != SONGTYPE::NONE) return songtype;

  // If not detected, then try to detect SONGTYPE by file lists.
  for (auto ii = resource_->data_begin(); ii != resource_->data_end(); ++ii)
  {
    songtype = GetSongTypeByName(ii->first);
    if (songtype != SONGTYPE::NONE)
      return songtype;
  }
  return SONGTYPE::NONE;
}


void Song::SetPath(const std::string & path)
{
	// Set new path for Resource
  // TODO:: method for ResourceFolder
	resource_->SetPath(path.c_str());
}

const std::string Song::GetPath() const
{
	return resource_->GetPath();
}

const char* Song::GetErrorStr() const
{
	return get_error_msg(error_);
}

Resource * Song::GetResource()
{
	return resource_;
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
    for (auto i = 0; i < chartlist_->size(); i++)
    {
      const Chart *c = chartlist_->GetChartData(i);
      ss << c->toString() << std::endl;
      chartlist_->CloseChartData();
    }
  }
	return ss.str();
}



}
