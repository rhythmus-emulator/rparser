#include "Resource.h"

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
  resource_type_(restype)
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
  ClearStatus();
  return true;
}

void Resource::ClearStatus()
{
  is_dirty_ = false;
  path_.clear();
  file_ext_.clear();
  error_code_ = ERROR::NONE;
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

    datas_[ii.first] = d;
    data_dirty_flag_[ii.first] = false;
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
  for (auto ii : datas_)
  {
    // continue if file is not dirty.
    if (!data_dirty_flag_[ii.first])
      continue;
    // TODO: make correct path (combination of folder and file path)
    s = WriteBinary(ii.first.c_str(), ii.second);
    if (!s)
    {
      SetError(ERROR::WRITE_NO_PATH);
      break;
    }
  }
  return s;
}

bool ResourceFolder::doUnload()
{
  // release allocated memory and reset flag
  for (auto ii : datas_)
  {
    free(ii.second.p);
  }

  filter_ext_.clear();
  datas_.clear();
  data_dirty_flag_.clear();

  ClearStatus();
}

void ResourceFolder::SetFilter(const char* filter_ext)
{
  filter_ext_ = filter_ext;
}


const Resource::BinaryData * ResourceFolder::GetPtr(const std::string & name) const
{
  auto ii = datas_.find(name);
  if (ii == datas_.end())
    return 0;
  else
    return &ii->second;
}

const char * ResourceFolder::GetPtr(const std::string & name, int & len) const
{
  const Resource::BinaryData* d = GetPtr(name);
  len = d->len;
  return d->p;
}

void ResourceFolder::FilterFiles(const char * filters, std::map<std::string, const BinaryData*>& chart_files)
{
  assert(filters);
  for (auto ii : datas_)
  {
    if (rutil::CheckExtension(ii.first, filters))
    {
      chart_files[ii.first] = &ii.second;
    }
  }
}

void ResourceFolder::AddBinary(const std::string & name, char * p, unsigned int len, bool setdirty, bool copy)
{
  BinaryData d;
  if (copy)
  {
    d.len = len;
    d.p = (char*)malloc(len);
    memcpy(d.p, p, len);
  }
  else {
    d.p = p;
    d.len = len;
  }
  datas_[name] = d;
  data_dirty_flag_[name] = setdirty;
  SetDirty(setdirty);
}

bool ResourceFolder::AddFile(const std::string & name, const std::string & filename, bool setdirty)
{
  FILE *fp = rutil::fopen_utf8(name.c_str(), "rb");
  if (!fp)
  {
    SetError(ERROR::OPEN_NO_FILE);
    return false;
  }
  BinaryData d;
  Read_from_fp(fp, d);
  fclose(fp);
  AddBinary(name, d.p, d.len, setdirty);
  return true;
}

bool ResourceFolder::Rename(const std::string & prev_name, const std::string & new_name)
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

bool ResourceFolder::Delete(const std::string & name)
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


#ifdef USE_ZLIB
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
#endif

ResourceBinary::ResourceBinary()
  : Resource(RESOURCE_TYPE::BINARY)
{
  data_raw_.p = 0;
  data_raw_.len = 0;
}

bool ResourceBinary::doOpen()
{
  FILE *f = rutil::fopen_utf8(GetPath().c_str(), "rb");
  if (!f)
  {
    SetError(ERROR::OPEN_NO_FILE);
    return false;
  }
  bool r = Read_from_fp(f, data_raw_);
  fclose(f);
  return r;
}

bool ResourceBinary::doFlush()
{
  FILE *f = rutil::fopen_utf8(GetPath().c_str(), "wb");
  if (!f)
  {
    SetError(ERROR::WRITE_NO_PATH);
    return false;
  }
  bool r = Write_from_fp(f, data_raw_);
  fclose(f);
  return r;
}

bool ResourceBinary::doUnload()
{
  if (data_raw_.p)
  {
    free(data_raw_.p);
    data_raw_.len = 0;
  }
  return true;
}

void ResourceBinary::AllocateRawBinary(char * p, unsigned int len, bool copy)
{
	BinaryData d;
	if (copy)
	{
		d.len = len;
		d.p = (char*)malloc(len);
		memcpy(d.p, p, len);
	}
	else {
		d.p = p;
		d.len = len;
	}
	data_raw_ = d;
}

const Resource::BinaryData * ResourceBinary::GetRawPtr() const
{
	return &data_raw_;
}

const char * ResourceBinary::GetRawPtr(int & len) const
{
	len = data_raw_.len;
	return data_raw_.p;
}

Resource* ResourceFactory::CreateResource(const char* path)
{
  return 0;
}

}