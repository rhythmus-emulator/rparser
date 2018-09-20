/*
 * by @lazykuna, MIT License.
 */

#ifndef RUTIL_BASIC_H
#define RUTIL_BASIC_H


#ifndef WIN32
 //
 // in case of linux, then USE_ICONV is automatically defined.
 // if you want to use iconv in windows, then define USE_ICONV.
 //
#define USE_ICONV
 // to manage binaries easier
#define ZIP_STATIC
#endif


#define DEFAULT_RESOLUTION_SIZE 192

#include <stdio.h>
#include <string>
#include <vector>
#include <map>

#include <assert.h>
#define ASSERT assert

#define R_CP_932 932	// JAP
#define R_CP_949 949	// KOR
#define R_CP_UTF8 1252


// pre-declaration for zip_t
struct zip;
typedef zip zip_t;


namespace rutil {

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
int EncodeFromWStr(const std::wstring& s, std::string& sOut, int to_codepage);
#endif


// I/O related
#ifdef WIN32
FILE* fopen_utf8(const char* fname, const char* mode);
int printf_utf8(const char* fmt, ...);
#else
#define fopen fopen_utf8
#define printf printf_utf8
#endif
bool RemoveFile(const char* fpath);
bool RenameFile(const char* prev_path, const char* new_path);
bool CreateFolder(const char* path);



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
bool CheckExtension(const std::string& path, const std::string &filter);
std::string ChangeExtension(const std::string& path, const std::string &new_ext);
std::string GetPathJoin(const std::string& s1, const std::string s2);



// 
uint32_t ReadLE32(const unsigned char* p);



// Directory / Archive related

bool IsDirectory(const std::string& path);
bool CreateDirectory(const std::string& path);
#define CreateDirectory_RPARSER CreateDirectory
// <path, isfile>
typedef std::vector<std::pair<std::string, int>> DirFileList;
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive=100);

class FileData {
public:
    std::string fn;
    uint8_t *p;
    uint32_t m_iLen;
    uint32_t m_iPos;

public:
    FileData();
    FileData(uint8_t *p, uint32_t iLen);
	~FileData();
	FileData(const std::string& fn);

    std::string GetFilename();
    uint32_t GetFileSize();
    uint32_t GetPos();
    void SetPos(uint32_t iPos);
    uint8_t* GetPtr();

    // reading
    int Seek(uint32_t p, int mode);
    int SeekSet(uint32_t p = 0);
    int SeekCur(uint32_t p);
    int SeekEnd(uint32_t p = 0);
    uint32_t ReadLE32();
    uint32_t Read(uint8_t *out, uint32_t len);
    bool IsEOF();
};

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
    virtual int Read(FileData &fd) const = 0;
    virtual int Read(const std::string fpath, FileData &fd) const;
    virtual int Flush() = 0;
    virtual int ReadFiles(std::vector<FileData>& fd) const = 0;
    virtual int Close() = 0;
    virtual int Create(const std::string &path) = 0;

    // @description read file with smart-file-finding routine
    bool ReadSmart(FileData &fd) const;

    std::vector<std::string> GetFileEntries(const char* ext_filter=0);   // sep with semicolon
    std::vector<std::string> GetFolderEntries();
};

class BasicDirectory : public IDirectory {
    int m_iRecursiveDepth;          // directory recursive depth (in BasicFolder)
public:
    int Open(const std::string &path);
    int Write(const FileData &fd);
    int Read(FileData &fd) const;
    int ReadFiles(std::vector<FileData>& fd) const;
    int Flush();
    int Create(const std::string& path);
    int Close();
    void SetRecursiveDepth(int iRecursiveDepth);
    static int Test(const std::string &path);

    BasicDirectory() : m_iRecursiveDepth(100) {}
};

#ifdef USE_ZLIB
class ArchiveDirectory : public IDirectory {
private:
    int m_iCodepage;    // encoding of original archive file
    zip_t *m_Archive;
public:
    int Open(const std::string &path);
    int Write(const FileData &fd);
    int Read(FileData &fd) const;
    int ReadFiles(std::vector<FileData>& fd) const;
    int Flush();
    int Create(const std::string& path);
    int Close();
    static int Test(const std::string &path);
    ArchiveDirectory() : m_Archive(0), m_iCodepage(0) {}
    ~ArchiveDirectory() { Close(); }
    void SetCodepage(int iCodepage);
};
#endif



// BMS-util related

// @description only change 2 character; 00-ZZ to integer
int atoi_bms(const char* p, int length=0xF);
// 00-FF
int atoi_bms16(const char* p, int length=2);


};

#endif