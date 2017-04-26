/*
 * by @lazykuna, MIT License.
 */
 
#ifndef RPARSER_SONG_H
#define RPARSER_SONG_H

// you may want to support archive library.
// (requires: zlip, libarchive)
#define RPARSER_USE_ZIP
#ifdef RPARSER_USE_ZIP
#include <archive.h>
#include <archive_entry.h>
#endif

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
	SM,
	DTX,
	OJM,
};

char *SongErrorCode[] = {
	"No Error",	// 0
	"No Save file format (or unsupported file format) specified",	// 1
	"The archive file isn't valid or supported one.",
	"Corrupted archive file (or might have password)",
	"This song archive file has no chart.",
	"Unsupported song file format.",	// 5
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

	// @description archive handle in case of archive
	struct archive * m_archive;
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
	// TODO: saving specific chart?
	bool SaveSong(const std::string &path, SONGTYPE songtype = SONGTYPE::UNKNOWN);
	bool SaveSongAsBms(const std::string &path);
	bool SaveSongAsBmsOn(const std::string &path);
	bool SaveSongAsOsu(const std::string &path);
	//bool SaveSongAsVos();	// - will not be implemented

	// @description
	// load song file
	bool LoadSong(const std::string &path, SONGTYPE songtype = SONGTYPE::UNKNOWN);
	bool LoadSongFromBms(const std::string &path);
	bool LoadSongFromBmsOn(const std::string &path);
	bool LoadSongFromOsu(const std::string &path);
	bool LoadSongFromVos(const std::string &path);

	// @description
	// in case of loading(importing) single chart ...
	// (won't clear previous chart data)
	bool LoadChart(const std::string& path, SONGTYPE songtype = SONGTYPE::UNKNOWN);

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
	bool LoadFromArchive(const std::string &path);
	bool PrepareArchiveRead(const std::string& path);
	void ClearArchiveRead();
	bool PrepareArchiveWrite(const std::string& path);
	void ClearArchiveWrite();
};

SONGTYPE DetectSongTypeExtension(const std::string& fname);
SONGTYPE DetectSongType(const std::string& path);

}

#endif