#include "rutil.h"
#include <string>
#include <sstream>
#include <algorithm>
#include <stack>
#include <cctype>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef WIN32
# include <io.h>
# include <sys/types.h>
# define stat _wstat
#else
# include <dirent.h>
# include <unistd.h>
# include <iconv.h>
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
  case E_UTF8:  // utf-8
    return "utf-8";
  case E_UTF32:
    return "utf-32";
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
  if (!cp_from_str || !cp_to_str) return std::string();

  iconv_t converter = iconv_open( cp_to_str, cp_from_str );
  if( converter == (iconv_t) -1 )
  {
    return std::string();
  }

  /* Copy the string into a char* for iconv */
  const char *szTextIn = const_cast<const char*>( s.data() );
  size_t iInLeft = s.size();

  /* Create a new string with enough room for the new conversion */
  std::string sBuf;
  sBuf.resize( s.size() * 5 );

  char *sTextOut = const_cast<char*>( sBuf.data() );
  size_t iOutLeft = sBuf.size();
  size_t size = iconv( converter, const_cast<char**>(&szTextIn), &iInLeft, &sTextOut, &iOutLeft );

  iconv_close( converter );

  if( size == (size_t)(-1) )
  {
    return std::string(); /* Returned an error */
  }

  if( iInLeft != 0 )
  {
    printf("iconv(%s to %s) for \"%s\": whole buffer not converted",cp_from_str, cp_to_str, s.c_str());
    return std::string();
  }

  if( sBuf.size() == iOutLeft )
    return std::string(); /* Conversion failed */

  sBuf.resize( sBuf.size()-iOutLeft );

  return sBuf;
}
int DecodeTo(std::string &s, int to_codepage)
{
  s = ConvertEncoding(s, to_codepage, E_UTF8);
  return s.size() > 0;
}
FILE* fopen_utf8(const char* fname, const char* mode)
{
  // re-process filename for linux
  std::string fname_unix;
  while (*fname)
  {
    if (
    // windows separator
       (*fname == '\\')
    // YEN sign
    || (static_cast<uint8_t>(*fname) == 194 && 
        static_cast<uint8_t>(*(fname+1)) == 165)
       ) fname_unix.push_back('/');
    else fname_unix.push_back(*fname);
    fname++;
  }
  return fopen(fname_unix.c_str(), mode);
}
FILE* fopen_utf8(const std::string& fname, const std::string& mode)
{
  return fopen_utf8(fname.c_str(), mode.c_str());
}
int printf_utf8(const char* fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  return vprintf(fmt, args);
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
uint32_t UTF16toUTF32(uint16_t codepoint)
{
  // constants
  constexpr uint32_t LEAD_OFFSET = 0xD800 - (0x10000 >> 10);
  constexpr uint32_t SURROGATE_OFFSET = 0x10000 - (0xD800 << 10) - 0xDC00;

  // computations
  uint32_t lead = LEAD_OFFSET + (codepoint >> 10);
  uint32_t trail = 0xDC00 + (codepoint & 0x3FF);

  return (lead << 10) + trail + SURROGATE_OFFSET;
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
    return std::string();

  // just return result string if UTF16
  if (to_codepage == E_UTF16)
  {
    const char* p_start = reinterpret_cast<const char*>(sWstr.c_str());
    const char* p_end = p_start + sWstr.size() * 2;
    return std::string(p_start, p_end);
  }

  // little more tricks in case of UTF32
  if (to_codepage == E_UTF32)
  {
    std::string r;
    for (uint16_t t : sWstr)
    {
      uint32_t v = UTF16toUTF32(t);
      r.push_back(((char*)&v)[0]);
      r.push_back(((char*)&v)[1]);
      r.push_back(((char*)&v)[2]);
      r.push_back(((char*)&v)[3]);
    }
    return r;
  }

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
  return (stat(fpath.c_str(), &sb) == 0 && S_ISREG(sb.st_mode));
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
  FILE *fp = fopen_utf8(path.c_str(), "rb");
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
void ReadFileData(const std::string& path, FileData& fd)
{
  size_t fsize = 0;
  fd.len = 0;
  fd.pos = 0;
  fd.p = 0;
  fd.fn = std::move(path);
  FILE *fp = fopen_utf8(path.c_str(), "rb");
  if (fp)
  {
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fd.len = fsize;
    fd.pos = 0;
    fd.p = (uint8_t*)malloc(fsize);
    fread(fd.p, 1, fsize, fp);
    fclose(fp);
  }
}

bool WriteFileData(const FileData& fd)
{
  FILE *fp = fopen_utf8(fd.fn.c_str(), "wb");
  bool r = false;
  if (fp)
  {
    r = (fd.len == fwrite(fd.p, fd.len, 1, fp));
    fclose(fp);
  }
  return r;
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
  if (sSep == str.end())
  {
    s1 = str;
    return 0;
  }
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
    //  c = '/';
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
  if (IsDirectory(path)) return path;
  size_t pos = path.find_last_of('/');
  if (pos == std::string::npos)
    return "";
  return std::string(path, 0, pos);
}

std::string GetFilename(const std::string & path)
{
  size_t pos = path.find_last_of('/');
  if (pos == std::string::npos)
    return path;
  return std::string(path.c_str()+pos+1);
}

std::string GetAlternativeFilename(const std::string& path)
{
  std::string fn = GetFilename(path);
  size_t pos = fn.find_last_of('.');
  if (pos == std::string::npos)
    return fn;
  return fn.substr(0, pos);
}

std::string GetExtension(const std::string& path, std::string *sOutName)
{
  size_t pos = path.find_last_of('.');
  size_t pos_dir = path.find_last_of('/');
  if (pos_dir != std::string::npos && pos < pos_dir /* in case of ../ */ ||
      pos == std::string::npos)
    return "";
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
  const std::string&& ext = std::move(GetExtension(path));
  const std::string&& ext_lower = std::move(lower(ext));
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
  return (stat(fpath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode));
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
      snprintf(newpath, (size_t)PATH_MAX, "%s/%s", path.c_str(), entry->d_name);
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
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive, bool file_only)
{
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATAW ffd;
  std::wstring spec, wpath, mask=L"*";
  std::stack<std::wstring> directories; // currently going-to-search directories
  std::stack<std::wstring> dir_name;    // name for current directories
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
          if (!file_only)
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
bool GetDirectoryFiles(const std::string& path, DirFileList& vFiles, int maxrecursive, bool file_only)
{
  DIR *dp;
  struct dirent *dirp;
  std::stack<std::string> directories;
  std::stack<std::string> dir_name;

  directories.push(path);
  dir_name.push("");

  std::string curr_dir_name;
  std::string curr_dir;

  while (maxrecursive >= 0 && directories.size() > 0)
  {
    maxrecursive--;
    curr_dir = directories.top();
    curr_dir_name = dir_name.top();
    directories.pop();
    dir_name.pop();
    if (curr_dir_name == "./" || curr_dir_name == "../")
      continue;
    if((dp  = opendir(curr_dir.c_str())) == NULL) {
      printf("Error opening %s dir.\n", curr_dir.c_str());
      return false;
    }
    while ((dirp = readdir(dp)) != NULL) {
      if (dirp->d_type == DT_DIR)
      {
        directories.push(curr_dir + "/" + dirp->d_name);
        dir_name.push(curr_dir_name + dirp->d_name + "/");
        if (!file_only)
          vFiles.push_back(std::pair<std::string, int>(curr_dir_name + std::string(dirp->d_name), 0));
      }
      else if (dirp->d_type == DT_REG)
      {
        vFiles.push_back(std::pair<std::string, int>(curr_dir_name + std::string(dirp->d_name), 1));
      }
    }
    closedir(dp);
  }

  return true;
}
#endif

struct DirMaskContext
{
  std::string dir_mask;
  bool is_mask;
  bool is_last;
  bool only_file;
};

void GetDirectoryEntriesWithMaskContext(std::vector<std::string>& paths, const DirMaskContext& mask_ctx)
{
  std::vector<std::string> new_paths;
  std::string new_path;
  DirFileList fl;

  for (const auto& path : paths)
  {
    /* more simple way to do, if it's not mask. */
    if (!mask_ctx.is_mask)
    {
      if (path == ".")
        new_path = mask_ctx.dir_mask;
      else
        new_path = path + '/' + mask_ctx.dir_mask;

      if (!mask_ctx.is_last)
      {
        if (IsDirectory(new_path))
          new_paths.push_back(new_path);
      }
      else
      {
        if (IsFile(new_path) || (IsDirectory(new_path) && !mask_ctx.only_file))
          new_paths.push_back(new_path);
      }
      continue;
    }

    /* if mask, then enum all file & do wild match */
    fl.clear();
    GetDirectoryFiles(path, fl, 0, mask_ctx.only_file);
    for (auto& file : fl)
    {
      std::string filename = file.first; //file.first.substr(path.size() + 1);
      if (wild_match(filename, mask_ctx.dir_mask))
      {
        // if it's not last, then only add directory
        if (!mask_ctx.is_last && file.second == 1)
          continue;
        new_paths.push_back(path + '/' + filename);
      }
    }
  }

  paths.swap(new_paths);
}

void GetFileInfo(const std::vector<std::string> &files, std::vector<FileInfo>& out)
{
  FileInfo fi;
  for (const auto& file : files)
  {
#ifdef WIN32
    struct _stat result;
    std::wstring wfn;
    DecodeToWStr(file, wfn, E_UTF8);
    if (_wstat(wfn.c_str(), &result) == 1)
      continue;
#else
    struct stat result;
    if (stat(file.c_str(), &result) == 1)
      continue;
#endif
    if (result.st_mode & S_IFREG)
      fi.entry_type = 1;
    else if (result.st_mode & S_IFDIR)
      fi.entry_type = 2;
    else continue;
    fi.modified_timestamp = result.st_mtime;
    fi.path = file;
    out.push_back(fi);
  }
}

void GetDirectoryEntriesMasking(const std::string& path,
  std::vector<std::string>& out,
  bool only_file)
{
  std::vector<DirMaskContext> mask_ctx;

  // create mask context
  {
    std::string path_new = path;
    size_t s = 0;
    for (size_t i = 0; i < path_new.size(); ++i)
      if (path_new[i] == '\\') path_new[i] = '/';

    if (path_new.back() != '/')
      path_new.push_back('/');

    for (size_t i = 0; i < path_new.size(); ++i)
    {
      if (path_new[i] == '\\') path_new[i] = '/';
      if (path_new[i] == '/')
      {
        if (s == i) /* dir mark at very beginning */
          continue;
        std::string dir_mask = path_new.substr(s, i - s);
        bool is_mask = (dir_mask.find('*') != std::string::npos);
        /**
         * An optimizing to reduce mask context:
         * - if current is not mask and previous is not mask,
         *   then don't add new mask; just add it to previous one.
         * - if prev is mask and current is not mask, add new one.
         * - if current is mask, add new one.
         */
        if (!mask_ctx.empty() && !mask_ctx.back().is_mask && !is_mask)
        {
          mask_ctx.back().dir_mask += "/" + dir_mask;
        }
        else
        {
          mask_ctx.emplace_back(DirMaskContext{
            dir_mask, is_mask, false, false
            });
        }
        s = i + 1;
      }
    }
  }

  if (mask_ctx.empty())
    return;
  mask_ctx.back().is_last = true;
  mask_ctx.back().only_file = only_file;
  out.push_back(".");

  for (const auto& m : mask_ctx)
  {
    GetDirectoryEntriesWithMaskContext(out, m);
  }
}

bool wild_match(const std::string& str, const std::string& pat) {
  std::string::const_iterator str_it = str.begin();
  for (std::string::const_iterator pat_it = pat.begin(); pat_it != pat.end();
    ++pat_it) {
    switch (*pat_it) {
    case '?':
      if (str_it == str.end()) {
        return false;
      }

      ++str_it;
      break;
    case '*': {
      if (pat_it + 1 == pat.end()) {
        return true;
      }

      const size_t max = strlen(&*str_it);
      for (size_t i = 0; i < max; ++i) {
        if (wild_match(&*(str_it + i), &*(pat_it + 1))) {
          return true;
        }
      }

      return false;
    }
    default:
      if (*str_it != *pat_it) {
        return false;
      }

      ++str_it;
    }
  }

  return str_it == str.end();
}

FileData::FileData()
{
  p = 0;
  len = 0;
  pos = 0;
}

FileData::FileData(const std::string& fn)
{
  this->fn = fn;
  p = 0;
  len = 0;
  pos = 0;
}

// only for seeking purpose
FileData::FileData(uint8_t *p, uint32_t iLen)
{
  this->p = p;
  len = iLen;
  pos = 0;
}

// for vector resizing
FileData::FileData(FileData &&fd)
{
  p = fd.p;
  len = fd.len;
  pos = fd.pos;
  fn = fd.fn;
  fd.p = 0;
  fd.len = fd.pos = 0;
}

FileData::~FileData()
{
  if (p)
  {
    free(p);
  }
}

std::string FileData::GetFilename() const { return fn; }
uint32_t FileData::GetFileSize() const { return len; }
uint32_t FileData::GetPos() const { return pos; }
void FileData::SetPos(uint32_t iPos) { pos = iPos; }
const uint8_t* FileData::GetPtr() const { return p; }
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
    iNewPos = pos + p;
    break;
  case SEEK_END:
    iNewPos = len - p;
    break;
  case SEEK_SET:
    iNewPos = p;
    break;
  }
  //if (m_iPos < 0) m_iPos = 0;
  if (iNewPos > len) return -1;
  else pos = iNewPos;
  return 0;
}

uint32_t FileData::ReadLE32()
{
  uint32_t v = rutil::ReadLE32(p + pos);
  SeekCur(4);
  if (pos > len) pos = len;
  return v;
}

uint32_t FileData::Read(uint8_t *out, uint32_t len)
{
  uint32_t copysize = len;
  if (pos + len > this->len)
  {
    copysize = this->len - pos;
  }
  memcpy(out, p + pos, copysize);
  pos += copysize;
  return copysize;
}

bool FileData::IsEOF() { return pos == len; };

bool FileData::IsEmpty() { return p == 0; };

bool md5(const void* p, int iLen, unsigned char* out)
{
#ifdef USE_OPENSSL
  MD5((unsigned char*)p, iLen, reinterpret_cast<unsigned char*>(out));
  return true;
#else
  for (size_t i = 0; i < 32; ++i)
    out[i] = '0';
  out[32] = 0;
  return false;
#endif
}

bool md5_str(const void* p, int iLen, char *out)
{
#ifndef USE_OPENSSL
#define MD5_DIGEST_LENGTH 16
#endif
  unsigned char result[MD5_DIGEST_LENGTH];
  if (md5(p, iLen, result))
  {
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
      sprintf(out, "%02x", result[i]);
      out += 2;
    }
    return true;
  }
  else return false;
}

