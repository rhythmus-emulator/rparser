/*
 * by @lazykuna, MIT License.
 */

#ifndef RUTIL_BASIC_H
#define RUTIL_BASIC_H

#ifdef _WIN32
# ifndef WIN32
#  define WIN32
# endif
#endif

#ifndef WIN32
 //
 // in case of linux, then USE_ICONV is automatically defined.
 // if you want to use iconv in windows, then define USE_ICONV.
 //
#define USE_ICONV
 // to manage binaries easier
//#define ZIP_STATIC
#endif


#define DEFAULT_RESOLUTION_SIZE 192

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <random>

// pre-declaration for zip_t
struct zip;
typedef zip zip_t;


namespace rutil {

enum EncodingTypes
{
  E_EUC_KR = 949,
  E_SHIFT_JIS = 932,
  E_UTF8 = 1252
};

// encoding related

// @description
// this returns UTF-8 string using specified encoding.
// if no encoding found then it returns 0 with no string changed.
// if from_codepage=0, then attempt encoding Shift_JIS / CP949
std::string ConvertEncoding(const std::string &s, int to_codepage, int from_codepage = 0);
std::string ConvertEncodingToUTF8(const std::string &s, int from_codepage = 0);
int DetectEncoding(const std::string &s);
#ifdef WIN32
int DecodeToWStr(const std::string& s, std::wstring& sOut, int from_codepage);
int EncodeFromWStr(const std::wstring& s, std::string& sOut, int to_codepage);
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
std::string upper(const std::string& s);
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



// I/O, Archive related

class FileData {
public:
  std::string fn;
  uint8_t *p;
  uint32_t len;
  uint32_t pos;

public:
  FileData();
  FileData(uint8_t *p, uint32_t iLen);
  FileData(const std::string& fn);

  std::string GetFilename() const;
  uint32_t GetFileSize() const;
  uint32_t GetPos() const;
  void SetPos(uint32_t iPos);
  const uint8_t* GetPtr() const;
  uint8_t* GetPtr();

  // reading
  int Seek(uint32_t p, int mode);
  int SeekSet(uint32_t p = 0);
  int SeekCur(uint32_t p);
  int SeekEnd(uint32_t p = 0);
  uint32_t ReadLE32();
  uint32_t Read(uint8_t *out, uint32_t len);
  bool IsEOF();
  bool IsEmpty();
};

FILE* fopen_utf8(const char* fname, const char* mode);
FILE* fopen_utf8(const std::string& fname, const std::string& mode);
int printf_utf8(const char* fmt, ...);

bool IsAccessable(const std::string& path);
bool IsFile(const std::string& path);
bool DeleteFile(const std::string& fpath);
bool Rename(const std::string& prev_path, const std::string& new_path);
std::string ReadFileText(const std::string& path);
FileData ReadFileData(const std::string& path);
bool WriteFileData(const FileData& fd);

bool IsDirectory(const std::string& path);
bool CreateDirectory(const std::string& path);
bool DeleteDirectory(const std::string& path);

#define CreateDirectory_RPARSER CreateDirectory
// <path, isfile>
typedef std::vector<std::pair<std::string, int>> DirFileList;
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive=100);


// if succeed, return true, and write md5 hash to char* out;
// else, return false.
// (requires MD5_DIGEST_LENGTH; 16)
bool md5(const void* p, int iLen, char* out);
// same to md5; returns 32byte string (%02x formatted)
bool md5_str(const void* p, int iLen, char* out);
std::string md5_str(const void* p, int iLen);

char *itoa(int value, char *str, int base);
char *gcvt(double value, int digits, char *string);


class Random
{
public:
  Random();
  void SetSeedByTime();
  void SetSeed(uint32_t seed);
  uint32_t GetSeed();
  virtual int32_t Next();
  virtual double NextDouble();
  virtual int32_t Next(int32_t min, int32_t max);
  virtual double NextDouble(double min, double max);
private:
  std::mt19937 randomgenerator_;
  std::uniform_real_distribution<double_t> dist_real_;
  std::uniform_int_distribution<int32_t> dist_int_;
  uint32_t seed_;
};

};

#endif
