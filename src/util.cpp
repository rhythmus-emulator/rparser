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

std::string lower(const std::string& s) {
	std::string sOut(s);
	std::transform(sOut.begin(), sOut.end(), sOut.begin(), ::tolower);
	return sOut;
}
bool endsWith(const std::string& s1, const std::string& s2, bool casesensitive)
{
	if (casesensitive)
	{
		int l = (s1.size() > s2.size())?s1.size:s2.size();
		const char* p1 = &s1.back();
		const char* p2 = &s2.back();
		while (l)
		{
			if (*p1 != *p2) return false;
			p1--; p2--; l--;
		}
		return true;
	}
	else
	{
		std::string ss1 = lower(s1);
		std::string ss2 = lower(s2);
		return endsWith(ss1, ss2, true);
	}
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

std::string GetExtension(const std::string& path, std::string *sOutName=0)
{
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return path;
	if (sOutName)
	{
		*sOutName = path.substr(0, pos);
	}
	return std::string(path.c_str() + pos + 1);
}

#ifdef WIN32
bool IsDirectory(const std::string & path)
{
	std::wstring wpath;
	DecodeToWStr(path, wpath, CP_UTF8);
	DWORD ftyp = GetFileAttributesW(wpath.c_str());
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    std::wstring spec, wpath, mask="*";
    std::stack<std::wstring> directories;	// currently going-to-search directories
	std::stack<std::wstring> dir_name;		// name for current directories
	std::wstring curr_dir_name;

    directories.push(path);
	dir_name.push("");

    while (!directories.empty() && maxrecursive >= 0) {
		maxrecursive--;
        path = directories.top();
        spec = path + L"\\" + mask;
		curr_dir_name = dir_name.top();
        directories.pop();
		dir_name.pop();

        hFind = FindFirstFile(spec.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE)  {
            return false;
        } 

        do {
            if (wcscmp(ffd.cFileName, L".") != 0 && 
                wcscmp(ffd.cFileName, L"..") != 0) {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    directories.push(path + L"\\" + ffd.cFileName);
					dir_name.push(curr_dir_name + ffd.cFileName + L"\\");

					std::string fn;
					EncodeFromWStr(curr_dir_name + ffd.cFileName, fn, CP_UTF8);
					vFiles.push_back(std::pairs<std::string, int>(fn,0));
                }
                else {
					std::string fn;
					EncodeFromWStr(curr_dir_name + ffd.cFileName, fn, CP_UTF8);
					vFiles.push_back(std::pairs<std::string, int>(fn,1));
                }
            }
        } while (FindNextFile(hFind, &ffd) != 0);

        if (GetLastError() != ERROR_NO_MORE_FILES) {
            FindClose(hFind);
            return false;
        }

        FindClose(hFind);
        hFind = INVALID_HANDLE_VALUE;
    }

	return true;
}
#else
bool IsDirectory(const std::string& path)
{
	return true;
}
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive)
{
    DIR *dp;
    struct dirent *dirp;
	std::vector<std::string> directories;
	std::vector<std::string> dir_name;

	directories.push(path);
	dir_name.push("");

	std::string curr_dir_name;
	std::string curr_dir;

	while (maxrecursive >= 0)
	{
		maxrecursive--;
		curr_dir = directories.top();
		curr_dir_name = dir_name.top();
		directories.pop();
		dir_name.pop();
		if((dp  = opendir(curr_dir.c_str())) == NULL) {
			printf("Error opening %s dir.\n", curr_dir.c_str());
			return false;
		}
		while ((dirp = readdir(dp)) != NULL) {
			if (dirp->d_type == DT_DIR)
			{
				directories.push(curr_dir + "/" + dirp->d_name);
				dir_name.push(curr_dir_name + dirp->d_name + "/");
			}
			else if (dirp->d_type == DT_REG)
			{
				vFiles.push_back(std::pair<std::string, int>(curr_dir_name + string(dirp->d_name), 1));
			}
		}
		closedir(dp);
	}

	return true;
}
#endif

