/*
 * by @lazykuna, MIT License.
 * 
 */

#ifndef RPARSER_DIRECTORY_H
#define RPARSER_DIRECTORY_H

#include "rutil.h"
#include "Error.h"

#ifdef USE_ZLIB
struct zip;
typedef struct zip zip_t;
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
  Directory(const std::string& path, DIRECTORY_TYPE directory_type);
  virtual ~Directory();

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

  bool Open();

  // Open files which located in same directory.
  // (This method is for DirectoryBinary. No effect for general directory.)
  virtual void OpenCurrentDirectory();

  // Flush all changes into file and reset all dirty flags.
  // If succeed, return true. else, return false and canceled.
  // Detailed error message is stored in error_msg_
  bool Save();
  bool SaveAs(const std::string& newpath);

  // Unload all Directory and allocated memory.
  // You can save data with setting parameter flush=true.
  bool Clear(bool flush = true);
  // Just unload currently loaded files.
  void UnloadFiles();
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
  bool Exist(const std::string &name, bool use_alternative_search = false) const;
  FileData* Get(const std::string &name, bool use_alternative_search = false);
  uint8_t* Get(const std::string &name, int &len, bool use_alternative_search = false);
  data_iter begin() noexcept;
  data_iter end() noexcept;
  const data_constiter begin() const noexcept;
  const data_constiter end() const noexcept;
  size_t count() const noexcept;

  bool GetFile(const std::string& filename, const char** out, size_t &len) const noexcept;
  void SetFile(const std::string& filename, const char* p, size_t len) noexcept;

private:
  std::string path_;
  std::string dirpath_;
  std::string file_ext_;
  DIRECTORY_TYPE directory_type_;
  ERROR error_code_;
  bool is_dirty_;
  const char **filter_ext_;

  /**
   * @brief marking for open status.
   * 0: not opened
   * 1: directory opened (now re-opening skipped without force flag)
   * 2: files are readed (now re-reading skipped without force flag)
   */
  int open_status_;

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
  DirectoryFolder(const std::string& path);
  virtual bool IsReadOnly();

private:
  virtual bool doRead(FileData &d);
  virtual bool doWritePrepare();
  virtual bool doWrite(FileData &d);
  virtual bool doRename(FileData &fd, const std::string& newname);
  virtual bool doOpen();
  virtual bool doDelete(const FileData& fd);
  virtual bool doCreate(const std::string& newpath);

protected:
  bool doOpenDirectory(const std::string& dirpath);
};

class DirectoryArchive : public DirectoryFolder
{
public:
  DirectoryArchive();
  DirectoryArchive(const std::string& path);
  void SetCodepage(int codepage);
  virtual bool IsReadOnly();

private:
  int codepage_;

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

  zip_t *archive_;
  int zip_error_;
#endif
};

class DirectoryBinary : public DirectoryFolder
{
public:
  DirectoryBinary();
  virtual bool IsReadOnly();
  virtual void OpenCurrentDirectory();

  // Some file (ex: lr2course, vos) won't behave in form of multiple file.
  // In this case, we use data-ptr reserved for raw format
  // instead of data-key mapping list.
  virtual bool AddFileData(FileData& d, bool setdirty = true, bool copy = false);
  virtual bool AddFile(const std::string &relpath , bool setdirty = true);
  FileData* GetDataPtr();

private:
  bool allow_multiple_files_;
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

  // Special flag for loading specific chart without others.
  std::string bms_chart_file_filter_;
};

/* @brief A user customizeable directory creator */
typedef Directory* (*dir_constructor)(const char*);

/* @brief A singleton class open / close directory and read files in need. */
class DirectoryManager
{
public:
  /* @brief Open directory - read all file lists, not reading them. */
  static bool OpenDirectory(const std::string& dir_or_filepath);

  /* @brief Create directory object. */
  static bool CreateDirectory(const std::string& dirpath);

  /* @brief Read all files in directory (kind of caching) */
  static bool ReadDirectoryFiles(const std::string& dir_or_filepath);

  /* @brief Save all files in directory */
  static bool SaveDirectory(const std::string& dirpath);

  /* @brief Check specified directory is already opened */
  static bool IsDirectoryOpened(const std::string& dirpath);

  /* @brief Closes and returns resource occupied by directory */
  static void CloseDirectory(const std::string& dir_or_filepath);

  /* @brief Get directory object. Should already opened, or nullptr. */
  static std::shared_ptr<Directory> GetDirectory(const std::string& dirpath);

  /* @brief Get file data for read-only */
  static bool GetFile(const std::string& filepath,
    const char** out, size_t &len, bool open_if_not_exist = false);

  /* @brief Copy file data. */
  static bool CopyFile(const std::string& filepath,
    char** out, size_t &len, bool open_if_not_exist = false);

  /* @brief Set file data */
  static void SetFile(const std::string& filepath,
    const char* p, size_t len);

  /* @brief Set directory constructor for specific extension to extend feature */
  void AddDirectoryConstructor(const std::string& ext, dir_constructor constructor);

  static DirectoryManager& getInstance();

private:
  /* This class is singleton so don't have to be created from other */
  DirectoryManager();
  ~DirectoryManager();

  /* @brief regulate path. internal function. */
  static std::string GetSafePath(const std::string& path);

  /**
   * @brief get directory name & file name from given path string
   * e.g. ./abc/abc.zip|x/a.jpg --> ./abc/abc.zip,  x/a.jpg
   *      ./abc/abcd/a.jpg      --> ./abc/abcd,     a.jpg
   */
  static void SeparatePath(const std::string& path, std::string& dir_out, std::string& fn_out);

  /* @brief get safe directory path if directory or filepath is given.
   * @warn target directory(file) should must exist already. */
  static std::string GetSafeDirectory(const std::string& dir_or_filepath);

  /* @brief construct directory object automatically by its extension */
  Directory* CreateDirectoryObject(const std::string& dirpath);

  /* directory_path, directory object */
  std::map<std::string, std::shared_ptr<Directory> > dir_container_;
  std::map<std::string, dir_constructor> ext_dirconstructor_map_;
};

}

#endif
