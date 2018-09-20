#include "Song.h"
#include "MetaData.h"
#include "Chart.h"
#include "ChartLoader.h"
#include "ChartWriter.h"
#include <list>

using namespace rutil;

#define RPARSER_SONG_ERROR_FILE				"Unable to read song file."
#define RPARSER_SONG_ERROR_NOCHART			"No chart found in song file."
#define RPARSER_SONG_ERROR_CHART			"Invalid chart file."
#define RPARSER_SONG_ERROR_SAVE_CHART		"Failed to generate chart file."
#define RPARSER_SONG_ERROR_SAVE_METADATA	"Failed to save metadata."
#define RPARSER_SONG_ERROR_SAVE_FILE		"Failed to save file (general I/O error)"


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



// ------ class Song ------

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

Song::Song()
	: songtype_(SONGTYPE::NONE), errormsg_(0)
{
}

Song::~Song()
{
	Close();
}

void Song::RegisterChart(Chart * c)
{
	charts_.push_back(c);
}

bool Song::DeleteChart(const Chart * c_)
{
	for (auto p = charts_.begin(); p != charts_.end(); ++p) {
		if (*p == c_) {
			charts_.erase(p);
			return true;
		}
	}
	return false;
}


bool Song::Open(const std::string & path, bool fastread, SONGTYPE songtype)
{
	// initialize
	Close();
	errormsg_ = 0;
	songtype_ = songtype;

	// Read given path first.
	// If required, only read chart files.
	const char * fastread_ext = total_readable_ext_.c_str();
	if (!fastread) fastread_ext = 0;
	if (!resource_.Open(path.c_str(), 0, fastread_ext))
	{
		errormsg_ = RPARSER_SONG_ERROR_FILE;
		songtype_ = SONGTYPE::NONE;
		return false;
	}

	// Filter out file from resource, which is feasible to read.
	std::map<std::string, const Resource::BinaryData*> chart_files;
	resource_.FilterFiles(total_readable_ext_.c_str(), chart_files);
	if (chart_files.size() == 0)
	{
		errormsg_ = RPARSER_SONG_ERROR_NOCHART;
		songtype_ == SONGTYPE::NONE;
		return false;
	}

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
		// if resource is archive file and SONGTYPE::BMS,
		// then change type to SONGTYPE::BMSARCH
		if (songtype_ == SONGTYPE::NONE)
		{
			errormsg_ = RPARSER_SONG_ERROR_NOCHART;
			return false;
		}
	}

	// If necessary, load song metadata.
	// Should load first than charts, as some chart may require metadata.
	LoadMetadata();

	// Attempt to read chart.
	for (auto ii : chart_files)
	{
		Chart *c = new Chart();
		c->SetFilePath(ii.first);
		if (!c->Load(ii.second->p, ii.second->len))
		{
			// Error might be occured during chart loading,
			// But won't stop loading as there *might* be 
			// So, just skip the wrong chart file and make log.
			errormsg_ = RPARSER_SONG_ERROR_CHART;
			delete c;
			continue;
		}
		RegisterChart(c);
	}

	return true;
}

#define SAVE_CHART_CODE\
			if (c->IsDirty())						\
			{										\
				Resource::BinaryData data;			\
				if (!c->MakeBinaryData(&data))		\
				{									\
					errormsg_ = RPARSER_SONG_ERROR_SAVE_CHART;\
					return false;					\
				}									\
				resource_.AllocateBinary(c->GetFilePath(), data.p, data.len, true, false);\
			}

bool Song::Save()
{
	// Only update dirty charts
	// (no need to update binary data in case of binary type)
	if (resource_.GetResourceType() != RESOURCE_TYPE::BINARY)
	{
		for (Chart *c : charts_)
		{
			SAVE_CHART_CODE;
		}
	}

	// save metadata, in case of need.
	if (!SaveMetadata())
	{
		errormsg_ = RPARSER_SONG_ERROR_SAVE_METADATA;
		return false;
	}

	// flush (save to real file)
	resource_.Flush();
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
	for (Chart *c : charts_)
	{
		delete c;
	}
	charts_.clear();
	resource_.Unload(false);
	errormsg_ = 0;
	songtype_ = SONGTYPE::NONE;
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
	// make it easy
	if (songtype == songtype_)
		return true;

	// keep consistency for all charts
	// and need to rename extension.
	for (Chart *c : charts_)
	{
		c->SetSongType(songtype);
		c->RenameExtension(GetChartExtension(songtype));
	}

	// If original type was binary file (e.g. VOS),
	// There might be no binary data stored in chart.
	// To prevent that case, save binary data here.
	SAVE_CHART_CODE;

	// Change Resource extension first and save.
	resource_.SetExtension(GetSongExtension(songtype));
	resource_.SetResourceType(GetSongResourceType(songtype));
	if (!resource_.Flush())
	{
		errormsg_ = RPARSER_SONG_ERROR_SAVE_FILE;
		return false;
	}

	songtype_ = songtype;
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

const std::vector<Chart*>* Song::GetCharts() const
{
	return &charts_;
}

void Song::GetCharts(std::vector<Chart*>& charts)
{
	charts = charts_;
}

const char* Song::GetErrorStr() const
{
	return errormsg_;
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
	for (auto c : charts_)
	{
		ss << c->toString() << std::endl << std::endl;
	}
	return ss.str();
}

const char* GetChartExtension(SONGTYPE iType)
{
	static const std::map<SONGTYPE, const char*> type_2_str_ = {
		{SONGTYPE::BMS, "bms"},
		{SONGTYPE::BMSON, "bmson"},
		{SONGTYPE::BMSARCH, "bms"},
		{SONGTYPE::VOS, "vos"},
		{SONGTYPE::OSU, "osu"},
		{SONGTYPE::SM, "sm"},
		{SONGTYPE::OJM, "ojm"},
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
		return RESOURCE_TYPE::BINARY;
	}
	return RESOURCE_TYPE::NONE;
}


}