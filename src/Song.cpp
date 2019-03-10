#include "Song.h"
#include "Chart.h"
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
  : songtype_(SONGTYPE::NONE), error_(ERROR::NONE),
    objs_shared_(0), metadata_shared_(0), resource_(0)
{
}

Song::~Song()
{
	Close();
}

void Song::RegisterChart(Chart * c, const std::string& filename)
{
	charts_.push_back({ c, filename, filename, false });
}

bool Song::DeleteChart(const Chart * c)
{
	for (auto p = charts_.begin(); p != charts_.end(); ++p) {
		if (p->c == c) {
			charts_.erase(p);
			return true;
		}
	}
	return false;
}

bool Song::RenameChart(const Chart* c, const std::string& newfilename)
{
	for (auto p = charts_.begin(); p != charts_.end(); ++p) {
		if (p->c == c) {
			p->new_filename = newfilename;
			p->is_dirty = true;
			return true;
		}
	}
	return false;
}

bool Song::Open(const std::string & path, bool fastread, SONGTYPE songtype)
{
  Chart *c;

	Close();

	// Read file binaries.
  resource_ = ResourceFactory::Open(path.c_str(), fastread ? GetSongExtensions() : 0);
	if (!resource_)
	{
		error_ = ERROR::OPEN_NO_FILE;
		songtype_ = SONGTYPE::NONE;
		return false;
	}

#if 0
  // If no chart than fail.
	if (chart_files.size() == 0)
	{
		error_ = ERROR::OPEN_NO_CHART;
		songtype_ == SONGTYPE::NONE;
		return false;
	}
#endif

  if (songtype == SONGTYPE::NONE) DetectSongtype();
  else songtype_ = songtype;

  // Load common metadata / shared objects if necessary.
	switch (songtype_)
	{
	case SONGTYPE::OSU:
	case SONGTYPE::VOS:
		metadata_shared_ = new MetaData();
		objs_shared_ = new NoteData();
		LoadMetadata();
		break;
	default:
		break;
	}

  // XXX: one file may create multiple charts
	ChartLoader *cl = CreateChartLoader(songtype);
  for (auto ii = resource_->data_begin(); ii != resource_->data_end(); ++ii)
  {
    c = new Chart(metadata_shared_, objs_shared_);
    if (!cl->Load(ii->second.p, ii->second.len))
    {
      // Error might be occured during chart loading,
      // But won't stop loading as there *might* be 
      // So, just skip the wrong chart file and make log.
      error_ = ERROR::OPEN_INVALID_CHART;
      delete c;
      continue;
    }
    RegisterChart(c, ii->first);
  }
	delete cl;

	return true;
}

bool Song::Save()
{
  if (!resource_) return false;

  ChartWriter* writer = GetChartWriter(songtype_);
  if (!writer)
  {
    error_ = ERROR::WRITE_SERIALIZE_CHART;
    return false;
  }

  // Put charts and metadata to writer
	for (ChartFile &cf : charts_)
	{
    if (!writer->Serialize(*cf.c))
    {
      error_ = ERROR::WRITE_SERIALIZE_CHART;
      return false;
    }
	}
  writer->SerializeCommonData(*objs_shared_, *metadata_shared_);

  // Write binaries to resource
  for (auto ii = writer->data_begin(); ii != writer->data_end(); ++ii)
  {
    resource_->AddBinary("TODO", *ii);
  }

	// flush (save to real file) and cleanup
	resource_->Flush();
  delete writer;
	return true;
}

bool Song::Rename(const std::string& newPath)
{
	// XXX: not implemented yet (do it in Resource class)
	return false;
}

bool Song::Close(bool save)
{
	if (save && !Save())
	{
		// error message already assigned at Save()
		return false;
	}

	// release resources and memory.
	for (ChartFile &cf : charts_)
	{
		delete cf.c;
	}
	charts_.clear();
  delete resource_;
	error_ = ERROR::NONE;
	songtype_ = SONGTYPE::NONE;
	delete objs_shared_;
	delete metadata_shared_;
}

bool Song::LoadMetadata()
{
	assert(songtype_ != SONGTYPE::NONE);

	// TODO
	switch (songtype_) {
	case SONGTYPE::OSU:
		return false;
	}
	return true;
}

bool Song::SaveMetadata()
{
	assert(songtype_ != SONGTYPE::NONE);

	// TODO
	switch (songtype_) {
	case SONGTYPE::OSU:
		return false;
	default:
		break;
	}
	return true;
}

bool Song::DetectSongtype()
{
  // Try to detect SONGTYPE by name of resource file.
  songtype_ = GetSongTypeByName(resource_->GetPath());
  if (songtype_ != SONGTYPE::NONE) return true;

  // If not detected, then try to detect SONGTYPE by file lists.
  for (auto ii = resource_->data_begin(); ii != resource_->data_end(); ++ii)
  {
    songtype_ = GetSongTypeByName(ii->first);
    if (songtype_ != SONGTYPE::NONE)
      return true;
  }
  return false;
}

bool Song::ChangeSongType(SONGTYPE songtype)
{
  // XXX: resource type should be also changed ...
  // XXX: "total integrated interface" necessary -- think about it
	MetaData *metadata_common = 0;
  NoteData *objs_common = 0;
  Chart *new_chart = 0;

	if (songtype == songtype_)
		return true;

	// check whether global timingdata / metadata is necesssary
	switch (songtype)
	{
	case SONGTYPE::OSU:
	case SONGTYPE::VOS:
		if (!metadata_shared_) metadata_shared_ = new MetaData();
		if (!objs_shared_) objs_shared_ = new NoteData();
		break;
	default:
		if (metadata_shared_) metadata_common = metadata_shared_, metadata_shared_ = 0;
		if (objs_shared_) objs_common = objs_shared_, objs_shared_ = 0;
		break;
	}

  for (ChartFile &cf : charts_)
  {
    new_chart = new Chart(metadata_shared_, objs_shared_);
    new_chart->swap(*cf.c);
    delete cf.c;
    cf.c = new_chart;
  }

	songtype_ = songtype;
	delete metadata_common;
	delete objs_common;
	return true;
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

void Song::GetCharts(std::vector<Chart*>& charts)
{
	charts.clear();
	for (ChartFile &cf : charts_)
		charts.push_back(cf.c);
}

const char* Song::GetErrorStr() const
{
	return get_error_msg(error_);
}

Resource * Song::GetResource()
{
	return resource_;
}

std::string Song::toString() const
{
	std::stringstream ss;
	ss << "Song path: " << GetPath() << std::endl;
	ss << "Song type: " << GetChartExtension(songtype_) << std::endl;
	ss << "Chart count: " << charts_.size() << std::endl << std::endl;
	for (auto cf : charts_)
	{
		ss << cf.c->toString() << std::endl << std::endl;
	}
	return ss.str();
}



}
