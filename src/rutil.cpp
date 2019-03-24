#include "rutil.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <stack>
#include <cctype>
#include <stdarg.h>
#include <sys/stat.h>
//#include <dirent.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef USE_ZLIB
# define ZIP_STATIC 1
# include <zip.h>
#endif

#ifdef USE_OPENSSL
# include <openssl/md5.h>
#endif

#ifdef WIN32
# include <windows.h>
# include <stdarg.h>
# undef CreateDirectory
# undef DeleteFile

# define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
# define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
# define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#endif

#ifdef USE_ICONV
# include <iconv.h>
#endif

#include <assert.h>
#ifdef DEBUG
# define ASSERT(x) assert(x)
#else
# define ASSERT(x)
#endif

namespace rutil {

bool TestUTF8Encoding(const std::string &s)
{
  const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.c_str());
  int num;
  while (*bytes != 0x00)
  {
    if ((*bytes & 0x80) == 0x00)
    {
      // U+0000 to U+007F 
      num = 1;
    }
    else if ((*bytes & 0xE0) == 0xC0)
    {
      // U+0080 to U+07FF 
      num = 2;
    }
    else if ((*bytes & 0xF0) == 0xE0)
    {
      // U+0800 to U+FFFF 
      num = 3;
    }
    else if ((*bytes & 0xF8) == 0xF0)
    {
      // U+10000 to U+10FFFF 
      num = 4;
    }
    else
      return false;
    bytes += 1;
    for (int i = 1; i < num; ++i)
    {
      if ((*bytes & 0xC0) != 0x80)
        return false;
      bytes += 1;
    }
  }
  return true;
}

bool TestJISEncoding(const std::string &s)
{
  // XXX: not exact method.
  const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.c_str());
  while (*bytes != 0x00)
  {
    if ((*bytes & 0x80) == 0x00)
    {
      // U+0000 to U+007F: single byte
      bytes++;
    }
    else if ( ((0x81 <= (*bytes) && (*bytes) < 0x9f) || (0xe0 <= (*bytes) && (*bytes) < 0xef)) &&
              ((0x40 <= (*(bytes+1)) && (*(bytes+1)) <= 0x7e) || (0x80 <= (*(bytes+1)) && (*(bytes+1)) <= 0xfc)) )
    {
      bytes+=2;
    }
    else return false;
  }
  return true;
}

bool TestEUCKREncoding(const std::string &s)
{
  const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.c_str());
  while (*bytes != 0x00)
  {
    if ((*bytes & 0x80) == 0x00)
    {
      // U+0000 to U+007F: single byte
      bytes++;
    }
    else if (0xa1 <= (*bytes) && (*bytes) <= 0xfe && 0xa1 <= (*(bytes+1)) && (*(bytes+1)) <= 0xfe)
    {
      // KS X 1001
      bytes+=2;
    }
    else if (0x81 <= (*bytes) && (*bytes) <= 0xc5 && (
             (0x41 <= (*(bytes+1)) && (*(bytes+1)) <= 0x5A) ||
             (0x61 <= (*(bytes+1)) && (*(bytes+1)) <= 0x7A) ||
             (0x81 <= (*(bytes+1)) && (*(bytes+1)) <= 0xFE)) )
    {
      // extended CP949
      bytes+=2;
    }
    else return false;
  }
  return true;
}

int DetectEncoding(const std::string &str)
{
  if (TestUTF8Encoding(str)) return E_UTF8;
  else if (TestJISEncoding(str)) return E_SHIFT_JIS;
  else if (TestEUCKREncoding(str)) return E_EUC_KR;
  return 0;
}

