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


namespace rparser
{

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

bool Song::Save()
{
	// Only update dirty charts
	for (Chart *c : charts_)
	{
		if (c->IsDirty())
		{
			Resource::BinaryData data;
			if (!c->MakeBinaryData(&data))
			{
				errormsg_ = RPARSER_SONG_ERROR_SAVE_CHART;
				return false;
			}
			resource_.AllocateBinary(c->GetFilePath(), data.p, data.len, true, false);
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

void Song::SetSongType(SONGTYPE songtype)
{
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
	ss << "Song type: " << GetSongTypeExtension(songtype_) << std::endl;
	ss << "Chart count: " << charts_.size() << std::endl << std::endl;
	for (auto c : charts_)
	{
		ss << c->toString() << std::endl << std::endl;
	}
	return ss.str();
}

std::string GetSongTypeExtension(SONGTYPE iType)
{
	static const std::map<SONGTYPE, const char*> type_2_str_ = {
		{SONGTYPE::BMS, "bms"},
		{SONGTYPE::BMSON, "bmson"},
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


}