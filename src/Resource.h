/*
 * by @lazykuna, MIT License.
 * 
 */

#ifndef RPARSER_RESOURCE_H
#define RPARSER_RESOURCE_H

#include "rutil.h"

namespace rparser
{

enum class RESOURCE_TYPE
{
	NONE,
	FOLDER,
	ARCHIVE,
	BINARY
};

class Resource
{
public:
	Resource();
	~Resource();

	struct BinaryData {
		char *p;
		unsigned int len;
	};

	// Open file in specific encoding (in case of archive file, e.g. zip).
	// encoding set to 0 means library will automatically find encoding.
	// opened file name automatically converted into UTF8, even in windows.
	// filter_ext is filtering extension to be stored in memory.
	// if filter_ext set to 0, then all file is read.
	bool Open(const char* filepath, const char* encoding = 0, const char* filter_ext = 0);

	// Flush all changes into file and reset all dirty flags.
	// If succeed, return true. else, return false and canceled.
	// Detailed error message is stored in error_msg_
	bool Flush();

	// Create new file, preparing with flushing.
	// This function sets resource_type_ to make flush work
	// and prepares directory (in case).
	bool Create(RESOURCE_TYPE rtype, bool flush = true);

	// Unload all resource and free allocated memory.
	// You can save data with setting parameter flush=true.
	bool Unload(bool flush = true);

	// Set destination path to save
	// Don't check existence for given path.
	void SetPath(const char* filepath);
	const std::string GetPath() const;

	const char* GetErrorMsg() const;

	bool IsLoaded();
	void AddBinary(std::string &name, char *p, int len, bool setdirty=true, bool copy=false);
	bool AddFile(std::string &name, std::string &filename, bool setdirty=true);
	bool Rename(std::string &prev_name, std::string &new_name);
	bool Delete(std::string &name);
	const BinaryData* GetPtr(std::string &name) const;
	const char* GetPtr(std::string &name, int &len) const;
	// Filter out files
	void FilterFiles(const char* filters,
		std::map<std::string, const BinaryData*>& chart_files);

	// Some file (ex: lr2course, vos) won't behave in form of multiple file.
	// In this case, we use data-ptr reserved for raw format
	// instead of data-key mapping list.
	void AllocateRawBinary(char *p, int len, bool copy=false);
	const BinaryData* GetRawPtr() const;
	const char* GetRawPtr(int &len) const;
private:
	std::string path_;
	std::string dirpath_;
	std::string file_ext_;
	RESOURCE_TYPE resource_type_;

	std::map<std::string, BinaryData> datas_;
	std::map<std::string, bool> data_dirty_flag_;
	std::list<std::string> removed_filepaths_;
	BinaryData data_raw_;
	const char* error_msg_;
	bool is_dirty_;

	std::string filter_ext_;
	std::string encoding_;
	void Read_fp(FILE *fp, BinaryData &d);
	bool Open_fp(FILE *fp);
	bool Open_dir(rutil::DirFileList files_);
#ifdef USE_ZILB
	bool Load_from_zip(FILE *fp);	// TODO
	bool WriteZip();	// TODO
#endif
	bool WriteBinary(const char* filepath, BinaryData& d);
	bool WriteAll();
};

}

#endif