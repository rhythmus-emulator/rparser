#include "Directory.h"
#include "common.h"
#include <mutex>

#ifdef USE_ZLIB
# define ZIP_STATIC 1
# include "zip.h"
#endif

namespace rparser
{

Directory::Directory()\
  : error_code_(ERROR::NONE), use_alternative_search_(false), ref_cnt_(0), open_status_(0)
{
}

Directory::Directory(const std::string& path)
  : error_code_(ERROR::NONE), use_alternative_search_(false), ref_cnt_(0), open_status_(0)
{
  SetPath(path);
}

Directory::~Directory()
{
  UnloadFiles();
}

bool Directory::Open()
{
  // must set dirpath before open file.
  if (dirpath_.empty())
    return false;

  // clear before open
  Clear(false);

  bool r = doOpen();
  open_status_ = 1;
  return r;
}

bool Directory::Save()
{
  if (IsReadOnly())
    return false;

  if (!doWritePrepare()) return false;

  bool s = true;
  for (auto &fds : *this)
  {
    s &= doWrite(fds);
  }

  return s;
}

bool Directory::SaveAs(const std::string& filepath)
{
  // XXX: should save previous path?
  SetPath(filepath.c_str());

  if (!doCreate(filepath))
    return false;

  // read all file before save to other directory
  // and set dirty flag to force save all files.
  if (!ReadAll()) return false;

  return Save();
}

bool Directory::Clear(bool flush)
{
  if (!Close(flush))
    return false;

  UnloadFiles();

  return true;
}

void Directory::UnloadFiles()
{
  files_.clear();
}

bool Directory::Close(bool flush)
{
  // just leave cache
  if (flush && !Save())
    return false;

  ClearStatus();

  return doClose();
}

bool Directory::IsReadOnly()
{
  return true;
}

bool Directory::GetFile(const std::string& filename, const char** out, size_t &len) const noexcept
{
  const File *f = nullptr;

  for (auto &fds : *this)
  {
    if (fds.filename == filename)
    {
      f = &fds;
      break;
    }
  }

  if (use_alternative_search_)
  {
    const std::string filenameonly(std::move(rutil::GetAlternativeFilename(filename)));
    for (auto &fds : *this)
    {
      if (rutil::GetAlternativeFilename(fds.filename) == filenameonly)
      {
        f = &fds;
        break;
      }
    }
  }

  if (!f)
    return false;

  // If file is not read, then attempt to read.
  // do const_cast as we know what we're doing...
  if (!f->p)
  {
    if (!const_cast<Directory*>(this)->doRead(*const_cast<File*>(f)))
      return false;
  }

  *out = f->p;
  len = f->len;
  return true;
}

void Directory::SetFile(const std::string& filename, const char* p, size_t len) noexcept
{
  File *f = nullptr;

  // search file. if not found, create one.
  for (auto &fd : *this)
  {
    if (fd.filename == filename)
    {
      f = &fd;
      break;
    }
  }

  if (use_alternative_search_)
  {
    const std::string filenameonly(std::move(rutil::GetAlternativeFilename(filename)));
    for (auto &fds : *this)
    {
      if (rutil::GetAlternativeFilename(fds.filename) == filenameonly)
      {
        f = &fds;
        break;
      }
    }
  }

  if (!f)
  {
    files_.emplace_back(File{ filename, 0, 0 });
    f = &files_.back();
  }
  else if (f->p)
    free(f->p);

  f->p = (char*)malloc(len);
  memcpy(f->p, p, len);
  f->len = len;
}

/*
 * do... functions DO real file system workaround.
 */

bool Directory::doRead(File&fd)
{
  return false;
};

bool Directory::doWritePrepare()
{
  return false;
}

bool Directory::doWrite(File &d)
{
  return false;
}

bool Directory::doRename(const std::string& oldname, const std::string& newname)
{
  return false;
}

bool Directory::doOpen()
{
  SetError(ERROR::UNKNOWN);
  return false;
};

bool Directory::doClose()
{
  // nothing to do with unloading in basic unload module
  return true;
}

bool Directory::doDelete(const std::string& filename)
{
  return false;
}

bool Directory::doCreate(const std::string& newpath)
{
  return false;
}

void Directory::ClearStatus()
{
  error_code_ = ERROR::NONE;
  open_status_ = 0;
}

void Directory::CreateEmptyFile(const std::string& filename)
{
  // as it's internal function, don't check is there duplication
  files_.emplace_back(File{ filename, 0, 0 });
}

void Directory::SetPath(const std::string& filepath)
{
  dirpath_ = rutil::CleanPath(filepath);
}

const std::string& Directory::GetPath() const
{
  return dirpath_;
}

void Directory::SetAlternativeSearch(bool use_alternative_search)
{
  use_alternative_search_ = use_alternative_search;
}

std::string Directory::GetRelativePath(const std::string &orgpath) const
{
  return orgpath.substr(dirpath_.size() + 1);
}

std::string Directory::GetAbsolutePath(const std::string &relpath) const
{
  return dirpath_ + "\\" + relpath;
}

const char * Directory::GetErrorMsg() const
{
  return get_error_msg(error_code_);
}

bool Directory::IsLoaded()
{
  return (open_status_ > 0);
}

bool Directory::AddExternalFile(const std::string &path, const std::string& new_name)
{
  if (IsReadOnly() || new_name.empty())
    return false;

  rutil::FileData d;
  rutil::ReadFileData(path, d);
  if (d.len == 0)
  {
    SetError(ERROR::OPEN_INVALID_FILE);
    return false;
  }
  SetFile(new_name, (char*)d.p, d.len);
  return true;
}

bool Directory::Exist(const std::string & name) const
{
  const char* p;
  size_t len;
  return GetFile(name, &p, len);
}

bool Directory::ReadAll(bool force)
{
  if (IsReadOnly())
    return false;

  bool r = true;
  for (auto &fds : *this)
  {
    if (fds.len > 0 && !force)
      continue;

    r |= doRead(fds);
  }
  return r;
}

bool Directory::Read(const std::string& filename, bool force)
{
  for (auto &fds : *this)
  {
    if (fds.filename == filename)
    {
      if (fds.len > 0 && !force)
        continue;
      return doRead(fds);
    }
  }
  return false;
}

bool Directory::Delete(const std::string &filename)
{
  if (IsReadOnly())
    return false;

  // fetch file ptr
  File* fds = nullptr;
  for (auto &fd : *this) if (fd.filename == filename)
  {
    fds = &fd;
    break;
  }
  if (!fds)
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }

