/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_UTIL_H
#define RPARSER_UTIL_H

#include <stdio.h>
#include <string>

#include <assert.h>
#define ASSERT assert

namespace rparser {

// @description
// actually, it only detects Shift_JIS / CP949 / UTF-8 encodings.
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

// @description
// set global seed (mostly used for BMS parsing)
void SetSeed(int seed);
// @description
// get seed
int GetSeed();


void lower(std::string& s);
// @description tidy pathname in case of using irregular separator
std::string CleanPath(const std::string& path);
// @description get absolute path
std::string GetAbsolutePath(const std::string& path);

std::string GetDirectoryname(const std::string& path);
std::string GetFilename(const std::string& path);
std::string GetExtension(const std::string& path);
bool IsDirectory(const std::string& path);


// @description
// simple library for memory stream, able to integrating with file stream
class MemoryInputStream : public std::istream
{
public:
	MemoryInputStream(const uint8_t* aData, size_t aLength) :
		std::istream(&m_buffer),
		m_buffer(aData, aLength)
	{
		rdbuf(&m_buffer); // reset the buffer after it has been properly constructed
	}

private:
	class MemoryBuffer : public std::basic_streambuf<char>
	{
	public:
		MemoryBuffer(const uint8_t* aData, size_t aLength)
		{
			setg((char*)aData, (char*)aData, (char*)aData + aLength);
		}
	};

	MemoryBuffer m_buffer;
};

};

#endif