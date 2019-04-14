/*
 * by @lazykuna, MIT License.
 * 
 */

#ifndef RPARSER_DIRECTORY_H
#define RPARSER_DIRECTORY_H

#include "rutil.h"
#include "Error.h"

namespace rparser
{

enum class DIRECTORY_TYPE
{
	NONE,
	FOLDER,
	ARCHIVE,
	BINARY,
};

class Directory
{
public:
	Directory();
	~Directory();

	struct BinaryData {
		char *p;
		unsigned int len;
	};

	// Open file in specific encoding (in case of archive file, e.g. zip).
	// encoding set to 0 means library will automatically find encoding.
	// opened file name automatically converted into UTF8, even in windows.
	// filter_ext is filtering extension to be stored in memory.
	// if filter_ext set to 0, then all file is read.
	bool Open(const char* filepath);

	// Flush all changes into file and reset all dirty flags.
	// If succeed, return true. else, return false and canceled.
	// Detailed error message is stored in error_msg_
  bool Flush();

	// Unload all Directory and free allocated memory.
	// You can save data with setting parameter flush=true.
  bool Unload(bool flush = true);

	// Set destination path to save
	// Don't check existence for given path.
	void SetPath(const char* filepath);
  void SetExtension(const char* extension);
  const std::string GetPath() const;
  const std::string GetDirectoryPath() const;
  std::string GetRelativePath(const std::string &orgpath) const;
  std::string GetAbsolutePath(const std::string &relpath) const;
	DIRECTORY_TYPE GetDirectoryType() const;
	const char* GetErrorMsg() const;
	bool IsLoaded();
  virtual bool AddBinary(const std::string& relpath, BinaryData& d, bool setdirty=true, bool copy=false);
  virtual bool AddFile(const std::string &relpath, bool setdirty = true);
  bool Delete(const std::string &name);
  bool Rename(const std::string &prev_name, const std::string &new_name);
  bool IsBinaryDirty(const std::string& key);
  typedef std::map<std::string, BinaryData>::iterator data_iter;
  const BinaryData* GetPtr(const std::string &name) const;
  const char* GetPtr(const std::string &name, int &len) const;
  void SetFilter(const char** filter_ext);
  bool CheckFilenameByFilter(const std::string& filename);
  data_iter data_begin();
  data_iter data_end();
  size_t count();

private:
  rutil::IDirectory* directory_;
	std::string path_;
	std::string dirpath_;
	std::string file_ext_;
  DIRECTORY_TYPE directory_type_;
  ERROR error_code_;
  bool is_dirty_;
  std::map<std::string, BinaryData> datas_;
  std::map<std::string, bool> data_dirty_flag_;
  const char **filter_ext_;

  virtual bool doOpen();
  virtual bool doFlush();
  virtual bool doUnload();

protected:
  Directory(DIRECTORY_TYPE restype);
  void SetDirectoryType(DIRECTORY_TYPE restype);
  static bool Read_from_fp(FILE *fp, BinaryData& d);
  static bool Write_from_fp(FILE *fp, BinaryData& d);
  void ClearStatus();
  void SetError(ERROR error);
  void SetDirty(bool flag = true);
};


class DirectoryFolder : public Directory
{
public:
  DirectoryFolder();
  void SetFilter(const char* filter_ext);
  // Filter out files
  void FilterFiles(const char* filters,
    std::map<std::string, const BinaryData*>& chart_files);

private:
  virtual bool doOpen();
  virtual bool doFlush();
  static bool WriteBinary(const char* filepath, BinaryData& d);
};

class DirectoryArchive : public DirectoryFolder
{
public:
  DirectoryArchive();

#ifdef USE_ZLIB
private:
  virtual bool doOpen();
  virtual bool doFlush();
  virtual bool doUnload();

	bool Load_from_zip(FILE *fp);	// TODO
	bool WriteZip();	// TODO
#endif
};

class DirectoryBinary : public Directory
{
public:
  DirectoryBinary();

  // Some file (ex: lr2course, vos) won't behave in form of multiple file.
  // In this case, we use data-ptr reserved for raw format
  // instead of data-key mapping list.
  virtual bool AddBinary(const std::string &key, BinaryData& d, bool setdirty = true, bool copy = false);
  virtual bool AddFile(const std::string &relpath , bool setdirty = true);
  BinaryData* GetDataPtr();
private:
  virtual bool doOpen();
};

class DirectoryFactory
{
public:
  static Directory* Open(const char* path, const char** filter_ext = 0);
};

}

#endif