// encoding part
#ifdef USE_ICONV
// use iconv
const char* GetCodepageString(int cp)
{
  switch (cp)
  {
  case E_EUC_KR:
    return "euc-kr";
  case E_SHIFT_JIS:
    return "shift-jis";
  case E_UTF8:	// utf-8
    return "utf-8";
  default:
    // unsupported
    return 0;
  }
}
std::string ConvertEncoding(const std::string &s, int to_codepage, int from_codepage)
{
  // Check encoding if necessary
  if (from_codepage == 0)
  {
    from_codepage = DetectEncoding(s);
    if (!from_codepage) return std::string();
  }

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

  return sBuf;
}
int DecodeTo(std::string &s, int to_codepage)
{
  return ConvertEncoding(s, to_codepage, E_UTF8);
}
#else
// use windows internal function
UINT GetWindowsCodePage(UINT codepage)
{
  switch (codepage)
  {
  case E_UTF8:
    return CP_UTF8;
  default:
    return codepage;
  }
}
int DecodeToWStr(const std::string& s, std::wstring& sOut, int from_codepage)
{
  const UINT from_codepage_win = GetWindowsCodePage(from_codepage);
  int iSize = MultiByteToWideChar(from_codepage_win, MB_ERR_INVALID_CHARS, s.data(), s.size(), nullptr, 0 );
  if( iSize == 0 ) return 0;
  sOut.append( iSize, L' ' );
  iSize = MultiByteToWideChar(from_codepage_win, MB_ERR_INVALID_CHARS, s.data(), s.size(), (wchar_t *) sOut.data(), iSize );
  return iSize;
  ASSERT( iSize != 0 );
}
int EncodeFromWStr(const std::wstring& s, std::string& sOut, int to_codepage)
{
  const UINT to_codepage_win = GetWindowsCodePage(to_codepage);
  int utf8size = WideCharToMultiByte(to_codepage_win, 0, s.data(), s.size(), 0, 0, 0, 0);
  ASSERT(utf8size);
  sOut.clear();
  sOut.append( utf8size, ' ' );
  utf8size = WideCharToMultiByte(to_codepage_win, 0, s.data(), s.size(), (char*)sOut.data(), utf8size, 0, 0);
  return utf8size;
}
std::string ConvertEncoding(const std::string &s, int to_codepage, int from_codepage)
{
  // Check encoding if necessary
  if (from_codepage == 0)
  {
    from_codepage = DetectEncoding(s);
    if (!from_codepage) return std::string();
  }

  // to UTF16
  std::wstring sWstr;
  int iSize = 0;
  if (DecodeToWStr(s, sWstr, from_codepage) == 0)
    return 0;

  // from UTF16
  std::string r;
  iSize = EncodeFromWStr(sWstr, r, to_codepage);
  if (!iSize) return std::string();
  else return r;
}
FILE* fopen_utf8(const char* fname, const char* mode)
{
  std::wstring sWfn;
  std::string smode(mode);
  std::wstring sWmode;    sWmode.assign(smode.begin(), smode.end());
  DecodeToWStr(fname, sWfn, E_UTF8);
  FILE* f = 0;
  if (_wfopen_s(&f, sWfn.c_str(), sWmode.c_str()) != 0)
    return 0;
  else return f;
}
FILE* fopen_utf8(const std::string& fname, const std::string& mode)
{
  return fopen_utf8(fname.c_str(), mode.c_str());
}
int printf_utf8(const char* fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  std::wstring sWOut;
  DecodeToWStr(fmt, sWOut, E_UTF8);
  return vwprintf(sWOut.c_str(), args);
}
#endif
std::string ConvertEncodingToUTF8(const std::string &s, int from_codepage)
{
  return ConvertEncoding(s, E_UTF8, from_codepage);
}
bool IsAccessable(const std::string& fpath)
{
#ifdef WIN32
  std::wstring sWOut;
  DecodeToWStr(fpath, sWOut, E_UTF8);
  return _waccess(sWOut.c_str(), 0) == 0;
#else
  return access(fpath.c_str(), 0) == 0;
#endif
}
bool IsFile(const std::string& fpath)
{
#ifdef WIN32
  std::wstring wfpath;
  DecodeToWStr(fpath, wfpath, E_UTF8);
  struct _stat sb;
  return (_wstat(wfpath.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
#else
  struct stat sb;
  return (stat(path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
#endif
}
bool DeleteFile(const std::string& fpath)
{
#ifdef WIN32
  std::wstring sWfn;
  DecodeToWStr(fpath, sWfn, E_UTF8);
  return (_wremove(sWfn.c_str()) == 0);
#else
  return (remove(fpath.c_str()) == 0);
#endif
}
bool Rename(const std::string& prev_path, const std::string& new_path)
{
#ifdef WIN32
  std::wstring sWpfn, sWnfn;
  DecodeToWStr(prev_path, sWpfn, E_UTF8);
  DecodeToWStr(new_path, sWnfn, E_UTF8);
  return (_wrename(sWpfn.c_str(), sWnfn.c_str()) == 0);
#else
  return (rename(prev_path.c_str(), new_path.c_str()) == 0);
#endif
}
std::string ReadFileText(const std::string& path)
{
  size_t fsize = 0;
  FILE *fp = fopen_utf8(path, "rb");
  if (fp)
  {
    char *p = (char*)malloc(10241);
    fsize = fread(p, 1, 10240, fp);
    fclose(fp);
    p[fsize] = 0;
    std::string r(p);
    free(p);
    return r;
  }
  else return std::string();
}
FileData ReadFileData(const std::string& path)
{
  FileData fd;
  size_t fsize = 0;
  fd.m_iLen = 0;
  fd.m_iPos = 0;
  fd.p = 0;
  FILE *fp = fopen_utf8(path, "rb");
  if (fp)
  {
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fd.m_iLen = fsize;
    fd.m_iPos = 0;
    fd.p = (uint8_t*)malloc(fsize);
    fread(fd.p, 1, fsize, fp);
    fclose(fp);
  }
  return fd;
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
std::string upper(const std::string& s) {
  std::string sOut(s);
  std::transform(sOut.begin(), sOut.end(), sOut.begin(), ::toupper);
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

bool IsDirectory(const std::string& fpath)
{
#ifdef WIN32
  std::wstring wfpath;
  DecodeToWStr(fpath, wfpath, E_UTF8);
  struct _stat sb;
  return (_wstat(wfpath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
#else
  struct stat sb;
  return (stat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
#endif
}
bool CreateDirectory(const std::string& path)
{
#ifdef WIN32
  std::wstring wpath;
  DecodeToWStr(path, wpath, E_UTF8);
  return _wmkdir(wpath.c_str()) == 0;
#else
  return mkdir(path.c_str(), 0755) == 0;
#endif
}

#ifdef WIN32
bool DeleteDirectory_internal(const std::wstring& path)
{
  struct _stat sb;
  if (_wstat(path.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode))
  {
    HANDLE hFind;
    WIN32_FIND_DATAW FindFileData;

    WCHAR DirPath[MAX_PATH];
    WCHAR FileName[MAX_PATH];

    wcscpy(DirPath, path.c_str());
    wcscat(DirPath, L"\\*");
    wcscpy(FileName, path.c_str());
    wcscat(FileName, L"\\");

    hFind = FindFirstFileW(DirPath, &FindFileData);
    if (hFind == INVALID_HANDLE_VALUE) return 0;
    wcscpy(DirPath, FileName);

    bool bSearch = true;
    while (bSearch)
    {
      if (FindNextFileW(hFind, &FindFileData))
      {
        if (!(wcscmp(FindFileData.cFileName, L".") && wcscmp(FindFileData.cFileName, L"..")))
          continue;
        wcscpy(FileName, DirPath);
        wcscat(FileName, FindFileData.cFileName);
        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
          // if directory, recursively delete
          if (!DeleteDirectory_internal(FileName))
          {
            FindClose(hFind);
            return 0;
          }
          RemoveDirectoryW(FileName);
        }
        else
        {
          // if file, check permission
          if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
          {
            //_chmod(FileName, _S_IWRITE);
            return 0;
          }
          if (!DeleteFileW(FileName))
          {
            FindClose(hFind);
            return 0;
          }
        }
      }
      else
      {
        if (GetLastError() == ERROR_NO_MORE_FILES)
          bSearch = false;
        else
        {
          FindClose(hFind);
          return 0;
        }
      }
    }
    FindClose(hFind);

    return RemoveDirectoryW(path.c_str()) == TRUE;
  }
  else
  {
    return false;
  }
}
bool DeleteDirectory(const std::string& path)
{
  std::wstring wpath;
  DecodeToWStr(path, wpath, E_UTF8);
  return DeleteDirectory_internal(wpath);
}
#else
bool DeleteDirectory(const std::string& path)
{
  DIR *dir;
  struct dirent *entry;
  char newpath[PATH_MAX];

  if (!path.size()) return false;
  dir = opendir(path.c_str());
  if (dir == NULL) {
    return false;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, "..")) {
      snprintf(newpath, (size_t)PATH_MAX, "%s/%s", dirname, entry->d_name);
      if (entry->d_type == DT_DIR) {
        if (!DeleteDirectory(newpath)) return false;
      }
      else {
        DeleteFile(newpath);
      }
    }
  }
  closedir(dir);
  rmdir(path.c_str());

  return true;
}
#endif

#ifdef WIN32
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive)
{
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATAW ffd;
  std::wstring spec, wpath, mask=L"*";
  std::stack<std::wstring> directories;	// currently going-to-search directories
  std::stack<std::wstring> dir_name;		// name for current directories
  std::wstring curr_dir_name;

  DecodeToWStr(path, wpath, E_UTF8);
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
      if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
      {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          directories.push(wpath + L"\\" + ffd.cFileName);
          dir_name.push(curr_dir_name + ffd.cFileName + L"\\");

          std::string fn;
          EncodeFromWStr(curr_dir_name + ffd.cFileName, fn, E_UTF8);
          vFiles.push_back(std::pair<std::string, int>(fn,0));
        }
        else {
          std::string fn;
          EncodeFromWStr(curr_dir_name + ffd.cFileName, fn, E_UTF8);
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
  fclose(fp);
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
    fd.push_back(std::move(fdat));
  }
  // Don't allow FileData left ptr,
  // as Destroyer will clear it when block is over.
  fdat.p = 0;
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
      fn = ConvertEncodingToUTF8(fn, m_iCodepage);
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
  MD5((unsigned char*)p, iLen, reinterpret_cast<unsigned char*>(out));
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
    return true;
  }
  else return false;
}

}