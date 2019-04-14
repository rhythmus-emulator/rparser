#include "Directory.h"
#include "common.h"

namespace rparser
{

inline void _allocate_binarydata(Directory::BinaryData &d, unsigned int len)
{
	d.len = len;
	d.p = (char*)malloc(len);
}

Directory::Directory() : Directory(DIRECTORY_TYPE::NONE)
{
}

Directory::Directory(DIRECTORY_TYPE restype) :
  error_code_(ERROR::NONE),
  is_dirty_(false),
  directory_type_(restype),
  directory_(0),
  filter_ext_(0)
{
  switch (restype)
  {
  case DIRECTORY_TYPE::FOLDER:
    directory_ = new rutil::BasicDirectory();
    break;
  case DIRECTORY_TYPE::ARCHIVE:
    directory_ = new rutil::ArchiveDirectory();
    break;
  }
}

Directory::~Directory()
{
	Unload(false);
}

void Directory::SetDirectoryType(DIRECTORY_TYPE restype)
{
  directory_type_ = restype;
}

bool Directory::Open(const char * filepath)
{
  // clear before open
  Unload(false);

  SetPath(filepath);
  return doOpen();
};

bool Directory::Flush()
{
  return doFlush();
}

bool Directory::Unload(bool flush)
{
  // if necessary, flush data
  if (flush && !doFlush())
    return false;

  ClearStatus();

  // release allocated memory and reset flag
  for (auto ii : datas_)
  {
    free(ii.second.p);
  }

  datas_.clear();
  data_dirty_flag_.clear();

  return doUnload();
}

bool Directory::doOpen()
{
  SetError(ERROR::UNKNOWN);
  return false;
};

bool Directory::doFlush()
{
  return false;
}

bool Directory::doUnload()
{
  return true;
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

const std::string Directory::GetPath() const
{
  return path_;
}

const std::string Directory::GetDirectoryPath() const
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

bool Directory::AddBinary(const std::string& key, BinaryData& d_, bool setdirty, bool copy)
{
  if (copy)
  {
    BinaryData d;
    d.len = d_.len;
    d.p = (char*)malloc(d_.len);
    datas_[key] = d;
  }
  else {
    datas_[key] = d_;
  }
  data_dirty_flag_[key] = setdirty;
  SetDirty(setdirty);
  return true;
}

bool Directory::AddFile(const std::string &relpath, bool setdirty)
{
  const std::string path = GetAbsolutePath(relpath);
  FILE *fp = rutil::fopen_utf8(path.c_str(), "rb");
  if (!fp)
  {
    SetError(ERROR::OPEN_NO_FILE);
    return false;
  }
  BinaryData d;
  Read_from_fp(fp, d);
  fclose(fp);
  AddBinary(relpath, d, setdirty);
  return true;
}

bool Directory::Rename(const std::string & prev_name, const std::string & new_name)
{
  auto it = datas_.find(prev_name);
  if (it == datas_.end())
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }

  // affects file system instantly.
  if (!rutil::Rename(prev_name.c_str(), new_name.c_str()))
  {
    SetError(ERROR::WRITE_RENAME);
    return false;
  }

  BinaryData d = it->second;
  datas_[new_name] = d;
  datas_.erase(it);
  return true;
}

bool Directory::Delete(const std::string & name)
{
  auto it = datas_.find(name);
  if (it == datas_.end())
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }

  // affects file system directly.
  if (!rutil::DeleteFile(name.c_str()))
  {
    SetError(ERROR::DELETE_NO_PATH);
    return false;
  }

  datas_.erase(it);
  return true;
}

const Directory::BinaryData * Directory::GetPtr(const std::string & name) const
{
  auto ii = datas_.find(name);
  if (ii == datas_.end())
    return 0;
  else
    return &ii->second;
}