  // affects file system instantly.
  if (!doDelete(fds->filename))
  {
    SetError(ERROR::DELETE_NO_PATH);
    return false;
  }

  files_.erase(files_.begin() + std::distance(files_.data(), fds));
  return true;
}

bool Directory::Rename(const std::string& prev_name, const std::string& new_name)
{
  if (IsReadOnly())
    return false;

  // fetch file ptr
  File* fds = nullptr;
  for (auto &fd : *this) if (fd.filename == prev_name)
  {
    fds = &fd;
    break;
  }
  if (!fds)
  {
    SetError(ERROR::OPEN_NO_FILE);
    return false;
  }

  // affects file system instantly.
  if (!doRename(fds->filename, new_name))
  {
    SetError(ERROR::WRITE_RENAME);
    return false;
  }

  fds->filename = new_name;
  return true;
}

Directory::data_iter Directory::begin() noexcept
{
  return files_.begin();
}

Directory::data_iter Directory::end() noexcept
{
  return files_.end();
}

const Directory::data_constiter Directory::begin() const noexcept
{
  return files_.cbegin();
}

const Directory::data_constiter Directory::end() const noexcept
{
  return files_.cend();
}

size_t Directory::count() const noexcept
{
  return files_.size();
}

void Directory::SetError(ERROR error)
{
  error_code_ = error;
}

DirectoryFolder::DirectoryFolder() {}

DirectoryFolder::DirectoryFolder(const std::string& path) : Directory(path) {}

bool DirectoryFolder::IsReadOnly()
{
  return false;
}

bool DirectoryFolder::doOpen()
{
  if (!rutil::IsDirectory(GetPath()))
    return false;

  return doOpenDirectory(GetPath());
}

bool DirectoryFolder::doOpenDirectory(const std::string& dirpath)
{
  rutil::DirFileList files;
  rutil::GetDirectoryFiles(dirpath, files);
  for (auto ii : files)
  {
    CreateEmptyFile(ii.first);
  }

  return true;
}

