#include "Resource.h"
#include "Resource.h"
#include "Resource.h"
#include "Resource.h"
#include "Resource.h"
#include "Resource.h"
#include "Resource.h"
#include "Resource.h"

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
#define RPARSER_ERROR_RENAME		"Cannot rename file."
#define RPARSER_ERROR_DELETE		"Cannot delete file."

inline void _allocate_binarydata(Resource::BinaryData &d, unsigned int len)
{
	d.len = len;
	d.p = (char*)malloc(len);
}

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
	SetPath(filepath);
	encoding_ = encoding;
	filter_ext_ = filter_ext;
	// check is directory
	// if not, attempt to read it archive or not.
	if (rutil::IsDirectory(filepath))
	{
		dirpath_ = filepath + (path_[path_.size()-1]=='/'?"":"/");
		rutil::DirFileList files;
		rutil::GetDirectoryFiles(filepath, files);
		resource_type_ = RESOURCE_TYPE::FOLDER;
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
		// Before attempt Open_fp,
		// Try extension check for binary file reading.
		if (file_ext_ == "zip")
			resource_type_ = RESOURCE_TYPE::ARCHIVE;
		else if (file_ext_ == "vos")
			resource_type_ = RESOURCE_TYPE::VOSBINARY;
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
	switch (resource_type_)
	{
	case RESOURCE_TYPE::ARCHIVE:
#ifdef USE_ZLIB
		return Load_from_zip(fp);
#else
		error_msg_ = RPARSER_ERROR_READ;
		return false;
#endif
	case RESOURCE_TYPE::VOSBINARY:
		assert(0);
		return false;
	}
	// if it's not archive file, then it's a simple single binary file.
	// check for availabity.
	if (resource_type_ == RESOURCE_TYPE::NONE)
	{
		return false;
	}
	// just read the whole bulk file.
	Read_fp(fp, data_raw_);
	resource_type_ = RESOURCE_TYPE::VOSBINARY;
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
	return r;
}

// --
// VOS file read / write
// --

// not analyze chart,
// but just read whole charts and metadata.
bool Resource::_Read_VOS(FILE *fp)
{
	unsigned int vos_version;
	fread(&vos_version, 4, 1, fp);
	if (vos_version == 2)
	{
		return _Read_VOS_v2(fp);
	}
	else if (vos_verson == 3)
	{
		return _Read_VOS_v3(fp);
	}
	// unsupported version
	error_msg_ = RPARSER_ERROR_READ;
	return false;
}

struct vos_header_v2_s
{
	unsigned int filename_size;
	char filename[12];
	unsigned int filesize;
};

bool Resource::_Read_VOS_v2(FILE *fp)
{
	BinaryData d;
	vos_header_v2_s vos_header;
	// vos file
	fread(&vos_header, sizeof(vos_header_v2_s), 1, fp);
	_allocate_binarydata(d, vos_header.filesize);
	fread(d.p, 1, d.len, fp);
	AddBinary("Vosctemp.trk", d.p, d.len, false, false);
	// midi file
	fread(&vos_header, sizeof(vos_header_v2_s), 1, fp);
	_allocate_binarydata(d, vos_header.filesize);
	fread(d.p, 1, d.len, fp);
	AddBinary("Vosctemp.mid", d.p, d.len, false, false);
}

struct vos_header_v3_s
{
	unsigned int header_size;	// useless

	char inf_flag[16];			// useless, just checking
	unsigned int offset_inf;	// end of INF file
	char mid_flag[16];
	unsigned int offset_mid;	// end of MID file
	char eof_flag[16];			// empty area (EOF string flag)
};

bool Resource::_Read_VOS_v3(FILE *fp)
{
	// .inf file format
	BinaryData d;
	vos_header_v3_s vos_header;
	fread(&vos_header, sizeof(vos_header_v3_s), 1, fp);
	// read inf file
	_allocate_binarydata(d, vos_header.offset_inf - vos_header.header_size);
	fread(d.p, 1, d.len, fp);
	// check out for VOS1 signature before adding binary
	if (memcmp(d.p, "VOS1", 4) == 0)
	{
		AddBinary("Vosctemp.inf", d.p+46, d.len-46, false, true);
		free(d.p);
	}
	else
	{
		AddBinary("Vosctemp.inf", d.p, d.len, false, false);
	}
	// read mid file
	_allocate_binarydata(d, vos_header.offset_mid - vos_header.offset_inf);
	fread(d.p, 1, d.len, fp);
	AddBinary("Vosctemp.mid", d.p, d.len, false, false);
}

// --
// VOS file read / write end
// --

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

void Resource::SetResourceType(RESOURCE_TYPE rtype)
{
	resource_type_ = rtype;
}

RESOURCE_TYPE Resource::GetResourceType() const
{
	return resource_type_;
}

const char * Resource::GetErrorMsg() const
{
	return error_msg_;
}

bool Resource::IsLoaded()
{
	return (resource_type_ != RESOURCE_TYPE::NONE);
}

void Resource::AddBinary(std::string & name, char * p, unsigned int len, bool setdirty, bool copy)
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

bool Resource::AddFile(std::string & name, std::string & filename, bool setdirty)
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

bool Resource::Rename(std::string & prev_name, std::string & new_name)
{
	auto it = datas_.find(prev_name);
	if (it == datas_.end())
	{
		error_msg_ = RPARSER_ERROR_NOFILE;
		return false;
	}

	// if FOLDER I/O, then it affects file system directly.
	if (resource_type_ == RESOURCE_TYPE::FOLDER)
	{
		if (!rutil::RenameFile(prev_name.c_str(), new_name.c_str()))
		{
			error_msg_ = RPARSER_ERROR_RENAME;
			return false;
		}
	}

	BinaryData d = it->second;
	datas_[new_name] = d;
	datas_.erase(it);
	return true;
}

bool Resource::Delete(std::string & name)
{
	auto it = datas_.find(prev_name);
	if (it == datas_.end())
	{
		error_msg_ = RPARSER_ERROR_NOFILE;
		return false;
	}

	// if FOLDER I/O, then it affects file system directly.
	if (resource_type_ == RESOURCE_TYPE::FOLDER)
	{
		if (!rutil::RemoveFile(name.c_str()))
		{
			error_msg_ = RPARSER_ERROR_DELETE;
			return false;
		}
	}

	datas_.erase(it);
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

void Resource::FilterFiles(const char * filters, std::map<std::string, const BinaryData*>& chart_files)
{
	assert(filters);
	for (auto ii : datas_)
	{
		if (rutil::CheckExtension(ii.first, filters))
		{
			chart_files[ii.first] = ii.second;
		}
	}
}

void Resource::AllocateRawBinary(char * p, unsigned int len, bool copy)
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