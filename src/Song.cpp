#include "Song.h"
#include "ChartUtil.h"
#include "MetaData.h"
#include "ChartLoader.h"
#include "ChartWriter.h"
#include <list>

using namespace rutil;

namespace rparser
{

// used when detecting chart file / inferencing song type
const std::list<std::pair<SONGTYPE, const char*>> type_2_ext_ = {
	{SONGTYPE::BMS, "bms"},
	{SONGTYPE::BMS, "bme"},
	{SONGTYPE::BMS, "bml"},
	{SONGTYPE::OSU, "osu"},
	{SONGTYPE::SM, "sm"},
};

const std::string Song::gen_readable_ext_()
{
	std::string r;
	// prepare for total readable extensions
	for (auto ii : type_2_ext_)
	{
		r += ii.second;
		r += ";";
	}
	if (r.back() == ';')
		r.pop_back();
	return r;
}
const std::string Song::total_readable_ext_ = Song::gen_readable_ext_();

// ------ class Song ------

Song::Song()
	: songtype_(SONGTYPE::NONE), error_(ERROR::NONE),
	timingdata_global_(0), metadata_global_(0)
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

	// initialize
	Close();
	songtype_ = songtype;

	// Read given path first.
	// If required, only read chart files.
	const char * fastread_ext = total_readable_ext_.c_str();
	if (!fastread) fastread_ext = 0;
	if (!resource_.Open(path.c_str(), 0, fastread_ext))
	{
		error_ = ERROR::OPEN_NO_FILE;
		songtype_ = SONGTYPE::NONE;
		return false;
	}

	// Filter out file from resource, which is feasible to read.
	std::map<std::string, const Resource::BinaryData*> chart_files;
	resource_.FilterFiles(total_readable_ext_.c_str(), chart_files);
#if 0
	if (chart_files.size() == 0)
	{
		error_ = ERROR::OPEN_NO_CHART;
		songtype_ == SONGTYPE::NONE;
		return false;
	}
#endif

	// Auto-detect songtype, if necessary.
	if (songtype_ == SONGTYPE::NONE)
	{
		for (auto ii : type_2_ext_)
		{
			const char* ext_curr = ii.second;
			if (chart_files.find(ext_curr) != chart_files.end())
			{
				songtype_ = ii.first;
			}
		}
#if 0
		// if resource is archive file and SONGTYPE::BMS,
		// then change type to SONGTYPE::BMSARCH
		if (songtype_ == SONGTYPE::NONE)
		{
			error_ = ERROR::OPEN_NO_CHART;
			return false;
		}
#endif
	}

	ChartLoader *cl = CreateChartLoader(songtype);
	switch (songtype)
	{
	case SONGTYPE::OSU:
	case SONGTYPE::VOS:
		metadata_global_ = new MetaData();
		timingdata_global_ = new TimingData();

		LoadMetadata();

		for (auto ii : chart_files)
		{
			c = new Chart(metadata_global_, timingdata_global_);
			if (!cl->Load(ii.second->p, ii.second->len))
			{
				// Error might be occured during chart loading,
				// But won't stop loading as there *might* be 
				// So, just skip the wrong chart file and make log.
				error_ = ERROR::OPEN_INVALID_CHART;
				delete c;
				continue;
			}
			RegisterChart(c, ii.first);
		}
		break;
	default:
		for (auto ii : chart_files)
		{
			c = new ChartBMS();
			if (!cl->Load(ii.second->p, ii.second->len))
			{
				// Error might be occured during chart loading,
				// But won't stop loading as there *might* be 
				// So, just skip the wrong chart file and make log.
				error_ = ERROR::OPEN_INVALID_CHART;
				delete c;
				continue;
			}
			RegisterChart(c, ii.first);
		}
		break;
	}
	delete cl;

	return true;
}