bool DirectoryFolder::doRead(File &f)
{
  const std::string fullpath(GetPath() + "/" + f.filename);
  rutil::FileData d;
  rutil::ReadFileData(fullpath, d);
  if (d.IsEmpty())
  {
    SetError(ERROR::OPEN_INVALID_FILE);
    return false;
  }
  // move file data ...
  f.p = (char*)d.p;
  f.len = d.len;
  d.p = 0;
  d.len = 0;
  return true;
}

bool DirectoryFolder::doRename(const std::string& oldname, const std::string& newname)
{
  const std::string fulloldpath(GetPath() + "/" + oldname);
  const std::string fullnewpath(GetPath() + "/" + newname);
  if (!rutil::Rename(fulloldpath, fullnewpath))
  {
    SetError(ERROR::WRITE_RENAME);
    return false;
  }
  return true;
}

bool DirectoryFolder::doDelete(const std::string& filename)
{
  const std::string fullpath(GetPath() + "/" + filename);
  if (!rutil::DeleteFile(fullpath.c_str()))
  {
    SetError(ERROR::DELETE_NO_PATH);
    return false;
  }
  return true;
}

bool DirectoryFolder::doCreate(const std::string& newpath)
{
  // check folder is valid and attempt to create
  const std::string dirpath = GetPath();
  if (!rutil::IsDirectory(dirpath) && !rutil::CreateDirectory(dirpath))
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }
  return true;
}

bool DirectoryFolder::doWritePrepare()
{
  return true;
}

bool DirectoryFolder::doWrite(File& f)
{
  // make full path before save data
  std::string fullpath = GetPath() + "/" + f.filename;

  // TODO: need to refactory FileData API
  bool r = false;
  {
    rutil::FileData fd;
    fd.p = (uint8_t*)f.p;
    fd.len = f.len;
    r = rutil::WriteFileData(fd);
    fd.p = 0;
    fd.len = 0;
  }

  if (!r) SetError(ERROR::WRITE_NO_PATH);
  return r;
}

void DirectoryArchive::SetCodepage(int codepage)
{
  codepage_ = codepage;
}

#ifdef USE_ZLIB

DirectoryArchive::DirectoryArchive() : codepage_(0), archive_(0), zip_error_(0)
{}

DirectoryArchive::DirectoryArchive(const std::string& path)
  : DirectoryFolder(path), codepage_(0), archive_(0), zip_error_(0)
{}

bool DirectoryArchive::IsReadOnly()
{
  return archive_ == 0;
}

bool DirectoryArchive::doOpen()
{
  Close();
  archive_ = zip_open(GetPath().c_str(), ZIP_RDONLY, &zip_error_);
  if (zip_error_)
  {
    zip_error_t zerror;
    zip_error_init_with_code(&zerror, zip_error_);
    printf("Zip Reading Error occured - code %d\nstr (%s)\n", zip_error_, zip_error_strerror(&zerror));
    zip_error_fini(&zerror);
    return zip_error_;
  }
  // get all file lists from zip, with encoding
  zip_int64_t iEntries = zip_get_num_entries(archive_, ZIP_FL_UNCHANGED);
  struct zip_stat zStat;
  for (int i = 0; i<iEntries; i++)
  {
    zip_stat_index(archive_, i, ZIP_FL_UNCHANGED, &zStat);
    std::string fn = zStat.name;
    if (codepage_)
    {
      fn = rutil::ConvertEncodingToUTF8(fn, codepage_);
    }
    CreateEmptyFile(fn);
  }
  return true;
}

bool DirectoryArchive::doClose()
{
  zip_close(archive_);
  archive_ = 0;
  return true;
}

bool DirectoryArchive::doWritePrepare()
{
  return true;
}

bool DirectoryArchive::doWrite(File &f)
{
  if (!archive_) return false;
  zip_source_t *s;
  s = zip_source_buffer(archive_, f.p, f.len, 0);
  if (!s)
  {
    printf("Zip source buffer creation failed!\n");
    return false;
  }
  zip_error_ = (int)zip_file_add(archive_, f.filename.c_str(), s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
  zip_source_free(s);
  if (zip_error_)
  {
    printf("Zip file appending failed! (code %d)\n", zip_error_);
    return false;
  }
  return true;
}

bool DirectoryArchive::doRename(const std::string& oldname, const std::string& newname)
{
  if (!archive_) return false;
  // TODO zip element rename
  return false;
}

bool DirectoryArchive::doRead(File &f)
{
  if (!archive_) return false;

  zip_file_t *zfp = nullptr;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    zfp = zip_fopen(archive_, f.filename.c_str(), ZIP_FL_UNCHANGED);
    if (!zfp)
    {
      printf("Failed to read file(%s) from zip\n", f.filename.c_str());
      return false;
    }
  }

  struct zip_stat zStat;
  zip_stat(archive_, f.filename.c_str(), 0, &zStat);
  f.len = (size_t)zStat.size;
  f.p = (char*)malloc(f.len);
  zip_fread(zfp, (void*)f.p, f.len);
  zip_fclose(zfp);
  return true;
}

