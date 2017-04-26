#include "util.h"
#include <string>
#include <algorithm>

#ifdef WIN32
#include <windows.h>
#endif


namespace rparser {

int global_seed;
void SetSeed(int seed) { global_seed = seed; }
int GetSeed() { return global_seed; }
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