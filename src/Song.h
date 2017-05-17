/*
 * by @lazykuna, MIT License.
 */
 
#ifndef RPARSER_SONG_H
#define RPARSER_SONG_H

#include "MetaData.h"

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

struct FileData {
	std::string fn;
	char *p;
	int iLen;
	bool isNewFile;
};

/* comment: may need to make 'save' module? */

/*
 * @description
 * Song contains all charts using same(or similar) resources.
 * So all charts must be in same folder.
 */
class SongResource;
class Song {
private:
    // @description Song object responsive for removing all chart datas when destroyed.
    std::vector<Chart*> m_vCharts;

	// @description in case of song has metadata
	SongMetaData m_SongMeta;

	// @description loaded binaries (in case of archive, will be saved together when song is saved.)
	std::vector<FileData> m_vFiles;

	// not saved; just indicates current opening state.
	SONGTYPE m_SongType;			// @description current song's file format
	std::string sSongPath;			// current song archive or directory path
	//bool bIsArchive;				// Is current song file loaded/saved in archive?
	struct archive * pArchive;		// Used in case of archive file (is-archive?)
	bool bFastLoad;					// Enabled when OnlyChartLoad. chart won't be cached in m_vFiles, and you can't save file.
	int iErrorcode;					// @description error code
	bool bLoading;					// @description is song loading?
	double dProgress;				// @description loading progress of file
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
	// * SaveAll() method save all m_vFile together, with all charts.
	virtual bool SaveAll();
	// * SaveAll() method save all m_vFile together, with specified charts.
	virtual bool SaveChart(const Chart* c);


	// @description load song
	// default: pass folder / archive (default)
	// may can pass single chart:
	// - in that case, LoadSongMetadata() is called, 
	//   and automatically LoadChart() is ONLY once called for that only chart.
	bool Load(const std::string &path,
		SONGTYPE songtype = SONGTYPE::UNKNOWN,
		bool onlychartread=false);
	// @description Only load song metadata file (in case of osu)
	bool LoadSongMetadata(const std::string &path, SONGTYPE songtype = SONGTYPE::UNKNOWN);
	// @description in case of loading(importing) single chart into m_vCharts
	bool LoadChart(const std::string& path, SONGTYPE songtype = SONGTYPE::UNKNOWN);


	// utilities
    void GetCharts(std::vector<Chart*>& charts);
    int GetChartCount();
	int GetError();
	const char* GetErrorStr();
	bool IsLoading();
	double GetLoadingProgress();


	// resource part
	FileData* GetResource(const std::string& fn);
	bool UpdateResource(const FileData& d);				// replace(update) or insert FileData in m_vFiles
	bool DeleteResource(const std::string& fn);
	void GetResourceList(const std::vector<std::string>& v);
	void ClearResource();								// clear all loaded resource file from memory pool


	// @description clear all current song metadata & resource
	void Clear();
	Song();
    ~Song();
private:
	// read from directory
	bool ReadDirectory(bool onlychartread=false);
	// write into directory (only write in case of FileData.isNewFile is true)
	bool WriteDirectory();
	// read files into m_vFiles.
	bool ReadArchiveFile(bool onlychartread=false);
	// write to archive in m_vFiles.
	bool WriteArchiveFile();
};


/*
 * @description
 * Some type(like osu ...) of song file has metadata in song, not in chart.
 * In that case, we use songmetadata to keep track of such format.
 * This metadata will be ignored in case of unsupported format (like bms)
 */
class Metadata;
class SongMetaData : public Metadata {
	// @description should only show at extra stage
	bool bExtraStage;
};


SONGTYPE DetectSongTypeExtension(const std::string& fname);
SONGTYPE DetectSongType(const std::string& path);

}

#endif