bool DirectoryArchive::doDelete(const std::string& filename)
{
  if (!archive_) return false;
  // TODO: Delete object from archive
  return true;
}

bool DirectoryArchive::doCreate(const std::string& newpath)
{
  // TODO: create new zip 'object'.
  return true;
}

#else

DirectoryArchive::DirectoryArchive() {  }

DirectoryArchive::DirectoryArchive(const std::string& path)
  : DirectoryFolder(path) {  }

bool DirectoryArchive::IsReadOnly()
{
  return true;
}

bool DirectoryArchive::doRead(File &f) { return false; }
bool DirectoryArchive::doWritePrepare() { return false; }
bool DirectoryArchive::doWrite(File &f) { return false; }
bool DirectoryArchive::doRename(const std::string& oldname, const std::string& newname)
{
  return false;
}
bool DirectoryArchive::doOpen() { return false; }
bool DirectoryArchive::doClose() { return false; }
bool DirectoryArchive::doDelete(const std::string& filename) { return false; }
bool DirectoryArchive::doCreate(const std::string& newpath) { return false; }

#endif


// ----------------- Misc for DirectoryManager

Directory* general_dir_constructor(const char* path)
{
  Directory *d = new DirectoryFolder(path);
  return d;
}

Directory* zip_dir_constructor(const char* path)
{
  Directory *d = new DirectoryArchive(path);
  return d;
}

std::mutex dir_mutex;

// --------------------- class DirectoryManager

bool DirectoryManager::OpenDirectory(const std::string& dirpath)
{
  std::string spath = GetSafePath(dirpath);

  /* create directory path object, without reading directory.
   * we don't care failure here (as duplicate one may existing already) */
  CreateDirectory(spath);
  auto dir = GetDirectory(spath);
  if (dir == nullptr)
    return false; /* even failed to create directory object. */

  /* now read directory. if fail, release object and exit */
  if (!dir->Open())
  {
    CloseDirectory(spath);
    return false;
  }
  dir->ref_cnt_++;

  return true;
}

bool DirectoryManager::CreateDirectory(const std::string& dirpath)
{
  DirectoryManager& m = getInstance();

  dir_mutex.lock();
  std::string spath = GetSafePath(dirpath);

  // check already existing, if so close it first.
  auto i = m.dir_container_.find(spath);
  if (i != m.dir_container_.end())
  {
    dir_mutex.unlock();
    return false;
  }

  // add new directory object
  Directory* dir = m.CreateDirectoryObject(spath);
  if (dir == nullptr)
  {
    // unsupported type directory? this should be happened?
    delete dir;
    dir_mutex.unlock();
    return false;
  }
  m.dir_container_[spath].reset(dir);

  dir_mutex.unlock();
  return true;
}

bool DirectoryManager::ReadDirectoryFiles(const std::string& dirpath)
{
  OpenDirectory(dirpath);
  auto dir = GetDirectory(dirpath);
  if (!dir)
    return false;
  dir->ReadAll();
  return true;
}

bool DirectoryManager::SaveDirectory(const std::string& dirpath)
{
  auto dir = GetDirectory(dirpath);
  if (!dir)
    return false;
  dir->Save();
  return true;
}

bool DirectoryManager::IsDirectoryOpened(const std::string& dirpath)
{
  DirectoryManager& m = getInstance();
  std::string spath = GetSafePath(dirpath);
  auto i = m.dir_container_.find(spath);
  return (i != m.dir_container_.end());
}