const char * Directory::GetPtr(const std::string & name, int & len) const
{
  const Directory::BinaryData* d = GetPtr(name);
  len = d->len;
  return d->p;
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

bool Directory::IsBinaryDirty(const std::string &key)
{
  return data_dirty_flag_[key];
}

Directory::data_iter Directory::data_begin()
{
  return datas_.begin();
}

Directory::data_iter Directory::data_end()
{
  return datas_.end();
}

void Directory::SetError(ERROR error)
{
  error_code_ = error;
}

void Directory::SetDirty(bool flag)
{
  is_dirty_ = flag;
}

bool Directory::Read_from_fp(FILE *fp, BinaryData &d)
{
  ASSERT(fp);
  fseek(fp, 0, SEEK_END);
  d.len = ftell(fp);
  rewind(fp);
  ASSERT(d.p == 0);
  d.p = (char*)malloc(d.len);
  fread(d.p, 1, d.len, fp);
  return true;
}

bool Directory::Write_from_fp(FILE *fp, BinaryData &d)
{
  ASSERT(fp);
  if (fwrite(d.p, d.len, 1, fp) != d.len)
    return false;
  return true;
}

DirectoryFolder::DirectoryFolder() : Directory(DIRECTORY_TYPE::FOLDER) {}

bool DirectoryFolder::doOpen()
{
  const char* filepath = GetPath().c_str();
  const char* dirpath = GetDirectoryPath().c_str();

  if (!rutil::IsDirectory(filepath))
    return false;

  rutil::DirFileList files;
  rutil::GetDirectoryFiles(filepath, files);
  for (auto ii : files)
  {
    // check extension
    if (CheckFilenameByFilter(ii.first)) continue;

    // fetch file pointer
    FILE *fp = rutil::fopen_utf8((std::string(dirpath) + std::string(ii.first)).c_str(), "rb");
    if (!fp)
    {
      SetError(ERROR::OPEN_INVALID_FILE);
      return false;
    }

    BinaryData d;
    Read_from_fp(fp, d);
    fclose(fp);

    AddBinary(ii.first, d);
  }

  return true;
}

bool DirectoryFolder::WriteBinary(const char* filepath, BinaryData& d)
{
  FILE *f = rutil::fopen_utf8(filepath, "wb");
  if (!f) return false;
  bool r = (fwrite(d.p, d.len, 1, f) != d.len);
  fclose(f);
  return r;
}

bool DirectoryFolder::doFlush()
{
  bool s = true;
  for (auto ii = data_begin(); ii != data_end(); ++ii)
  {
    // continue if file is not dirty.
    if (!IsBinaryDirty(ii->first))
      continue;
    // TODO: make correct path (combination of folder and file path)
    s = WriteBinary(ii->first.c_str(), ii->second);
    if (!s)
    {
      SetError(ERROR::WRITE_NO_PATH);
      break;
    }
  }
  return s;
}

#ifdef USE_ZLIB

DirectoryArchive::DirectoryArchive() { SetDirectoryType(DIRECTORY_TYPE::ARCHIVE); }

bool DirectoryArchive::Load_from_zip(FILE * fp)
{
  // TODO
	return false;
}

bool DirectoryArchive::WriteZip()
{
	// TODO
	return false;
}

#else

DirectoryArchive::DirectoryArchive() {  }

#endif

DirectoryBinary::DirectoryBinary()
  : Directory(DIRECTORY_TYPE::BINARY)
{
}

bool DirectoryBinary::doOpen()
{
  SetPath(GetPath().c_str());
  return Directory::AddFile(GetRelativePath(GetPath()), false);
}

Directory::BinaryData* DirectoryBinary::GetDataPtr()
{
  return &data_begin()->second;
}

bool DirectoryBinary::AddBinary(const std::string &key, BinaryData& d, bool setdirty, bool copy)
{
  // only allow single file
  if (count() == 0) return Directory::AddBinary(key, d, setdirty, copy);
  return false;
}

bool DirectoryBinary::AddFile(const std::string &relpath, bool setdirty)
{
  // only allow single file
  if (count() == 0) return Directory::AddFile(relpath, setdirty);
  else  return false;
}

Directory* DirectoryFactory::Open(const char* path, const char** filter_ext)
{
  Directory* r;
  std::string ext = rutil::lower(rutil::GetExtension(path));
  if (ext == "zip") r = new DirectoryArchive();
  else if (rutil::IsDirectory(path)) r = new DirectoryFolder();
  else r = new DirectoryBinary();
  r->SetFilter(filter_ext);
  return r;
}

}