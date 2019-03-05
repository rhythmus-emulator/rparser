/*
 * by @lazykuna, MIT License.
 * 
 */

#ifndef RPARSER_RESOURCE_H
#define RPARSER_RESOURCE_H

#include "rutil.h"
#include "Error.h"

namespace rparser
{

enum class RESOURCE_TYPE
{
	NONE,
	FOLDER,
	ARCHIVE,
	BINARY,
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
	bool Open(const char* filepath);

	// Flush all changes into file and reset all dirty flags.
	// If succeed, return true. else, return false and canceled.
	// Detailed error message is stored in error_msg_
  virtual bool Flush();

	// Unload all resource and free allocated memory.
	// You can save data with setting parameter flush=true.
  virtual bool Unload(bool flush = true);

	// Set destination path to save
	// Don't check existence for given path.
	void SetPath(const char* filepath);
  void SetExtension(const char* extension);
  const std::string GetPath() const;
  const std::string GetDirectoryPath() const;
	RESOURCE_TYPE GetResourceType() const;
	const char* GetErrorMsg() const;
	bool IsLoaded();

private:
	std::string path_;
	std::string dirpath_;
	std::string file_ext_;
	RESOURCE_TYPE resource_type_;
  ERROR error_code_;
  bool is_dirty_;

  virtual bool doOpen();
  virtual bool doFlush();
  virtual bool doUnload();

protected:
  static bool Read_from_fp(FILE *fp, BinaryData& d);
  static bool Write_from_fp(FILE *fp, BinaryData& d);
  void ClearStatus();
  Resource(RESOURCE_TYPE restype);
  void SetError(ERROR error);
  void SetDirty(bool flag = true);
};


class ResourceFolder : public Resource
{
public:
  ResourceFolder();
  void SetFilter(const char* filter_ext);
  void AddBinary(const std::string &name, char *p, unsigned int len, bool setdirty = true, bool copy = false);
  bool AddFile(const std::string &name, const std::string &filename, bool setdirty = true);
  bool Rename(const std::string &prev_name, const std::string &new_name);
  bool Delete(const std::string &name);
  const BinaryData* GetPtr(const std::string &name) const;
  const char* GetPtr(const std::string &name, int &len) const;
  // Filter out files
  void FilterFiles(const char* filters,
    std::map<std::string, const BinaryData*>& chart_files);

private:
  virtual bool doOpen();
  virtual bool doFlush();
  virtual bool doUnload();
  static bool WriteBinary(const char* filepath, BinaryData& d);

  std::map<std::string, BinaryData> datas_;
  std::map<std::string, bool> data_dirty_flag_;
  std::string filter_ext_;
};

#ifdef USE_ZLIB
class ResourceArchive : public ResourceFolder
{
public:
  ResourceArchive();

private:
  virtual bool doOpen();
  virtual bool doFlush();
  virtual bool doUnload();

	bool Load_from_zip(FILE *fp);	// TODO
	bool WriteZip();	// TODO
};
#endif

class ResourceBinary : public Resource
{
public:
  ResourceBinary();

  // Some file (ex: lr2course, vos) won't behave in form of multiple file.
  // In this case, we use data-ptr reserved for raw format
  // instead of data-key mapping list.
  void AllocateRawBinary(char *p, unsigned int len, bool copy = false);
  const BinaryData* GetRawPtr() const;
  const char* GetRawPtr(int &len) const;

private:
  virtual bool doOpen();
  virtual bool doFlush();
  virtual bool doUnload();

  BinaryData data_raw_;
};

class ResourceFactory
{
public:
  static Resource* Open(const char* path);
};

}

#endif