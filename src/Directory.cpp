#include "Directory.h"
#include "common.h"

namespace rparser
{

Directory::Directory() : Directory(DIRECTORY_TYPE::NONE)
{
}

Directory::Directory(DIRECTORY_TYPE restype) :
  error_code_(ERROR::NONE),
  is_dirty_(false),
  directory_type_(restype),
  filter_ext_(0)
{
}

Directory::~Directory()
{
  ReleaseMemory();
}

void Directory::SetDirectoryType(DIRECTORY_TYPE restype)
{
  directory_type_ = restype;
}

bool Directory::Open(const std::string& filepath)
{
  // clear before open
  Clear(false);

  SetPath(filepath.c_str());
  return doOpen();
};

bool Directory::Save()
{
  if (IsReadOnly())
    return false;

  if (!doWritePrepare()) return false;

  bool s = true;
  for (auto &fds : *this)
  {
    // continue if file is not dirty.
    if (!fds.is_dirty)
      continue;

    s &= doWrite(fds.d);

    fds.is_dirty = false;
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
  for (auto &fds : *this) fds.is_dirty = true;

  return Save();
}

bool Directory::Clear(bool flush)
{
  if (!Close(flush))
    return false;

  ReleaseMemory();

  return true;
}

void Directory::ReleaseMemory()
{
  for (auto &ii : files_)
  {
    free(ii.d.p);
  }

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

/*
 * do... functions DO real file system workaround.
 */

bool Directory::doRead(FileData &fd)
{
  return false;
};

bool Directory::doWritePrepare()
{
  return false;
}

bool Directory::doWrite(FileData &d)
{
  return false;
}

bool Directory::doRename(FileData &fd, const std::string& newname)
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

bool Directory::doDelete(const FileData& fd)
{
  return false;
}

bool Directory::doCreate(const std::string& newpath)
{
  return false;
}

void Directory::ClearStatus()
{
  is_dirty_ = false;
  path_.clear();
  file_ext_.clear();
  error_code_ = ERROR::NONE;
  filter_ext_ = 0;
}

void Directory::SetPath(const char * filepath)
{
  path_ = rutil::CleanPath(filepath);
  dirpath_ = rutil::GetDirectory(path_);
  file_ext_ = rutil::lower(rutil::GetExtension(path_));
}

const std::string& Directory::GetPath() const
{
  return path_;
}

const std::string& Directory::GetDirectoryPath() const
{
  return dirpath_;
}

std::string Directory::GetRelativePath(const std::string &orgpath) const
{
  return orgpath.substr(dirpath_.size() + 1);
}

std::string Directory::GetAbsolutePath(const std::string &relpath) const
{
  return dirpath_ + "\\" + relpath;
}

void Directory::SetExtension(const char * extension)
{
  path_ = rutil::ChangeExtension(path_, extension);
  file_ext_ = extension;
}

DIRECTORY_TYPE Directory::GetDirectoryType() const
{
  return directory_type_;
}

const char * Directory::GetErrorMsg() const
{
  return get_error_msg(error_code_);
}

bool Directory::IsLoaded()
{
  return (directory_type_ != DIRECTORY_TYPE::NONE);
}

bool Directory::AddFileData(FileData& d_, bool setdirty, bool copy)
{
  if (IsReadOnly())
    return false;

  if (copy)
  {
    FileData d;
    d.len = d_.len;
    d.p = (uint8_t*)malloc(d_.len);
    d.fn = d_.fn;
    files_.emplace_back(FileDataSegment{ d, setdirty });
  }
  else {
    files_.emplace_back(FileDataSegment{ d_, setdirty });
  }
  SetDirty(setdirty);
  return true;
}

bool Directory::AddFile(const std::string &relpath, bool setdirty)
{
  if (IsReadOnly())
    return false;

  const std::string path = GetAbsolutePath(relpath);
  FileData d = rutil::ReadFileData(path);
  if (d.len == 0)
  {
    SetError(ERROR::OPEN_INVALID_FILE);
    return false;
  }
  d.fn = relpath;
  AddFileData(d, setdirty);
  return true;
}

const Directory::FileDataSegment* Directory::GetSegment(const std::string& name, bool use_alternative_search) const
{
  return const_cast<Directory*>(this)->GetSegment(name);
}

Directory::FileDataSegment* Directory::GetSegment(const std::string& name, bool use_alternative_search)
{
  for (auto &fds : *this)
  {
    if (fds.d.GetFilename() == name)
      return &fds;
  }
  // Alternative search = search file without extension if file is not found.
  if (use_alternative_search)
  {
    const std::string filenameonly(std::move(rutil::GetFilename(name)));
    for (auto &fds : *this)
    {
      if (rutil::GetFilename(fds.d.GetFilename()) == filenameonly)
        return &fds;
    }
  }
  return 0;
}

const rutil::FileData * Directory::Get(const std::string & name, bool use_alternative_search) const
{
  const auto *ii = GetSegment(name);
  if (ii == 0)
    return 0;
  else
    return &ii->d;
}

const uint8_t * Directory::Get(const std::string & name, int & len, bool use_alternative_search) const
{
  const rutil::FileData* d = Get(name);
  len = d->len;
  return d->GetPtr();
}

void Directory::SetFilter(const char** filter_ext)
{
  filter_ext_ = filter_ext;
}

bool Directory::CheckFilenameByFilter(const std::string& filename)
{
  std::string ext = filename.substr(filename.find_last_of('.') + 1);
  const char **exts = filter_ext_;
  while (exts)
  {
    if (ext == *exts) return true;
    exts++;
  }
  return false;
}

bool Directory::ReadAll()
{
  if (IsReadOnly())
    return false;

  bool r = true;
  for (auto &fds : *this)
  {
    r |= Read(fds.d);
  }
  return r;
}

bool Directory::Read(FileData &d)
{
  if (IsReadOnly())
    return false;

  return doRead(d);
}

bool Directory::Delete(const std::string & name)
{
  if (IsReadOnly())
    return false;

  auto *fds = GetSegment(name);
  if (!fds)
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }

  // affects file system instantly.
  if (!doDelete(fds->d))
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

  auto *fds = GetSegment(prev_name);
  if (!fds)
  {
    SetError(ERROR::OPEN_NO_FILE);
    return false;
  }

  // affects file system instantly.
  if (!doRename(fds->d, new_name))
  {
    SetError(ERROR::WRITE_RENAME);
    return false;
  }

  fds->d.fn = new_name;
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

void Directory::SetDirty(bool flag)
{
  is_dirty_ = flag;
}

void Directory::CreateEmptyFileData(const std::string& filename)
{
  FileData fd;
  fd.fn = filename;
  AddFileData(fd, false, false);
}

DirectoryFolder::DirectoryFolder() : Directory(DIRECTORY_TYPE::FOLDER) {}

bool DirectoryFolder::IsReadOnly()
{
  return false;
}

bool DirectoryFolder::doOpen()
{
  if (!rutil::IsDirectory(GetPath()))
    return false;

  rutil::DirFileList files;
  rutil::GetDirectoryFiles(GetPath(), files);
  for (auto ii : files)
  {
    // check extension
    if (CheckFilenameByFilter(ii.first)) continue;
    CreateEmptyFileData(ii.first);
  }

  return true;
}

bool DirectoryFolder::doRead(FileData &d)
{
  if (!d.IsEmpty()) return true;
  const std::string fullpath(std::string(GetDirectoryPath()) + "/" + std::string(d.GetFilename()));
  d = rutil::ReadFileData(fullpath);
  if (d.IsEmpty())
  {
    SetError(ERROR::OPEN_INVALID_FILE);
    return false;
  }
  return true;
}

bool DirectoryFolder::doRename(FileData &fd, const std::string& newname)
{
  const std::string fullpath(std::string(GetDirectoryPath()) + "/" + newname);
  if (!rutil::Rename(fd.fn, fullpath))
  {
    SetError(ERROR::WRITE_RENAME);
    return false;
  }
  return true;
}

bool DirectoryFolder::doDelete(const FileData& fd)
{
  const std::string fullpath(GetDirectoryPath() + "/" + fd.fn);
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
  const std::string dirpath(GetDirectoryPath());
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

bool DirectoryFolder::doWrite(FileData& fd)
{
  // make full path before save data
  FileData tmpfd = fd;
  tmpfd.fn = GetDirectoryPath() + "/" + tmpfd.fn;
  if (!rutil::WriteFileData(tmpfd))
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }
  return true;
}

void DirectoryArchive::SetCodepage(int codepage)
{
  codepage_ = codepage;
}

#ifdef USE_ZLIB

DirectoryArchive::DirectoryArchive() : codepage_(0), archive_(0), zip_error_(0)
{ SetDirectoryType(DIRECTORY_TYPE::ARCHIVE); }

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
    CreateEmptyFileData(fn);
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

bool DirectoryArchive::doWrite(FileData &fd)
{
  zip_source_t *s;
  s = zip_source_buffer(archive_, fd.p, fd.len, 0);
  if (!s)
  {
    printf("Zip source buffer creation failed!\n");
    return false;
  }
  zip_error_ = zip_file_add(archive_, fd.fn.c_str(), s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
  zip_source_free(s);
  if (zip_error_)
  {
    printf("Zip file appending failed! (code %d)\n", zip_error_);
    return false;
  }
  return true;
}

bool DirectoryArchive::doRename(FileData &fd, const std::string& newname)
{
  // TODO zip element rename
  return false;
}

bool DirectoryArchive::doRead(FileData &fd)
{
  if (IsReadOnly())
    return false;

  zip_file_t *zfp = zip_fopen(archive_, fd.fn.c_str(), ZIP_FL_UNCHANGED);
  if (!zfp)
  {
    printf("Failed to read file(%s) from zip\n", fd.fn.c_str());
    return false;
  }
  struct zip_stat zStat;
  zip_stat(archive_, fd.fn.c_str(), 0, &zStat);
  fd.len = zStat.size;
  fd.p = (unsigned char*)malloc(fd.len);
  zip_fread(zfp, (void*)fd.p, fd.len);
  zip_fclose(zfp);
  return true;
}

bool DirectoryArchive::doDelete(const FileData& fd)
{
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

bool DirectoryArchive::IsReadOnly()
{
  return true;
}

#endif

DirectoryBinary::DirectoryBinary()
{
  SetDirectoryType(DIRECTORY_TYPE::BINARY);
}

bool DirectoryBinary::IsReadOnly()
{
  return false;
}

rutil::FileData* DirectoryBinary::GetDataPtr()
{
  return &begin()->d;
}

bool DirectoryBinary::AddFileData(FileData& d, bool setdirty, bool copy)
{
  // only allow single file
  if (count() == 0) return Directory::AddFileData(d, setdirty, copy);
  return false;
}

bool DirectoryBinary::AddFile(const std::string &relpath, bool setdirty)
{
  // only allow single file
  if (count() == 0) return Directory::AddFile(relpath, setdirty);
  else  return false;
}

bool DirectoryBinary::doOpen()
{
  if (!rutil::IsFile(GetPath()))
    return false;
  CreateEmptyFileData(rutil::GetFilename(GetPath()));
  return true;
}



DirectoryFactory& DirectoryFactory::Create(const std::string& path)
{
  DirectoryFactory *df = new DirectoryFactory(path);
  return *df;
}

DirectoryFactory::DirectoryFactory(const std::string& path)
  : path_(path), directory_(nullptr), is_directory_fetched_(false)
{
  std::string ext = rutil::lower(rutil::GetExtension(path));
  if (ext == "zip") directory_ = new DirectoryArchive();
  else if (rutil::IsDirectory(path)) directory_ = new DirectoryFolder();
  else directory_ = new DirectoryBinary();
}

DirectoryFactory::~DirectoryFactory()
{
  if (!is_directory_fetched_)
    delete directory_;
}

Directory* DirectoryFactory::GetDirectory()
{
  is_directory_fetched_ = true;
  return directory_;
}

DirectoryFactory& DirectoryFactory::SetFilter(const char** filter_ext)
{
  directory_->SetFilter(filter_ext);
  return *this;
}

bool DirectoryFactory::Open()
{
  return directory_->Open(path_);
}

}