std::string md5_str(const void* p, int iLen)
{
  char s[33];
  s[0] = s[32] = 0;
  md5_str(p, iLen, s);
  return std::string(s);
}

char h2c(char x)
{
  if (x >= '0' && x <= '9')
    return x - '0';
  else if (x >= 'A' && x <= 'F')
    return x - 'A' + 10;
  else if (x >= 'a' && x <= 'f')
    return x - 'a' + 10;
  else return 0;
}

char c2h(char x)
{
  if (x < 10) return x + '0';
  else if (x < 16) return x + 'a';
  else return c2h(x % 16);
}

std::string md5_sum(const std::string &s1, const std::string &s2)
{
  const char *c1 = s1.c_str();
  const char *c2 = s2.c_str();
  std::string r;
  while (*c1 && *c2)
    r.push_back(c2h((h2c(*c1++) + h2c(*c2++)) % 16));
  while (r.size() < 32)
    r.push_back('0');
  return r;
}

#ifdef WIN32
char *itoa(int value, char *str, int base)
{
  return _itoa(value, str, base);
}

char *gcvt(double value, int digits, char *string)
{
  return _gcvt(value, digits, string);
}
#else
char *itoa(int value, char *str, int base)
{
  int sum = value;
  int i = 0;
  int ilen = 0;
  int digit;
  char s;
  do
  {
    digit = sum % base;
    if (digit < 0xA)
      str[i++] = '0' + digit;
    else
      str[i++] = 'A' + digit - 0xA;
    sum /= base;
  } while (sum/* && (i < (len - 1))*/);
  //if (i == (len - 1) && sum)
  //  return -1;
  str[i] = '\0';
  ilen = i;
  for (i=0; i<ilen/2; i++)
  {
    s = str[i];
    str[ilen-1-i] = str[i];
    str[ilen-1-i] = s;
  }
  return 0;
}

