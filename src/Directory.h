/*
 * by @lazykuna, MIT License.
 * 
 */

#ifndef RPARSER_DIRECTORY_H
#define RPARSER_DIRECTORY_H

#include "rutil.h"
#include "Error.h"

#ifdef USE_ZLIB
# define ZIP_STATIC 1
# include <zip.h>
#endif

namespace rparser
{

using FileData = rutil::FileData;

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

  struct FileDataSegment
  {
    FileData d;
    bool is_dirty;
  };

	// Open file in specific encoding (in case of archive file, e.g. zip).
	// encoding set to 0 means library will automatically find encoding.
	// opened file name automatically converted into UTF8, even in windows.
	// filter_ext is filtering extension to be stored in memory.
	// if filter_ext set to 0, then all file is read.
	bool Open(const std::string& filepath);

	// Flush all changes into file and reset all dirty flags.
	// If succeed, return true. else, return false and canceled.
	// Detailed error message is stored in error_msg_
  bool Save();
  bool SaveAs(const std::string& newpath);

	// Unload all Directory and allocated memory.
	// You can save data with setting parameter flush=true.
  bool Clear(bool flush = true);
  // Just close directory handle.
  bool Close(bool flush = true);
  // if Closed, then readonly state.
  virtual bool IsReadOnly();


	// Set destination path to save
	// Don't check existence for given path.
  const std::string &GetPath() const;
  const std::string &GetDirectoryPath() const;
  std::string GetRelativePath(const std::string &orgpath) const;
  std::string GetAbsolutePath(const std::string &relpath) const;
	DIRECTORY_TYPE GetDirectoryType() const;
	const char* GetErrorMsg() const;
	bool IsLoaded();
  // TODO:
  // Lock while Add/Delete/Renaming object (addr may changed due to vector realloc)
  virtual bool AddFileData(FileData& d, bool setdirty=true, bool copy=false);
  virtual bool AddFile(const std::string &relpath, bool setdirty = true);
  bool Delete(const std::string &name);
  bool Rename(const std::string &prev_name, const std::string &new_name);
  bool ReadAll();
  bool Read(FileData &d);
  void SetFilter(const char** filter_ext);
  bool CheckFilenameByFilter(const std::string& filename);

  typedef std::vector<FileDataSegment>::iterator data_iter;
  typedef std::vector<FileDataSegment>::const_iterator data_constiter;
  const FileDataSegment* GetSegment(const std::string& name, bool use_alternative_search = false) const;
  FileDataSegment* GetSegment(const std::string& name, bool use_alternative_search = false);
  const FileData* Get(const std::string &name, bool use_alternative_search = false) const;
  const uint8_t* Get(const std::string &name, int &len, bool use_alternative_search = false) const;
  data_iter begin();
  data_iter end();
  const data_constiter begin() const;
  const data_constiter end() const;
  size_t count() const;

private:
	std::string path_;
	std::string dirpath_;
	std::string file_ext_;
  DIRECTORY_TYPE directory_type_;
  ERROR error_code_;
  bool is_dirty_;
  const char **filter_ext_;

  virtual bool doRead(FileData &d);
  virtual bool doWritePrepare();
  virtual bool doWrite(FileData &d);
  virtual bool doRename(FileData &fd, const std::string& newname);
  virtual bool doOpen();
  virtual bool doClose();
  virtual bool doDelete(const FileData& fd);
  virtual bool doCreate(const std::string& newpath);

  void SetPath(const char* filepath);
  void SetExtension(const char* extension);

protected:
  std::vector<FileDataSegment> files_;
  Directory(DIRECTORY_TYPE restype);
  void SetDirectoryType(DIRECTORY_TYPE restype);
  void ClearStatus();
  void SetError(ERROR error);
  void SetDirty(bool flag = true);
  void CreateEmptyFileData(const std::string& filename);
};


class DirectoryFolder : public Directory
{
public:
  DirectoryFolder();
  virtual bool IsReadOnly();

private:
  virtual bool doRead(FileData &d);
  virtual bool doWritePrepare();
  virtual bool doWrite(FileData &d);
  virtual bool doRename(FileData &fd, const std::string& newname);
  virtual bool doOpen();
  virtual bool doDelete(const FileData& fd);
  virtual bool doCreate(const std::string& newpath);
};

class DirectoryArchive : public DirectoryFolder
{
public:
  DirectoryArchive();
  void SetCodepage(int codepage);
  virtual bool IsReadOnly();

#ifdef USE_ZLIB
private:
  virtual bool doRead(FileData &d);
  virtual bool doWritePrepare();
  virtual bool doWrite(FileData &d);
  virtual bool doRename(FileData &fd, const std::string& newname);
  virtual bool doOpen();
  virtual bool doClose();
  virtual bool doDelete(const FileData& fd);
  virtual bool doCreate(const std::string& newpath);

  int codepage_;
  zip_t *archive_;
  int zip_error_;
#endif
};

class DirectoryBinary : public DirectoryFolder
{
public:
  DirectoryBinary();
  virtual bool IsReadOnly();

  // Some file (ex: lr2course, vos) won't behave in form of multiple file.
  // In this case, we use data-ptr reserved for raw format
  // instead of data-key mapping list.
  virtual bool AddFileData(FileData& d, bool setdirty = true, bool copy = false);
  virtual bool AddFile(const std::string &relpath , bool setdirty = true);
  FileData* GetDataPtr();

private:
  virtual bool doOpen();
};

class DirectoryFactory
{
public:
  static DirectoryFactory& Create(const std::string& path);
  Directory* GetDirectory();
  DirectoryFactory& SetFilter(const char** filter_ext);
  bool Open();
  ~DirectoryFactory();
private:
  DirectoryFactory(const std::string& path);
  const std::string path_;
  Directory* directory_;
  bool is_directory_fetched_;
};

}

#endif