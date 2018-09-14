#include <Resource.h>

namespace rparser
{

// Error flags ...
#define RPARSER_ERROR_NOFILE		"No file exists, or file isn't openable."
#define RPARSER_ERROR_DIR			"Cannot read directory."
#define RPARSER_ERROR_READ			"File found, but it's not suitable for opening."
#define RPARSER_ERROR_INVALIDZIP	"Damaged, or incompatible zip format."
#define RPARSER_ERROR_WRITE			"Cannot write file, is file or directory already existing?"\
	" Or, is disk space enough or permission allowed?"
#define RPARSER_ERROR_WRITE_NONE	"Cannot write file. Call Create() before writing."
#define RPARSER_ERROR_WRITE_FORMAT	"Cannot write file. Unsupported format."
#define RPARSER_ERROR_CREATE		"Cannot create file. Please close before create."

Resource::Resource():
	error_msg_(0),
	is_dirty_(false),
	resource_type_(RESOURCE_TYPE::NONE)
{
	data_raw_.p = 0;
	data_raw_.len = 0;
}

Resource::~Resource()
{
	Unload(false);
}

bool Resource::Open(const char * filepath, const char * encoding, const char* filter_ext)
{
	// clear before open
	Unload(false);

	// start opening
	path_ = rutil::CleanPath(filepath);
	filepath = path_.c_str();		// replace 'iterator' with safe path string
	file_ext_.clear();
	const char* org_ptr_ = filepath;
	const char* ext_ptr_ = 0;
	const char* dir_ptr_ = 0;
	dirpath_.clear();
	file_ext_.clear();
	while (*filepath)
	{
		if (*filepath == '.')
		{
			ext_ptr = filepath + 1;
		}
		if (*filepath == '/')
		{
			dir_ptr_ = filepath;
		}
		filepath++;
	}
	if (dir_ptr_) dirpath_ = path_.substr(0, dir_ptr_- org_ptr_) + "/";
	if (ext_ptr_) file_ext_ = ext_ptr_;
	encoding_ = encoding;
	filter_ext_ = filter_ext;
	// check is directory
	// if not, attempt to read it archive or not.
	if (rutil::IsDirectory(filepath))
	{
		dirpath_ = filepath + (path_[path_.size()-1]=='/'?"":"/");
		rutil::DirFileList files;
		rutil::GetDirectoryFiles(filepath, files);
		return Open_dir(files);
	}
	else
	{
		FILE *f = rutil::fopen_utf8(filepath, "rb");
		if (!f)
		{
			error_msg_ = RPARSER_ERROR_READ;
			return false;
		}
		bool r = Open_fp(fp, encoding, filter_ext);
		fclose(fp);
		return r;
	}
}

void Resource::Read_fp(FILE *fp, BinaryData &d)
{
	fseek(fp, 0, SEEK_END);
	d.len = ftell(fp);
	rewind(fp);
	assert(d.p == 0);
	d.p = (char*)malloc(d.len);
	fread(d.p, 1, d.len, fp);
	return true;
}

bool Resource::Open_fp(FILE * fp)
{
	// check extension whether it is archive file or not
	if (rutil::lower(file_ext_) == "zip")
	{
#ifdef USE_ZLIB
		return Load_from_zip(fp);
#else
		error_msg_ = RPARSER_ERROR_READ;
		return false;
#endif
	}
	// if it's not archive file, then it's a simple single binary file.
	// just read the whole bulk file.
	Read_fp(fp, data_raw_);
	resource_type_ = RESOURCE_TYPE::BINARY;
	return true;
}

bool Resource::Open_dir(rutil::DirFileList files_)
{
	bool r = true;
	for (auto ii : files_)
	{
		// check extension
		// fetch file pointer
		FILE *fp = rutil::fopen_utf8(dirpath_ + ii.first, "rb");
		if (!fp)
		{
			error_msg_ = RPARSER_ERROR_NOFILE;
			r = false;
			break;
		}
		BinaryData d;
		d.p = 0;
		d.len = 0;
		Read_fp(fp, d);
		fclose(fp);
		datas_[ii.first] = d;
		data_dirty_flag_[ii.first] = false;
	}
	if (r)
	{
		resource_type_ = RESOURCE_TYPE::FOLDER;
	}
	return r;
}

#ifdef USE_ZLIB
bool Resource::Load_from_zip(FILE * fp)
{
	resource_type_ = RESOURCE_TYPE::ARCHIVE;
	return false;
}

bool Resource::WriteZip()
{
	// TODO
	return false;
}
#endif

bool Resource::WriteBinary(const char* filepath, BinaryData& d)
{
	FILE *f = rutil::fopen_utf8(filepath, "wb");
	if (!f)
	{
		error_msg_ = RPARSER_ERROR_WRITE;
		return false;
	}
	if (fwrite(d.p, d.len, 1, f) != d.len)
	{
		error_msg_ = RPARSER_ERROR_WRITE;
		return false;
	}
	fclose(f);
	return true;
}

bool Resource::WriteAll()
{
	bool s = true;
	for (auto ii : datas_)
	{
		if (!data_dirty_flag_[ii.first])
			continue;
		// TODO: make combination of folder and file path.
		s = WriteBinary(ii.first.c_str(), ii.second);
		if (!s)
		{
			error_msg_ = RPARSER_ERROR_WRITE;
			break;
		}
	}
	return s;
}

bool Resource::Flush()
{
	switch (resource_type_)
	{
	case RESOURCE_TYPE::ARCHIVE:
#ifdef USE_ZLIB
		return WriteZip();
#else
		error_msg_ = RPARSER_ERROR_WRITE_FORMAT;
		break;
#endif
	case RESOURCE_TYPE::FOLDER:
		return WriteAll();
	case RESOURCE_TYPE::BINARY:
		return WriteBinary(path_.c_str(), data_raw_);
	case RESOURCE_TYPE::NONE:
		error_msg_ = RPARSER_ERROR_WRITE_NONE;
		break;
	}
	return false;
}

bool Resource::Create(RESOURCE_TYPE rtype, bool flush)
{
	if (resource_type_ != RESOURCE_TYPE::NONE)
	{
		error_msg_ = RPARSER_ERROR_CREATE;
		return false;
	}
	assert(rtype != RESOURCE_TYPE::NONE);
	resource_type_ = rtype;
	if (flush)
	{
		return Flush();
	}
	return true;
}

void Resource::Unload(bool flush)
{
	// if necessary, flush data
	if (flush)
	{
		Flush();
	}
	// release allocated memory and reset flag
	for (auto ii : datas_)
	{
		free(ii.second.p);
	}
	if (data_raw_.p)
	{
		free(data_raw_.p);
		data_raw_.p = 0;
		data_raw_.len = 0;
	}
	path_.clear();
	file_ext_.clear();
	error_msg_ = 0;
	filter_ext_.clear();
	encoding_.clear();
	datas_.clear();
	data_dirty_flag_.clear();
	is_dirty_ = false;
	resource_type_ = RESOURCE_TYPE::NONE;
}

const char * Resource::GetErrorMsg()
{
	return error_msg_;
}

bool Resource::IsLoaded()
{
	return (resource_type_ != RESOURCE_TYPE::NONE);
}

void Resource::AllocateBinary(std::string & name, char * p, int len, bool setdirty, bool copy)
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
	is_dirty |= setdirty;
}

bool Resource::AllocateFile(std::string & name, std::string & filename, bool setdirty)
{
	FILE *fp = rutil::fopen_utf8(name, "rb");
	if (!fp)
	{
		error_msg_ = RPARSER_ERROR_NOFILE;
		return false;
	}
	BinaryData d;
	Read_fp(fp, d);
	fclose(fp);
	AllocateBinary(name, d.p, d.len, setdirty)
	return true;
}

const Resource::BinaryData * Resource::GetPtr(std::string & name) const
{
	auto ii = datas_.find(name);
	if (ii == datas_.end())
		return 0;
	else
		return &ii->second;
}

const char * Resource::GetPtr(std::string & name, int & len) const
{
	const Resource::BinaryData* d = GetPtr(name);
	len = d->len;
	return d->p;
}

void Resource::AllocateRawBinary(char * p, int len, bool copy)
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

const Resource::BinaryData * Resource::GetRawPtr() const
{
	return &data_raw_;
}

const char * Resource::GetRawPtr(int & len) const
{
	len = data_raw_.len;
	return data_raw_.p;
}

}