char *gcvt(double value, int ndigits, char *buf)
{
  char *p = buf;

  sprintf (buf, "%-#.*g", ndigits, value);

  /* It seems they expect us to return .XXXX instead of 0.XXXX  */
  if (*p == '-')
    p++;
  if (*p == '0' && p[1] == '.')
    memmove (p, p + 1, strlen (p + 1) + 1);

  /* They want Xe-YY, not X.e-YY, and XXXX instead of XXXX.  */
  p = strchr (buf, 'e');
  if (!p)
  {
    p = buf + strlen (buf);
    /* They don't want trailing zeroes.  */
    while (p[-1] == '0' && p > buf + 2)
      *--p = '\0';
  }
  if (p > buf && p[-1] == '.')
    memmove (p - 1, p, strlen (p) + 1);
  return buf;
}
#endif

long atoi_16(const char* str, unsigned int len)
{
  if (len == 0) return strtol(str, NULL, 16);
  else {
    std::string v(str, len);
    return strtol(v.c_str(), NULL, 16);
  }
}


Random::Random()
{
  SetSeedByTime();
  this->dist_real_ = std::uniform_real_distribution<double_t>();
  this->dist_int_ = std::uniform_int_distribution<int32_t>();
}

void Random::SetSeed(uint32_t seed)
{
  seed_ = seed;
  this->randomgenerator_ = std::mt19937(seed_);
}

uint32_t Random::GetSeed() const
{
  return seed_;
}

void Random::SetSeedByTime()
{
  SetSeed((uint32_t)time(0));
}

int32_t Random::Next()
{
  return dist_int_(randomgenerator_);
}

double Random::NextDouble()
{
  return dist_real_(randomgenerator_);
}

int32_t Random::Next(int32_t min, int32_t max)
{
  std::uniform_int_distribution<int32_t> distribution(min, max);
  return distribution(randomgenerator_);
}

double Random::NextDouble(double minValue, double maxValue)
{
  std::uniform_real_distribution<double_t> distribution(minValue, minValue);
  return distribution(randomgenerator_);
}

}
