/*
 * by @lazykuna, MIT License.
 *
 * Song loader / saver, representing rparser library.
 * This library has functions as follows:
 * - Load and Save
 * - Edit
 * - Digest (in mixable form)
 * - Resource managing (binary form)
 */
 
#ifndef RPARSER_SONG_H
#define RPARSER_SONG_H

#include "Chart.h"
#include "Resource.h"
#include "MetaData.h"
#include "Error.h"

#include <algorithm>
#include <sstream>
#include "rutil.h"

namespace rparser {

// MUST be in same order with Chart::CHARTTYPE
enum class SONGTYPE {
	NONE = 0,
	BMS,
	BMSON,
	OSU,
	VOS,
	SM,
	DTX,
	OJM,
	BMSARCH,
};

class Chart;

/*
 * \brief
 * Contains song chart, metadata and all resource(image, sound).
 *
 * \param resource_
 * \param charts_
 * \param charts_old_filename_ used whether chart filename has changed when saving
 * \param timingdata_global_ in case of a song has global timingdata
 * \param songmeta_global_ in case of a song has global metadata
 * \param songtype_ (not saved) detected chart format of this song
 * \param error_ last error status
 */
class Song {
private:
	struct ChartFile
	{
		Chart* c;
		std::string new_filename;
		std::string old_filename;
		const char hash[32];
		bool is_dirty;
	};

	Resource resource_;
    std::vector<ChartFile> charts_;
	TimingObjVec *tobjs_global_;
    MetaData *metadata_global_;
	SONGTYPE songtype_;
	ERROR error_;
public:
    /* \brief register Chart to Song vector. */
    void RegisterChart(Chart* c, const std::string& filename);

    /* \brief Delete chart from Song object.
	 * \warning You must release Chart object by yourself. */
    bool DeleteChart(const Chart* c);

	bool RenameChart(const Chart* c, const std::string& newfilename);

	void GetCharts(std::vector<Chart*>& charts);

	/* \brief
	 * Required in special case which metadata is separated with chart file
	 * e.g. osu file format. */
	bool LoadMetadata();
    bool SaveMetadata();

    const char* GetErrorStr() const;
	Resource* GetResource();



    // @description
	// Read song file, and load charts and other metadata.
    // Acceptable file path: folder / archive / raw file(not a single chart; special case)
	bool Open(const std::string &path, bool onlyreadchart=false, SONGTYPE songtype = SONGTYPE::NONE);
	// @description
	// save all changes into song file, if available.
	bool Save();
	// @description
	// clear all current song metadata & resource, close directory.
    bool Close(bool save=false);
	bool Rename(const std::string& newPath);

	void SetPath(const std::string& path);
	const std::string GetPath() const;

	// @description
	// Change(convert) song type to other (including file extension)
	bool ChangeSongType(SONGTYPE songtype);

	virtual std::string toString() const;
    Song();
    ~Song();
private:
	static const std::string total_readable_ext_;
	static const std::string gen_readable_ext_();

	// in case of need ...
	std::string errormsg_detailed_;
};

const char* GetChartExtension(SONGTYPE iType);
const char* GetSongExtension(SONGTYPE iType);
RESOURCE_TYPE GetSongResourceType(SONGTYPE iType);

}

#endif
