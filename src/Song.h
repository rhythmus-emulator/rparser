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

#include "Resource.h"
#include "MetaData.h"

#include <algorithm>
#include "rutil.h"

namespace rparser {


enum class SONGTYPE {
	NONE,
	BMS,
	BMSON,
	OSU,
	VOS,
	SM,
	DTX,
	OJM,
};

class Chart;

/*
 * @description
 * Song contains all charts using same(or similar) resources.
 * So all charts must be in same folder.
 */

class Song {
private:
	// @description
	// Responsive for file load and managing resources related to song.
	Resource resource_;

    // @description
	// Chart object container.
    std::vector<Chart*> charts_;

    // @description
	// in case of song has metadata
	// metadata: a general data, used for every song file
    MetaData songmeta_;

    // not saved; just indicates current opening state.
	SONGTYPE songtype_;            // Detected chart format of this song
	const char* errormsg_;
public:
    // @description
    // register Chart to Song vector.
    void RegisterChart(Chart* c);

    // @description
    // Delete Chart from registered array.
	// Returns true if succeed, false if chart ptr not found.
    // * You must release Chart object by yourself.
    bool DeleteChart(const Chart* c);

	// @description
	// Get all charts, if required.
	const std::vector<Chart*>* GetCharts() const;
	void GetCharts(std::vector<Chart*>& charts);

	// @description
	// Required in special case, like
	bool LoadMetadata();
    bool SaveMetadata();



    // utilities
    const char* GetErrorStr() const;
	Resource* GetResource();



    // @description
	// Read song file, and load charts and other metadata.
    // Acceptable file path: folder / archive / raw file(not a single chart; special case)
	bool Open(const std::string &path, bool fastread=false, SONGTYPE songtype = SONGTYPE::NONE);
	// @description
	// save all changes into song file, if available.
	bool Save();
	// @description
	// clear all current song metadata & resource, close directory.
    bool Close(bool save=false);

	void SetPath(const std::string& path);
	void SetSongType(SONGTYPE songtype);

	std::string toString() const;
    Song();
    ~Song();
private:
	const std::string total_readable_ext_;
	const std::string gen_readable_ext_();
	std::string path_;

	// in case of need ...
	std::string errormsg_detailed_;
};



std::string GetSongTypeExtension(SONGTYPE iType);

}

#endif
