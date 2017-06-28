/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_UTIL_H
#define RPARSER_UTIL_H

#define DEFAULT_RESOLUTION_SIZE 192

#include <stdio.h>
#include <string>
#include <vector>

#define ZIP_STATIC
#include <zip.h>

#include <assert.h>
#define ASSERT assert


#define R_CP_932 932	// JAP
#define R_CP_949 949	// KOR
#define R_CP_UTF8 1252


//
// in case of linux, then USE_ICONV is automatically defined.
// if you want to use iconv in windows, then define USE_ICONV.
//
#ifndef WIN32
#define USE_ICONV
#endif

namespace rparser {

// encoding related

// @description
// this returns UTF-8 string using specified encoding.
// if no encoding found then it returns 0 with no string changed.
// if from_codepage=0, then attempt encoding Shift_JIS / CP949
int AttemptEncoding(std::string &s, int to_codepage, int from_codepage);
int AttemptEncoding(std::string &s, int from_codepage=0);
int DecodeTo(std::string &s, int to_codepage);
#ifdef WIN32
int DecodeToWStr(const std::string& s, std::wstring& sOut, int from_codepage);
int EncodeFromWStr(const std::wstring& sOut, std::string& s, int to_codepage);
FILE* fopen_utf8(const char* fname, const char* mode);
int printf_utf8(const char* fmt, ...);
#else
#define fopen fopen_utf8
#define printf printf_utf8
#endif



// seed related (depreciated)

// @description
// set global seed (mostly used for BMS parsing)
void SetSeed(int seed);
// @description
// get seed
int GetSeed();



// string related

std::string lower(const std::string& s);
std::string trim(const std::string &s);
int split(const std::string& str, const char sep, std::vector<std::string>& vsOut);
int split(const std::string& str, const char sep, std::string &s1, std::string &s2);
#define IN_ARRAY(v,o) (std::find((v).begin(), (v).end(), (o)) != (v).end())
#define IN_MAP(v,o) ((v).find(o) != (v).end())
bool endsWith(const std::string& s1, const std::string& s2, bool casesensitive=true);
// @description tidy pathname in case of using irregular separator
std::string CleanPath(const std::string& path);
std::string GetDirectory(const std::string& path);
std::string GetParentDirectory(const std::string& path);
std::string GetFilename(const std::string& path);
std::string GetExtension(const std::string& path, std::string *sOutName=0);
std::string GetPathJoin(const std::string& s1, const std::string s2);



// Directory / Archive related

bool IsDirectory(const std::string& path);
bool CreateDirectory(const std::string& path);
#define CreateDirectory_RPARSER CreateDirectory
// <path, isfile>
typedef std::vector<std::pair<std::string, int>> DirFileList;
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive=100);

struct FileData {
    std::string fn;
    unsigned char *p;
    long long iLen;
};
void DeleteFileData(FileData& fd);

class IDirectory {
protected:
    int error;
    std::string m_sPath;
    std::vector<std::string> m_vFilename;
    std::vector<std::string> m_vFolder;     // folder of direct ancestor
public:
    IDirectory(): error(0) {};
    virtual int Open(const std::string &path) = 0;
    virtual int Write(const FileData &fd) = 0;
    virtual int Read(FileData &fd) = 0;
    virtual int Read(const std::string fpath, FileData &fd);
    virtual int Flush() = 0;
    virtual int ReadFiles(std::vector<FileData>& fd) = 0;
    virtual int Close() = 0;
    virtual int Create(const std::string &path) = 0;

    // @description read file with smart-file-finding routine
    bool ReadSmart(FileData &fd);

    std::vector<std::string> GetFileEntries(const char* ext_filter=0);   // sep with semicolon
    std::vector<std::string> GetFolderEntries();
};

class BasicDirectory : public IDirectory {
    int m_iRecursiveDepth;          // directory recursive depth (in BasicFolder)
public:
    int Open(const std::string &path);
    int Write(const FileData &fd);
    int Read(FileData &fd);
    int ReadFiles(std::vector<FileData>& fd);
    int Flush();
    int Create(const std::string& path);
    int Close();
    void SetRecursiveDepth(int iRecursiveDepth);
    static int Test(const std::string &path);

    BasicDirectory() : m_iRecursiveDepth(100) {}
};

class ArchiveDirectory : public IDirectory {
private:
    int m_iCodepage;    // encoding of original archive file
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
    ArchiveDirectory() : m_Archive(0) {}
    ~ArchiveDirectory() { Close(); }
    void SetCodepage(int iCodepage);
};



// BMS-util related

// @description only change 2 character; 00-ZZ to integer
int atoi_bms(const char* p);
// 00-FF
int atoi_bms16(const char* p, int length=2);


};

#endif