void DirectoryManager::CloseDirectory(const std::string& dirpath, bool force)
{
  DirectoryManager& m = getInstance();

  dir_mutex.lock();

  // check opened
  auto i = m.dir_container_.find(GetSafePath(dirpath));
  if (i == m.dir_container_.end())
  {
    dir_mutex.unlock();
    return;
  }

  Directory& dir = *i->second;
  if (--dir.ref_cnt_ <= 0 || force)
    m.dir_container_.erase(i);

  dir_mutex.unlock();
}

std::shared_ptr<Directory> DirectoryManager::GetDirectory(const std::string& dirpath)
{
  auto &m = getInstance();

  dir_mutex.lock();
  auto i = m.dir_container_.find(GetSafePath(dirpath));
  dir_mutex.unlock();

  if (i == m.dir_container_.end())
    return nullptr;
  else return i->second;
}

bool DirectoryManager::GetFile(const std::string& filepath,
  const char** out, size_t &len)
{
  std::string dn, fn;
  SeparatePath(filepath, dn, fn);
  const auto dir = GetDirectory(dn);
  if (!dir)
    return false;
  return dir->GetFile(fn, out, len);
}

bool DirectoryManager::CopyFile(const std::string& filepath,
  char** out, size_t &len)
{
  const char* p;
  if (!GetFile(filepath, &p, len))
    return false;
  *out = (char*)malloc(len);
  memcpy(*out, p, len);
  return true;
}

void DirectoryManager::SetFile(const std::string& filepath,
  const char* out, size_t len)
{
  std::string dn, fn;
  SeparatePath(filepath, dn, fn);
  auto dir = GetDirectory(dn);
  if (!dir)
    return;
  dir->SetFile(fn, out, len);
}

bool DirectoryManager::OpenAndGetFile(const std::string& filepath,
  const char** out, size_t &len)
{
  std::string dir, fn;
  SeparatePath(filepath, dir, fn);
  if (!OpenDirectory(dir))
    return false;
  return GetFile(fn, out, len);
}

bool DirectoryManager::OpenAndCopyFile(const std::string& filepath,
  char** out, size_t &len)
{
  std::string dir, fn;
  SeparatePath(filepath, dir, fn);
  if (!OpenDirectory(dir))
    return false;
  return CopyFile(fn, out, len);
}

void DirectoryManager::CloseFile(const std::string& filepath)
{
  std::string dir = GetSafeDirectory(filepath);
  CloseDirectory(dir);
}

void DirectoryManager::AddDirectoryConstructor(const std::string& ext, dir_constructor constructor)
{
  ext_dirconstructor_map_[ext] = constructor;
}

DirectoryManager::DirectoryManager()
{
  ext_dirconstructor_map_["Directory"] = general_dir_constructor;
  ext_dirconstructor_map_["zip"] = zip_dir_constructor;
}

DirectoryManager::~DirectoryManager() {}

std::string DirectoryManager::GetSafePath(const std::string& path)
{
  std::string r = path;
  for (size_t i = 0; i < r.size(); ++i)
    if (r[i] == '\\') r[i] = '/';
  return r;
}

void DirectoryManager::SeparatePath(const std::string& path, std::string& dir_out, std::string& fn_out)
{
  std::string spath = GetSafePath(path);
  size_t i = spath.find('|');
  if (i == std::string::npos)
  {
    i = spath.find_last_of('/');
    if (i == std::string::npos)
    {
      // bad!
      dir_out = "";
      fn_out = spath;
      return;
    }
  }
  dir_out = spath.substr(0, i);
  fn_out = spath.substr(i + 1);
}

std::string DirectoryManager::GetSafeDirectory(const std::string& dir_or_filepath)
{
  std::string spath = GetSafePath(dir_or_filepath);
  size_t i = spath.find('|');
  if (i == std::string::npos &&
      (rutil::IsDirectory(spath) ||
      rutil::GetExtension(spath) == "zip"))
    return spath;
  else
  {
    std::string dn, fn;
    SeparatePath(spath, dn, fn);
    return dn;
  }
}

Directory* DirectoryManager::CreateDirectoryObject(const std::string& dirpath)
{
  std::string ext = rutil::GetExtension(dirpath);
  if (ext.empty())
    ext = "Directory";
  auto i = ext_dirconstructor_map_.find(ext);
  if (i == ext_dirconstructor_map_.end())
    return nullptr;
  else return i->second(dirpath.c_str());
}

DirectoryManager& DirectoryManager::getInstance()
{
  static DirectoryManager m;
  return m;
}

}
