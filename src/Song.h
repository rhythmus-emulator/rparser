/*
 * by @lazykuna, MIT License.
 */
 
#ifndef RPARSER_SONG_H
#define RPARSER_SONG_H

#include "MetaData.h"

// you may want to support archive library.
// (requires: zlip, libzip)
#define RPARSER_USE_LIBZIP
#ifdef RPARSER_USE_LIBZIP
#include <zip.h>
#endif

#include "util.h"
#include "Chart.h"
#include <algorithm>

namespace rparser {

char *SongErrorCode[] = {
    "No Error", // 0
    "No Save file format (or unsupported file format) specified",   // 1
    "The archive file isn't valid or supported one.",
    "Corrupted archive file (or might have password)",
    "This song archive file has no chart.",
    "Unsupported song file format.",    // 5
};

struct FileData {
    std::string fn;
    unsigned char *p;
    int iLen;
};

// TODO: move it to general IO util
class IDirectory {
protected:
    int error;
    std::string m_sPath;
    std::vector<std::string> m_vFilename;
    std::vector<std::string> m_vFolder;     // folder of direct ancestor
public:
    SongIO(Song *s): s(s), error(0) {};
    virtual int Open(const std::string &path) = 0;
    virtual int Write(const FileData &fd) = 0;
    virtual int Read(FileData &fd) = 0;
    virtual int Flush() = 0;
    virtual int ReadFiles() = 0;
    virtual int Close() = 0;
    virtual int Create(const std::string &path) = 0;
};

class BasicDirectory : public IDirectory {
public:
    int Open(const std::string &path);
    int Write(const FileData &fd);
    int Read(FileData &fd);
    int ReadFiles(std::vector<FileData>& fd);
    int Flush();
    int Create(const std::string& path);
    int Close();
    static int Test(const std::string &path);
};

class ArchiveDirectory : public IDirectory {
private:
    std::string m_sEncoding;    // encoding of original archive file
    zip_t *m_Archive;
public:
    int Open(const std::string &path);
    int Write(const FileData &fd);
    int Read(FileData &fd);
    int ReadFiles(std::vector<FileData>& fd);
    int Flush();
    int Create(const std::string& path);
    int Close();
    static int Test(const std::string &path);
    SongArchiveIO() : m_Archive(0) {}
    ~SongArchiveIO() { Close(); }
    void SetEncoding(const std::string& sEncoding);
};

/*
 * @description
 * Song contains all charts using same(or similar) resources.
 * So all charts must be in same folder.
 */
class SongMetaData;
class Song {
private:
    // @description Song object responsive for removing all chart datas when destroyed.
    std::vector<Chart*> m_vCharts;

    // @description in case of song has metadata
    SongMetaData m_SongMeta;

    // not saved; just indicates current opening state.
    std::string m_sPath;
    CHARTTYPE m_charttype;          // Detected chart format of this song
    bool bIsArchive;                // Is current song file loaded/saved in archive?
    bool bFastLoad;                 // Enabled when OnlyChartLoad. chart won't be cached in m_vFiles, and you can't save file.
    int iErrorcode;                 // @description error code

    IDirectory *m_pDir;             // @description Song IO
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
    bool Open(const std::string &path,
        SONGTYPE songtype = SONGTYPE::UNKNOWN);
    // @description Only load song metadata file (in case of osu)
    bool LoadSongMetadata(const std::string &path, SONGTYPE songtype = SONGTYPE::UNKNOWN);
    // @description in case of loading(importing) single chart into m_vCharts
    bool LoadChart(const std::string& relpath, SONGTYPE songtype = SONGTYPE::UNKNOWN);


    // utilities
    void GetCharts(std::vector<Chart*>& charts);
    int GetChartCount();
    int GetError();
    const char* GetErrorStr();


    // @description clear all current song metadata & resource, close directory.
    void Close();
    Song();
    ~Song();
private:
    bool OpenDirectory();
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

SONGTYPE rparser::TestSongTypeExtension(const std::string & fname);

}

#endif