bool Song::Save()
{
	// Only update dirty charts
	for (ChartFile &cf : charts_)
	{
		if (cf.is_dirty)
		{
			Resource::BinaryData data;
			// remove previous file and create new file again
			resource_.Delete(cf.old_filename);
			if (!SerializeChart(*cf.c, data))
			{
				error_ = ERROR::WRITE_SERIALIZE_CHART;
				return false;
			}
			resource_.AddBinary(cf.new_filename, data.p, data.len, true, false);
			cf.old_filename = cf.new_filename;
			cf.is_dirty = false;
		}
	}

	// save metadata, in case of need.
	if (!SaveMetadata())
	{
		error_ = ERROR::WRITE_METADATA;
		return false;
	}

	// flush (save to real file)
	resource_.Flush();
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
	resource_.Unload(false);
	error_ = ERROR::NONE;
	songtype_ = SONGTYPE::NONE;
	delete timingdata_global_;
	delete metadata_global_;
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
		// pass
	}
	return true;
}

bool Song::ChangeSongType(SONGTYPE songtype)
{
	MetaData *metadata_common = 0;
	TimingData *timingdata_common = 0;
	Chart *new_chart = 0;

	if (songtype == songtype_)
		return true;

	// check whether global timingdata / metadata is necesssary
	switch (songtype)
	{
	case SONGTYPE::OSU:
	case SONGTYPE::VOS:
		if (!metadata_global_) metadata_global_ = new MetaData();
		if (!timingdata_global_) timingdata_global_ = new TimingData();
		for (ChartFile &cf : charts_)
		{
			new_chart = new Chart(metadata_global_, timingdata_global_);
			new_chart->swap(*cf.c);
			delete cf.c;
			cf.c = new_chart;
		}
		break;
	default:
		if (metadata_global_) metadata_common = metadata_global_, metadata_global_ = 0;
		if (timingdata_global_) timingdata_common = timingdata_global_, timingdata_global_ = 0;
		for (ChartFile &cf : charts_)
		{
			new_chart = new ChartBMS();
			new_chart->swap(*cf.c);
			delete cf.c;
			cf.c = new_chart;
		}
		break;
	}

	songtype_ = songtype;
	delete metadata_common;
	delete timingdata_common;
	return true;
}

void Song::SetPath(const std::string & path)
{
	// Set new path for Resource
	resource_.SetPath(path.c_str());
	//
}

const std::string Song::GetPath() const
{
	return resource_.GetPath();
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
	return &resource_;
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

const char* GetChartExtension(SONGTYPE iType)
{
	static const std::map<SONGTYPE, const char*> type_2_str_ = {
		{SONGTYPE::BMS, "bms"},
		{SONGTYPE::BMSON, "bmson"},
		{SONGTYPE::BMSARCH, "bms"},
		{SONGTYPE::OSU, "osu"},
		{SONGTYPE::SM, "sm"},
		{SONGTYPE::OJM, "ojm"},

		// binary type: return ext "chart"
		{SONGTYPE::VOS, "chart"},

		// nonetype (dummy)
		{SONGTYPE::NONE, ""},
	};
	// this function should ensure no-exception
	// by supplying string for all types of SONGTYPE.
	return type_2_str_.find(iType)->second;
}

const char* GetSongExtension(SONGTYPE iType)
{
	// not include directory type extension,
	// it'll return empty string.

	static const std::map<SONGTYPE, const char*> type_2_ext_ = {
		{SONGTYPE::BMSARCH, "zip"},
		{SONGTYPE::VOS, "vos"},
		{SONGTYPE::OSU, "osu"},
		{SONGTYPE::OJM, "ojm"},
		{SONGTYPE::DTX, "dtx"},
	};

	auto ii = type_2_ext_.find(iType);
	if (ii == type_2_ext_.end())
		return "";
	else
		return ii->second;
}

RESOURCE_TYPE GetSongResourceType(SONGTYPE iType)
{
	switch (iType)
	{
	case SONGTYPE::BMS:
	case SONGTYPE::BMSON:
	case SONGTYPE::SM:
		return RESOURCE_TYPE::FOLDER;
	case SONGTYPE::OSU:
	case SONGTYPE::OJM:
	case SONGTYPE::BMSARCH:
	case SONGTYPE::DTX:
		return RESOURCE_TYPE::ARCHIVE;
	case SONGTYPE::VOS:
		return RESOURCE_TYPE::VOSBINARY;
	}
	return RESOURCE_TYPE::NONE;
}


}