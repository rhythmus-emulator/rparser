/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_UTIL_H
#define RPARSER_UTIL_H

#define DEFAULT_RESOLUTION_SIZE 192

#include <stdio.h>
#include <string>

#include <assert.h>
#define ASSERT assert

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
int AttemptEncoding(std::string &s, int from_codepage=0);
std::string DecodeTo(std::string &s, int to_codepage);
#ifndef USE_ICONV	//#ifdef WIN32
FILE* fopen_utf8(const wchar_t* fname, const char* mode);
FILE* fopen_utf8(const char* fname, const char* mode);
std::wstring DecodeUTF8ToWStr(const std::string& s);
std::wstring DecodeUTF8ToWStr(char *p, int iLen);
std::string DecodeWStrToUTF8(const std::wstring& s);
int printf_utf8();
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

void lower(std::string& s);
bool EndsWith(const std::string& s1, const std::string& s2);
// @description tidy pathname in case of using irregular separator
std::string CleanPath(const std::string& path);
// @description get absolute path
std::string GetAbsolutePath(const std::string& path);

std::string GetDirectoryname(const std::string& path);
std::string GetFilename(const std::string& path);
std::string GetExtension(const std::string& path);
bool IsDirectory(const std::string& path);



// BMS-util related

// @description only change 2 character; 00-ZZ to integer
int atoi_bms(const char* p);
// 00-FF
int atoi_bms16(const char* p);


};

#endif