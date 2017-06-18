#include "util.h"
#include <string>
#include <algorithm>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_ICONV
#include <iconv.h>
#endif

namespace rparser {

// encoding part
#ifdef USE_ICONV
// use iconv
int AttemptEncoding(std::string &s, int from_codepage)
{
	
	return 0;
}
std::string AutoDetectEncoding(char *data, int iLen);
std::string DecodeToUTF8(char *p, int iLen, char *encoding_from);
std::string EncodeTo(char *p_utf8, int iLen, const std::string& to_charset);
#else
// use windows internal function
int AttemptEncoding(std::string &s, int from_codepage)
{
	if (iLen == 0)
	{
		int iSize = 0;
		iSize = AttemptEncoding(s, 949);
		if (iSize) return iSize;
		iSize = AttemptEncoding(s, 932);
		if (iSize) return iSize;
		else return 0;
	}
	int iSize = MultiByteToWideChar( from_codepage, MB_ERR_INVALID_CHARS, s.data(), s.size(), nullptr, 0 );
	if( iSize == 0 ) return 0;
	
	// from_codepage -> UTF16
	std::wstring sOut;
	sOut.append( iSize, L' ' );
	iSize = MultiByteToWideChar( from_codepage, MB_ERR_INVALID_CHARS, s.data(), s.size(), (wchar_t *) sOut.data(), iSize );
	ASSERT( iSize != 0 );

	// UTF16 -> UTF8
	int utf8size = WideCharToMultiByte(CP_UTF8, 0, sOut.data(), sOut.size(), 0, 0, 0, 0);
	ASSERT(utf8size);
	s.clear();
	s.append( utf8size, ' ' );
	utf8size = WideCharToMultiByte(CP_UTF8, 0, sOut.data(), sOut.size(), s.data(), utf8size, 0, 0);
	return utf8size;
}
std::string DecodeTo(std::string &s, int to_codepage)
{
	// UTF8 -> UTF16
	int iSize = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), s.size(), nullptr, 0 );
	if( iSize == 0 ) return 0;
	std::wstring sOut;
	sOut.append( iSize, L' ' );
	iSize = MultiByteToWideChar( CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), s.size(), (wchar_t *) sOut.data(), iSize );
	ASSERT( iSize != 0 );

	// UTF16 -> to_codepage
	int utf8size = WideCharToMultiByte(to_codepage, 0, sOut.data(), sOut.size(), 0, 0, 0, 0);
	ASSERT(utf8size);
	std::string sUTF8;
	sUTF8.append( utf8size, ' ' );
	utf8size = WideCharToMultiByte(to_codepage, 0, sOut.data(), sOut.size(), sUTF8.data(), utf8size, 0, 0);
	return sUTF8;
}
FILE* fopen_utf8(const wchar_t* fname, const char* mode)
{
	return 0;
}
FILE* fopen_utf8(const char* fname, const char* mode)
{
	return 0;
}
std::wstring DecodeUTF8ToWStr(const std::string& s)
{
	return 0;
}
std::wstring DecodeUTF8ToWStr(char *p, int iLen)
{
	return 0;
}
std::string DecodeWStrToUTF8(const std::wstring& s)
{
	return 0;
}
int printf_utf8()
{
	return 0;
}
#endif

// seed part ..?

int global_seed;
void SetSeed(int seed) { global_seed = seed; }
int GetSeed() { return global_seed; }


// string-path part

void lower(std::string& s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

std::string CleanPath(const std::string& path)
{
	std::string r;
	for (int i = 0; i < path.length(); i++) {
		char c = path[i];
		if (c == '\\')
			c = '/';
		else if (c == '¥')
			c = '/';
		r.push_back(c);
	}
	return r;
}

std::string GetDirectoryname(const std::string & path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return "";
	return std::string(path, pos);
}

std::string GetFilename(const std::string & path)
{
	size_t pos = path.find_last_of('/');
	if (pos == std::string::npos)
		return path;
	return std::string(path.c_str()+pos+1);
}

std::string GetExtension(const std::string & path)
{
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return path;
	return std::string(path.c_str() + pos + 1);
}

bool IsDirectory(const std::string & path)
{
	std::wstring wpath = DecodeUTF8ToWStr(path);
	DWORD ftyp = GetFileAttributesW(wpath.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

FILE * fopen_utf8(const wchar_t * fname, const char * mode)
{
	return nullptr;
}

FILE * fopen_utf8(const char * fname, const char * mode)
{
	return nullptr;
}

std::wstring DecodeUTF8ToWStr(const std::string & s)
{
	return std::wstring();
}

std::wstring DecodeUTF8ToWStr(char * p, int iLen)
{
	return std::wstring();
}

std::string DecodeWStrToUTF8(const std::wstring & s)
{
	return std::string();
}

}