#include "Resource.h"
#include "common.h"

namespace rparser
{

inline void _allocate_binarydata(Resource::BinaryData &d, unsigned int len)
{
	d.len = len;
	d.p = (char*)malloc(len);
}

Resource::Resource() : Resource(RESOURCE_TYPE::NONE)
{
}

Resource::Resource(RESOURCE_TYPE restype) :
  error_code_(ERROR::NONE),
  is_dirty_(false),
  resource_type_(restype),
  filter_ext_(0)
{
}

Resource::~Resource()
{
	Unload(false);
}

bool Resource::Open(const char * filepath)
{
  // clear before open
  Unload(false);

  SetPath(filepath);
  return doOpen();
};

bool Resource::Flush()
{
  return doFlush();
}

bool Resource::Unload(bool flush)
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

bool Resource::doOpen()
{
  SetError(ERROR::UNKNOWN);
  return false;
};

bool Resource::doFlush()
{
  return false;
}

bool Resource::doUnload()
{
  return true;
}

void Resource::ClearStatus()
{
  is_dirty_ = false;
  path_.clear();
  file_ext_.clear();
  error_code_ = ERROR::NONE;
  filter_ext_ = 0;
}

void Resource::SetPath(const char * filepath)
{
  path_ = rutil::CleanPath(filepath);
  dirpath_ = rutil::GetDirectory(path_);
  file_ext_ = rutil::lower(rutil::GetExtension(path_));
}

const std::string Resource::GetPath() const
{
  return path_;
}

const std::string Resource::GetDirectoryPath() const
{
  return dirpath_;
}

std::string Resource::GetRelativePath(const std::string &orgpath) const
{
  return orgpath.substr(dirpath_.size() + 1);
}

std::string Resource::GetAbsolutePath(const std::string &relpath) const
{
  return dirpath_ + "\\" + relpath;
}

void Resource::SetExtension(const char * extension)
{
  path_ = rutil::ChangeExtension(path_, extension);
  file_ext_ = extension;
}

RESOURCE_TYPE Resource::GetResourceType() const
{
  return resource_type_;
}

const char * Resource::GetErrorMsg() const
{
  return get_error_msg(error_code_);
}

bool Resource::IsLoaded()
{
  return (resource_type_ != RESOURCE_TYPE::NONE);
}

bool Resource::AddBinary(const std::string& key, BinaryData& d_, bool setdirty, bool copy)
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

bool Resource::AddFile(const std::string &relpath, bool setdirty)
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

bool Resource::Rename(const std::string & prev_name, const std::string & new_name)
{
  auto it = datas_.find(prev_name);
  if (it == datas_.end())
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }

  // affects file system instantly.
  if (!rutil::RenameFile(prev_name.c_str(), new_name.c_str()))
  {
    SetError(ERROR::WRITE_RENAME);
    return false;
  }

  BinaryData d = it->second;
  datas_[new_name] = d;
  datas_.erase(it);
  return true;
}

bool Resource::Delete(const std::string & name)
{
  auto it = datas_.find(name);
  if (it == datas_.end())
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }

  // affects file system directly.
  if (!rutil::RemoveFile(name.c_str()))
  {
    SetError(ERROR::DELETE_NO_PATH);
    return false;
  }

  datas_.erase(it);
  return true;
}

const Resource::BinaryData * Resource::GetPtr(const std::string & name) const
{
  auto ii = datas_.find(name);
  if (ii == datas_.end())
    return 0;
  else
    return &ii->second;
}

const char * Resource::GetPtr(const std::string & name, int & len) const
{
  const Resource::BinaryData* d = GetPtr(name);
  len = d->len;
  return d->p;
}

void Resource::SetFilter(const char** filter_ext)
{
  filter_ext_ = filter_ext;
}

bool Resource::CheckFilenameByFilter(const std::string& filename)
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

bool Resource::IsBinaryDirty(const std::string &key)
{
  return data_dirty_flag_[key];
}

Resource::data_iter Resource::data_begin()
{
  return datas_.begin();
}

Resource::data_iter Resource::data_end()
{
  return datas_.end();
}

void Resource::SetError(ERROR error)
{
  error_code_ = error;
}

void Resource::SetDirty(bool flag)
{
  is_dirty_ = flag;
}

bool Resource::Read_from_fp(FILE *fp, BinaryData &d)
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

bool Resource::Write_from_fp(FILE *fp, BinaryData &d)
{
  ASSERT(fp);
  if (fwrite(d.p, d.len, 1, fp) != d.len)
    return false;
  return true;
}

ResourceFolder::ResourceFolder() : Resource(RESOURCE_TYPE::FOLDER) {}

bool ResourceFolder::doOpen()
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

bool ResourceFolder::WriteBinary(const char* filepath, BinaryData& d)
{
  FILE *f = rutil::fopen_utf8(filepath, "wb");
  if (!f) return false;
  bool r = (fwrite(d.p, d.len, 1, f) != d.len);
  fclose(f);
  return r;
}

bool ResourceFolder::doFlush()
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

ResourceArchive::ResourceArchive() {  }

bool ResourceArchive::Load_from_zip(FILE * fp)
{
	resource_type_ = RESOURCE_TYPE::ARCHIVE;
	return false;
}

bool ResourceArchive::WriteZip()
{
	// TODO
	return false;
}

#else

ResourceArchive::ResourceArchive() {  }

#endif

ResourceBinary::ResourceBinary()
  : Resource(RESOURCE_TYPE::BINARY)
{
}

bool ResourceBinary::doOpen()
{
  SetPath(GetPath().c_str());
  return Resource::AddFile(GetRelativePath(GetPath()), false);
}

Resource::BinaryData* ResourceBinary::GetDataPtr()
{
  return &data_begin()->second;
}

bool ResourceBinary::AddBinary(const std::string &key, BinaryData& d, bool setdirty, bool copy)
{
  // only allow single file
  if (count() == 0) return Resource::AddBinary(key, d, setdirty, copy);
  return false;
}

bool ResourceBinary::AddFile(const std::string &relpath, bool setdirty)
{
  // only allow single file
  if (count() == 0) return Resource::AddFile(relpath, setdirty);
  else  return false;
}

Resource* ResourceFactory::Open(const char* path, const char** filter_ext)
{
  Resource* r;
  std::string ext = rutil::lower(rutil::GetExtension(path));
  if (ext == "zip") r = new ResourceArchive();
  else if (rutil::IsDirectory(path)) r = new ResourceFolder();
  else r = new ResourceBinary();
  r->SetFilter(filter_ext);
  return r;
}

}