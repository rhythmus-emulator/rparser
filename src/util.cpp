#include "util.h"
#include <string>
#include <algorithm>

#ifdef WIN32
#include <windows.h>
#include <stdarg.h>
#endif

#ifdef USE_ICONV
#include <iconv.h>
#define CP_JAP 949
#define CP_KOR 932
#define CP_UTF8 1252
#endif

namespace rparser {

// encoding part
#ifdef USE_ICONV
// use iconv
const char* GetCodepageString(int cp)
{
	switch (cp)
	{
	case 949:
		return "shift-jis";
	case 932:
		return "cp949";
	case 1252:	// utf-8
		return "utf-8";
	default:
		// unsupported
		return 0;
	}
}
int AttemptEncoding(std::string &s, int to_codepage, int from_codepage)
{
	const char* cp_from_str = GetCodepageString(from_codepage);
	const char* cp_to_str = GetCodepageString(to_codepage);
	if (!cp_from_str || !cp_to_str) return 0;

	iconv_t converter = iconv_open( cp_to_str, cp_from_str );
	if( converter == (iconv_t) -1 )
	{
		return 0;
	}

	/* Copy the string into a char* for iconv */
	ICONV_CONST char *szTextIn = const_cast<ICONV_CONST char*>( s.data() );
	size_t iInLeft = s.size();

	/* Create a new string with enough room for the new conversion */
	std::string sBuf;
	sBuf.resize( s.size() * 5 );

	char *sTextOut = const_cast<char*>( sBuf.data() );
	size_t iOutLeft = sBuf.size();
	size_t size = iconv( converter, &szTextIn, &iInLeft, &sTextOut, &iOutLeft );

	iconv_close( converter );

	if( size == (size_t)(-1) )
	{
		return false; /* Returned an error */
	}

	if( iInLeft != 0 )
	{
		printf("iconv(%s to %s) for \"%s\": whole buffer not converted",cp_from_str, cp_to_str, s.c_str());
		return false;
	}

	if( sBuf.size() == iOutLeft )
		return false; /* Conversion failed */

	sBuf.resize( sBuf.size()-iOutLeft );

	s = sBuf;
	return true;
}
int AttemptEncoding(std::string &s, int from_codepage)
{
	if (from_codepage == 0)
	{
		int iSize = 0;
		iSize = AttemptEncoding(s, CP_UTF8, 949);
		if (iSize) return iSize;
		iSize = AttemptEncoding(s, CP_UTF8, 932);
		return iSize;
	}
	return AttemptEncoding(s, CP_UTF8, from_codepage);
}
int DecodeTo(std::string &s, int to_codepage)
{
	return AttemptEncoding(s, to_codepage, CP_UTF8);
}
#else
// use windows internal function
int DecodeToWStr(const std::string& s, std::wstring& sOut, int from_codepage)
{
	int iSize = MultiByteToWideChar( from_codepage, MB_ERR_INVALID_CHARS, s.data(), s.size(), nullptr, 0 );
	if( iSize == 0 ) return 0;
	sOut.append( iSize, L' ' );
	iSize = MultiByteToWideChar( from_codepage, MB_ERR_INVALID_CHARS, s.data(), s.size(), (wchar_t *) sOut.data(), iSize );
	return iSize;
	ASSERT( iSize != 0 );
}
int EncodeFromWStr(const std::wstring& sOut, std::string& s, int to_codepage)
{
	int utf8size = WideCharToMultiByte(to_codepage, 0, sOut.data(), sOut.size(), 0, 0, 0, 0);
	ASSERT(utf8size);
	s.clear();
	s.append( utf8size, ' ' );
	utf8size = WideCharToMultiByte(to_codepage, 0, sOut.data(), sOut.size(), s.data(), utf8size, 0, 0);
	return utf8size;
}
int AttemptEncoding(std::string &s, int to_codepage, int from_codepage)
{
	// to UTF16
	std::wstring sWstr;
	if (DecodeToWStr(s, sWstr, from_codepage) == 0)
		return 0;

	// from UTF16
	return EncodeFromWStr(sWstr, s, to_codepage);
}
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
	
	// from_codepage -> UTF16
	std::wstring sWstr;
	if (DecodeToWStr(s, sWstr, from_codepage) == 0)
		return 0;

	// UTF16 -> UTF8
	return EncodeFromWStr(sWstr, s, CP_UTF8);
}
int DecodeTo(std::string &s, int to_codepage)
{
	// UTF8 -> UTF16
	std::wstring sWstr;
	if (DecodeToWStr(s, sWstr, CP_UTF8) == 0)
		return 0;

	// UTF16 -> to_codepage
	return EncodeFromWStr(sWstr, s, to_codepage);
}
FILE* fopen_utf8(const char* fname, const char* mode)
{
	std::wstring sWfn;
	std::wstring sWmode(mode);
	DecodeToWStr(fname, sWfn, CP_UTF8);
	FILE* f=0;
	if (_wfopen_s(&f, sWfn.c_str(), sWmode.c_str()) != 0)
		return 0;
	else return f;
}
int printf_utf8(const char* fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	std::wstring sWOut;
	DecodeToWStr(fmt, sWOut, CP_UTF8);
	return vwprintf(sWOut.c_str(), args);
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