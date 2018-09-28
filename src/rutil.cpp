#include "rutil.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <stack>
#include <cctype>
//#include <dirent.h>

#ifdef USE_ZLIB
#define ZIP_STATIC 1
#include <zip.h>
#endif

#ifdef USE_OPENSSL 
#include <openssl/md5.h>
#endif

#ifdef WIN32
#include <windows.h>
#include <stdarg.h>
#endif

#ifdef USE_ICONV
#include <iconv.h>
#define CP_932 932	// JAP
#define CP_949 949	// KOR
#define CP_UTF8 1252
#endif

namespace rutil {

// encoding part
#ifdef USE_ICONV
// use iconv
const char* GetCodepageString(int cp)
{
	switch (cp)
	{
	case 949:
		return "euc-kr";
	case 932:
		return "shift-jis";
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
int EncodeFromWStr(const std::wstring& s, std::string& sOut, int to_codepage)
{
	int utf8size = WideCharToMultiByte(to_codepage, 0, s.data(), s.size(), 0, 0, 0, 0);
	ASSERT(utf8size);
	sOut.clear();
	sOut.append( utf8size, ' ' );
	utf8size = WideCharToMultiByte(to_codepage, 0, s.data(), s.size(), (char*)sOut.data(), utf8size, 0, 0);
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
	if (from_codepage == 0)
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
    std::string smode(mode);
    std::wstring sWmode;    sWmode.assign(smode.begin(), smode.end());
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

bool RemoveFile(const char* fpath)
{
#ifdef WIN32
	std::wstring sWfn;
	DecodeToWStr(fpath, sWfn, CP_UTF8);
	return (_wremove(sWfn.c_str()) == 0);
#else
	return (remove(fpath) == 0);
#endif
}
bool RenameFile(const char* prev_path, const char* new_path)
{
#ifdef WIN32
	std::wstring sWpfn, sWnfn;
	DecodeToWStr(prev_path, sWpfn, CP_UTF8);
	DecodeToWStr(new_path, sWnfn, CP_UTF8);
	return (_wrename(sWpfn.c_str(), sWnfn.c_str()) == 0);
#else
	return (rename(prev_path, new_path) == 0);
#endif
}
bool CreateFolder(const char* path)
{
#ifdef WIN32
	std::wstring wpath;
	DecodeToWStr(path, wpath, CP_UTF8);
	return (CreateDirectoryW(wpath.c_str(), 0) != 0);
#else
	return (mkdir(path, 664) == 0);
#endif
}

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
bool isspace(char c) { return (c == ' ' || c == '\t' || c == '\r' || c == '\n'); }
std::string trim(const std::string &line)
{
    auto wsfront = std::find_if_not(line.begin(), line.end(), [](int c) {return isspace(c); });
    auto wsback = std::find_if_not(line.rbegin(), line.rend(), [](int c) {return isspace(c); }).base();
    std::string line_trim = (wsback <= wsfront ? std::string() : std::string(wsfront, wsback));
    return line_trim;
}
int split(const std::string& str, const char sep, std::vector<std::string>& vsOut)
{
    std::stringstream ss;
    ss.str(str);
    std::string item;
    vsOut.clear();
    while (std::getline(ss, item, sep)) {
		vsOut.push_back(item);
    }
    return vsOut.size();
}
int split(const std::string& str, const char sep, std::string &s1, std::string &s2)
{
    auto sSep = std::find_if(str.begin(), str.end(), [&](char c) { return c == sep; });
    if (sSep == str.end()) return 0;
    s1 = std::string(str.begin(), sSep);
    s2 = std::string(sSep+1, str.end());
    return 1;
}
bool endsWith(const std::string& s1, const std::string& s2, bool casesensitive)
{
	if (casesensitive)
	{
		int l = (s1.size() < s2.size())?s1.size():s2.size();
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
	for (unsigned int i = 0; i < path.length(); i++) {
		char c = path[i];
		if (c == '\\')
			c = '/';
		//else if (c == '¥')
		//	c = '/';
		r.push_back(c);
	}
	return r;
}
std::string GetParentDirectory(const std::string& path)
{
    return GetDirectory(GetDirectory(path));
}
std::string GetDirectory(const std::string & path)
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

std::string GetExtension(const std::string& path, std::string *sOutName)
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
std::string GetPathJoin(const std::string& s1, const std::string s2)
{
	if (s1.empty()) return s1;
	if (s1.back() == '/' || s1.back() == '\\')
		return s1 + s2;
	else
		return s1 + '/' + s2;
}

bool CheckExtension(const std::string& path, const std::string &filter)
{
	const std::string&& ext = GetExtension(path);
	const std::string&& ext_lower = lower(ext);
	std::vector<std::string> filter_exts;
	split(filter, ';', filter_exts);
	auto it = std::find(filter_exts.begin(), filter_exts.end(), ext_lower);
	return (it != filter_exts.end());
}

std::string ChangeExtension(const std::string& path, const std::string &new_ext)
{
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos)
		return path + "." + new_ext;
	return path.substr(0, pos + 1) + new_ext;
}

uint32_t ReadLE32(const unsigned char* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
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
bool CreateDirectory(const std::string& path)
{
	std::wstring wpath;
	DecodeToWStr(path, wpath, CP_UTF8);
	return _wmkdir(wpath.c_str()) == 0;
}
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW ffd;
    std::wstring spec, wpath, mask=L"*";
    std::stack<std::wstring> directories;	// currently going-to-search directories
	std::stack<std::wstring> dir_name;		// name for current directories
	std::wstring curr_dir_name;

    DecodeToWStr(path, wpath, CP_UTF8);
    directories.push(wpath);
	dir_name.push(L"");

    while (!directories.empty() && maxrecursive >= 0) {
		maxrecursive--;
        wpath = directories.top();
        spec = wpath + L"\\" + mask;
		curr_dir_name = dir_name.top();
        directories.pop();
		dir_name.pop();

        hFind = FindFirstFileW(spec.c_str(), &ffd);
        if (hFind == INVALID_HANDLE_VALUE)  {
            return false;
        } 

        do {
            if (wcscmp(ffd.cFileName, L".") != 0 && 
                wcscmp(ffd.cFileName, L"..") != 0) {
                if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    directories.push(wpath + L"\\" + ffd.cFileName);
					dir_name.push(curr_dir_name + ffd.cFileName + L"\\");

					std::string fn;
					EncodeFromWStr(curr_dir_name + ffd.cFileName, fn, CP_UTF8);
					vFiles.push_back(std::pair<std::string, int>(fn,0));
                }
                else {
					std::string fn;
					EncodeFromWStr(curr_dir_name + ffd.cFileName, fn, CP_UTF8);
					vFiles.push_back(std::pair<std::string, int>(fn,1));
                }
            }
        } while (FindNextFileW(hFind, &ffd) != 0);

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
	struct stat st;
	if(stat(path.c_str(),&st) == 0)
		if(st.st_mode & S_IFDIR != 0)
			return true;
	return false;
}
bool CreateDirectory(const std::string& path)
{
	return mkdir(path.c_str(), 0755) == 0;
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

FileData::FileData()
{
	p = 0;
	m_iLen = 0;
    m_iPos = 0;
}

FileData::FileData(const std::string& fn)
{
	FileData();
	this->fn = fn;
}

// only for seeking purpose
FileData::FileData(uint8_t *p, uint32_t iLen)
{
    this->p = p;
    m_iLen = iLen;
    m_iPos = 0;
}

FileData::~FileData()
{
	if (p) free(p);
    p = 0;
}

std::string FileData::GetFilename() { return fn; }
uint32_t FileData::GetFileSize() { return m_iLen; }
uint32_t FileData::GetPos() { return m_iPos; }
void FileData::SetPos(uint32_t iPos) { m_iPos = iPos; }
uint8_t* FileData::GetPtr() { return p; }

int FileData::SeekSet(uint32_t p)
{
    return Seek(p, SEEK_SET);
}
int FileData::SeekCur(uint32_t p)
{
    return Seek(p, SEEK_CUR);
}
int FileData::SeekEnd(uint32_t p)
{
    return Seek(p, SEEK_END);
}

int FileData::Seek(uint32_t p, int mode)
{
    uint32_t iNewPos;
    switch (mode)
    {
    case SEEK_CUR:
        iNewPos = m_iPos + p;
        break;
    case SEEK_END:
        iNewPos = m_iLen - p;
        break;
    case SEEK_SET:
        iNewPos = p;
        break;
    }
    //if (m_iPos < 0) m_iPos = 0;
    if (iNewPos > m_iLen) return -1;
    else m_iPos = iNewPos;
    return 0;
}

uint32_t FileData::ReadLE32()
{
    uint32_t v = rutil::ReadLE32(p + m_iPos);
    SeekCur(4);
    if (m_iPos > m_iLen) m_iPos = m_iLen;
    return v;
}

uint32_t FileData::Read(uint8_t *out, uint32_t len)
{
    uint32_t copysize = len;
    if (m_iPos + len > m_iLen)
    {
        copysize = m_iLen - m_iPos;
    }
    memcpy(out, p + m_iPos, copysize);
    return copysize;
}

bool FileData::IsEOF() { return m_iPos == m_iLen; };

// ------ class IDirectory ------

bool IDirectory::ReadSmart(FileData &fd) const
{
	// first do search with specified name
	if (IN_ARRAY(m_vFilename, fd.fn))
		return Read(fd)>0;
	
	// second split ext, and find similar files(ext)
	std::string sExt, sFileNameNoExt;
	sExt = lower(GetExtension(fd.fn, &sFileNameNoExt));
	if (sExt == ".wav" ||
		sExt == ".ogg" ||
		sExt == ".flac" ||
		sExt == ".mp3")
	{
		if (IN(m_vFilename, ".wav")) fd.fn = sFileNameNoExt + ".wav";
		else if (IN(m_vFilename, ".ogg")) fd.fn = sFileNameNoExt + ".ogg";
		else if (IN(m_vFilename, ".flac")) fd.fn = sFileNameNoExt + ".flac";
		else if (IN(m_vFilename, ".mp3")) fd.fn = sFileNameNoExt + ".mp3";
		else return false;
		return Read(fd)>0;
	}
	if (sExt == ".avi" ||
		sExt == ".wmv" ||
		sExt == ".mpg" ||
		sExt == ".mpeg" ||
		sExt == ".webm" ||
		sExt == ".mkv")
	{
		if (IN(m_vFilename, ".avi")) fd.fn = sFileNameNoExt + ".avi";
		else if (IN(m_vFilename, ".wmv")) fd.fn = sFileNameNoExt + ".wmv";
		else if (IN(m_vFilename, ".mpg")) fd.fn = sFileNameNoExt + ".mpg";
		else if (IN(m_vFilename, ".mpeg")) fd.fn = sFileNameNoExt + ".mpeg";
		else if (IN(m_vFilename, ".webm")) fd.fn = sFileNameNoExt + ".webm";
		else if (IN(m_vFilename, ".mkv")) fd.fn = sFileNameNoExt + ".mkv";
		else return false;
		return Read(fd)>0;
	}
	if (sExt == ".bmp" ||
		sExt == ".jpg" ||
		sExt == ".jpeg" ||
		sExt == ".png" ||
		sExt == ".gif")
	{
		if (IN(m_vFilename, ".bmp")) fd.fn = sFileNameNoExt + ".bmp";
		else if (IN(m_vFilename, ".jpg")) fd.fn = sFileNameNoExt + ".jpg";
		else if (IN(m_vFilename, ".jpeg")) fd.fn = sFileNameNoExt + ".jpeg";
		else if (IN(m_vFilename, ".png")) fd.fn = sFileNameNoExt + ".png";
		else if (IN(m_vFilename, ".gif")) fd.fn = sFileNameNoExt + ".gif";
		else return false;
		return Read(fd)>0;
	}

	return false;
}

int IDirectory::Read(const std::string fpath, FileData &fd) const
{
	fd.p = 0;
    fd.fn = fpath;
    return Read(fd);
}

std::vector<std::string> IDirectory::GetFileEntries(const char* ext_filter)
{
	if (!ext_filter) return m_vFilename;

	std::vector<std::string> exts;
	std::vector<std::string> vsOut;
	split(ext_filter,';',exts);
	for (auto &fn: m_vFilename)
	{
		std::string sExt = GetExtension(fn);
		if (IN_ARRAY(exts, sExt))
			vsOut.push_back(fn);
	}
	return vsOut;
}

std::vector<std::string> IDirectory::GetFolderEntries()
{
	return m_vFolder;
}

// ------ class BasicDirectory ------

int BasicDirectory::Open(const std::string &path)
{
	if (!IsDirectory(path)) return -1;
	m_sPath = path;
    DirFileList vFiles;
	GetDirectoryFiles(path, vFiles, m_iRecursiveDepth);
	for (auto& entry: vFiles)
	{
		if (entry.second == 1)	// file
		{
			m_vFilename.push_back(entry.first);
		}
		else	// folder
		{
			m_vFolder.push_back(entry.first);
		}
	}
    return 0;
}

int BasicDirectory::Write(const FileData &fd)
{
	std::string sFullpath = GetPathJoin(m_sPath, fd.fn);
	FILE *fp = fopen_utf8(sFullpath.c_str(), "wb");
	if (!fp) return 0;
	int r = fwrite(fd.p, 1, fd.m_iLen, fp);
	fclose(fp);
	return r;
}

int BasicDirectory::Read(FileData &fd) const
{
	std::string sFullpath = GetPathJoin(m_sPath, fd.fn);
	FILE *fp = fopen_utf8(sFullpath.c_str(), "rb");
	if (!fp) return 0;
	fseek(fp, 0, SEEK_END);
	fd.m_iLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	fd.p = (unsigned char*)malloc(fd.m_iLen);
	ASSERT(fd.p != 0);
	fd.m_iLen = fread(fd.p, 1, fd.m_iLen, fp);
	return fd.m_iLen;
}

int BasicDirectory::ReadFiles(std::vector<FileData>& fd) const
{
	FileData fdat;
	int r = m_vFilename.size();
	for (auto &fn : m_vFilename)
	{
        fdat.fn = fn;
        fdat.p = 0;
		if (!Read(fdat))
		{
			r = 0;
			break;
		}
        fd.push_back(fdat);
	}
	return r;
}

int BasicDirectory::Flush()
{
	// do nothing
	return 0;
}

int BasicDirectory::Create(const std::string& path)
{
	return CreateDirectory_RPARSER(path);
}

int BasicDirectory::Close()
{
	// do nothing
	return 0;
}

void BasicDirectory::SetRecursiveDepth(int iRecursiveDepth)
{
    m_iRecursiveDepth = iRecursiveDepth;
}

int BasicDirectory::Test(const std::string &path)
{
	return IsDirectory(path);
}

#ifdef USE_ZLIB
// ------ class ArchiveDirectory ------

int ArchiveDirectory::Open(const std::string& path)
{
    Close();
	m_sPath = path;
    m_Archive = zip_open(path.c_str(), ZIP_RDONLY, &error);
    if (error)
    {
        zip_error_t zerror;
        zip_error_init_with_code(&zerror, error);
        printf("Zip Reading Error occured - code %d\nstr (%s)\n", error, zip_error_strerror(&zerror));
        zip_error_fini(&zerror);
        return error;
    }
    // get all file lists from zip, with encoding
    int iEntries = zip_get_num_entries(m_Archive, ZIP_FL_UNCHANGED);
    struct zip_stat zStat;
    for (int i=0; i<iEntries; i++)
    {
        zip_stat_index(m_Archive, i, ZIP_FL_UNCHANGED, &zStat);
		std::string fn = zStat.name;
		if (m_iCodepage)
		{
			AttemptEncoding(fn, m_iCodepage);
		}
        m_vFilename.push_back(fn);
    }
    return 0;
}

int ArchiveDirectory::Read(FileData &fd) const
{
    ASSERT(fd.p == 0 && m_Archive);
    zip_file_t *zfp = zip_fopen(m_Archive, fd.fn.c_str(), ZIP_FL_UNCHANGED);
    if (!zfp)
    {
        printf("Failed to read file(%s) from zip\n", fd.fn.c_str());
        return -1;
    }
    struct zip_stat zStat;
    zip_stat(m_Archive, fd.fn.c_str(), 0, &zStat);
    fd.m_iLen = zStat.size;
    fd.p = (unsigned char*)malloc(fd.m_iLen);
    zip_fread(zfp, (void*)fd.p, fd.m_iLen);
    zip_fclose(zfp);
    return fd.m_iLen;
}

int ArchiveDirectory::ReadFiles(std::vector<FileData>& fd) const
{
	// not implemented
	ASSERT(0);
    return 0;
}

int ArchiveDirectory::Write(const FileData &fd)
{
    ASSERT(m_Archive);
    zip_source_t *s;
    s = zip_source_buffer(m_Archive, fd.p, fd.m_iLen, 0);
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
    return fd.m_iLen;
}

int ArchiveDirectory::Flush()
{
    // COMMENT is there more effcient way?
	zip_close(m_Archive);
	m_Archive = 0;
	return Open(m_sPath);
	//return 0;
}

int ArchiveDirectory::Create(const std::string& path)
{
    Close();
    m_Archive = zip_open(path.c_str(), ZIP_CREATE | ZIP_EXCL, &error);
    if (error)
    {
        zip_error_t zerror;
        zip_error_init_with_code(&zerror, error);
        printf("Zip Creating Error occured - code %d\nstr (%s)\n", error, zip_error_strerror(&zerror));
        zip_error_fini(&zerror);
        return error;
    }
    return 0;
}

int ArchiveDirectory::Close()
{
    if (m_Archive)
    {
        return zip_close(m_Archive);
    }
    return 0;
}

int ArchiveDirectory::Test(const std::string& path)
{
    return endsWith(path, ".zip", false) || endsWith(path, ".osz", false);
}

void ArchiveDirectory::SetCodepage(int iCodepage)
{
	m_iCodepage = iCodepage;
}
#endif


bool md5(const void* p, int iLen, char* out)
{
#ifdef USE_OPENSSL
	MD5((unsigned char*)p, iLen, out);
	return true;
#else
	return false;
#endif
}

bool md5_str(const void* p, int iLen, char *out)
{
#ifndef USE_OPENSSL
#define MD5_DIGEST_LENGTH 16
#endif
	char result[MD5_DIGEST_LENGTH];
	if (md5(p, iLen, result))
	{
		char buf[6];
		for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
		{
			sprintf(buf, "%02x", result[i]);
			out += 2;
		}
	}
	else return false;
}


// BMS-related utils
int atoi_bms(const char* p, int length)
{
	int r = 0;
	while (p && length)
	{
		r *= 26+10;
		if (*p >= 'a' && *p <= 'z')
			r += 10+(*p-'a');
		else if (*p >= 'A' && *p <= 'Z')
			r += 10+(*p-'A');
		else if (*p >= '0' && *p <= '9')
			r += (*p-'0');
		else break;
		++p; --length;
	}
	return r;
}
int atoi_bms16(const char* p, int length)
{
	// length up-to-2
	int r = 0;
	while (p && length)
	{
		r *= 16;
		if (*p >= 'a' && *p <= 'f')
			r += 10+(*p-'a');
		else if (*p >= 'A' && *p <= 'F')
			r += 10+(*p-'A');
		else if (*p >= '0' && *p <= '9')
			r += (*p-'0');
		else break;
		++p; --length;
	}
	return r;
}

}