/*
 * by @lazykuna, MIT License.
 */
 
#ifndef RPARSER_SONG_H
#define RPARSER_SONG_H

#include "Chart.h"
#include "util.h"
#include <algorithm>

namespace rparser {

enum class SONGTYPE {
	UNKNOWN,
	BMS,
	BMSON,
	OSU,
	VOS,
};

char *SongErrorCode[] = {
	"No Error",
	"No Save file format (or unsupported file format) specified",
	"The archive file isn't valid or supported one.",
	"This archive file has password.",
};

/*
 * @description
 * Song contains all charts using same(or similar) resources.
 * So all charts must be in same folder.
 */
class Song {
private:
    // @description Song object responsive for removing all chart datas when destroyed.
    std::vector<Chart*> m_Charts;
	// @description Is song is archive or folder?
	bool m_IsSongDir;
	// @description song file's path
	std::string m_SongDir;
	// @description song file's name, if it's archive.
	std::string m_SongFilename;
	// @description chart file's base directory from song's directory
	// (ex) if abc.zip/abc/abc.bms, then 'abc' is base directory.
	std::string m_SongBaseDir;
	// @description current song's file format
	SONGTYPE m_SongType;

	// @description zlib handle in case of archive
	void* m_zlibHandle;
	// @description error code
	int m_errorcode;
public:
    // @description
    // register chart to Song array.
    // registered chart will be automatically removed from memory when Song object is deleted.
    void RegisterChart(Chart* c);

    // @description
    // delete chart from registered array.
    // you must release object by yourself.
    void DeleteChart(const Chart* c);

	// @description
	// save song file
	bool SaveSong(const std::string &path, SONGTYPE songtype = SONGTYPE::UNKNOWN);
	bool SaveSongAsBms();
	bool SaveSongAsBmsOn();
	bool SaveSongAsOsu();
	//bool SaveSongAsVos();	// - will not be implemented

	// @description
	// load song file
	bool LoadSong(const std::string &path, SONGTYPE songtype = SONGTYPE::UNKNOWN);
	bool LoadSongFromBms();
	bool LoadSongFromBmsOn();
	bool LoadSongFromOsu();
	bool LoadSongFromVos();

	// utilities
    void GetCharts(std::vector<Chart*>& charts);
    int GetChartCount();
	int GetError();
	const char* GetErrorStr();
	// @description
	// read raw resource binary using chart relative path
	// (should free binary yourself)
	char* ReadResourceFile(const std::string& rel_path);

	void Clear();
    ~Song();

private:
	// @description
	// initalize song loading, like retaining zlib handle or finding base directory
	bool LoadSongInit();
};

}

#endif