// ------ class BasicDirectory ------

int rparser::ArchiveDirectory::Open(const std::string &path)
{
	// parse all files to full depth
	return 0;
}

int rparser::ArchiveDirectory::Write(const FileData &fd)
{
	return 0;
}

int rparser::ArchiveDirectory::Read(FileData &fd)
{
	return 0;
}

int rparser::ArchiveDirectory::ReadFiles(std::vector<FileData>& fd)
{
	return 0;
}

int rparser::ArchiveDirectory::Flush()
{
	// do nothing
	return 0;
}

int rparser::ArchiveDirectory::Create(const std::string& path)
{
	return 0;
}

int rparser::ArchiveDirectory::Close()
{
	return 0;
}

static int Test(const std::string &path)
{
	return 0;
}

// ------ class ArchiveDirectory ------

int rparser::ArchiveDirectory::Open(const std::string& path)
{
    Close();
	m_sPath = path;
    m_Archive = zip_open(path.c_str(), ZIP_RDONLY, &error);
    if (error)
    {
        printf("Zip Reading Error occured - code %d\nstr (%s)\n", error, zip_error_to_str(error));
        return error;
    }
    // get all file lists from zip
    // TODO: should detect encoding
    int iEntries = zip_get_num_entries(m_Archive);
    zip_stat zStat;
    for (int i=0; i<iEntries; i++)
    {
        zip_stat_index(m_Archive, i, ZIP_FL_UNCHANGED, &zStat);
        m_vFilename.push_back(zStat.name);
    }
    // read 
    return 0;
}

int rparser::ArchiveDirectory::Read(FileData &fd)
{
    ASSERT(fd.p == 0 && m_Archive);
    zip_file_t *zfp = zip_fopen(m_Archive, fd.fn.c_str(), ZIP_FL_UNCHANGED);
    if (!zfp)
    {
        printf("Failed to read file(%s) from zip\n", fd.fn.c_str());
        return -1;
    }
    zip_stat zStat;
    zip_stat(m_Archive, fd.fn.c_str(), 0, &zStat);
    fd.iLen = zStat.size;
    fd.p = (unsigned char*)malloc(fd.iLen);
    zip_fread(zfp, (void*)fd.p, fd.iLen);
    zip_fclose(zfp);
    return 0;
}

int rparser::ArchiveDirectory::Write(const FileData &fd)
{
    ASSERT(m_Archive);
    zip_source_t *s;
    s = zip_source_buffer(m_Archive, fd.p, fd.iLen, 0);
    if (!s)
    {
        printf("Zip source buffer creation failed!\n");
        return -1;
    }
    error = zip_file_add(m_Archive, fd.fn.c_str(), s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
    zip_source_free(s);
    if (error)
    {
        printf("Zip file appending failed! (code %d)\n", error);
    }
    return 0;
}

int rparser::ArchiveDirectory::Flush()
{
    // do nothing?
    // maybe need to call Close() ...
    return 0;
}

int rparser::ArchiveDirectory::Create(const std::string& path)
{
    Close();
    m_Archive = zip_open(path.c_str(), ZIP_CREATE | ZIP_EXCL, &error);
    if (error)
    {
        printf("Zip Creating Error occured - code %d\nstr (%s)\n", error, zip_error_to_str(error));
        return error;
    }
    return 0;
}

int rparser::ArchiveDirectory::Close()
{
    if (m_Archive)
    {
        return zip_close(m_Archive);
    }
    return 0;
}

int rparser::ArchiveDirectory::Test(const std::string& path)
{
    return endsWith(path, ".zip") || endsWith(path, ".osz");
}

void rparser::ArchiveDirectory::SetEncoding(const std::string& sEncoding)
{
	m_sEncoding = sEncoding;
}



// BMS-related utils

int atoi_bms(const char* p)
{
	return 0;
}
int atoi_bms16(const char* p)
{
	return 0;
}

}