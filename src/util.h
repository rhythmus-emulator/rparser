/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_UTIL_H
#define RPARSER_UTIL_H

#include <stdio.h>
#include <string>

namespace rparser {

// @description
// actually, it only detectes Shift_JIS / CP949 / UTF-8 encodings.
std::string AutoDetectEncoding(char *data, int iLen);
std::string DecodeToUTF8(char *p, int iLen, char *encoding_from);
// @description we may need this function in case of saving BMS in Shift_JIS format.
std::string EncodeToJIS(char *p_utf8, int iLen);


#ifdef WIN32
FILE* fopen_utf8(const wchar_t* fname, const char* mode);
FILE* fopen_utf8(const char* fname, const char* mode);
std::wstring DecodeUTF8ToWStr(const std::string& s);
std::wstring DecodeUTF8ToWStr(char *p, int iLen);
std::string DecodeWStrToUTF8(const std::wstring& s);
#else
#define fopen fopen_utf8
#endif



#ifdef WIN32
// @description
// just for debugging, to see some information.
int printf_utf8();
#else
#define printf printf_utf8
#endif

};

#endif