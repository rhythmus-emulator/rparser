/*
 * by @lazykuna, MIT License.
 * 
 */

#ifndef RPARSER_DIRECTORY_H
#define RPARSER_DIRECTORY_H

#include "rutil.h"
#include <mutex>
#include "Error.h"

#ifdef USE_ZLIB
struct zip;
typedef struct zip zip_t;
#endif

namespace rparser
{

class DirectoryManager;

class Directory
{
public:
  Directory();
  Directory(const std::string& path);
  virtual ~Directory();

  // Open file in specific encoding (in case of archive file, e.g. zip).
  // encoding set to 0 means library will automatically find encoding.
  // opened file name automatically converted into UTF8, even in windows.
  // filter_ext is filtering extension to be stored in memory.
  // if filter_ext set to 0, then all file is read.
  bool Open();

  // Flush all changes into file and reset all dirty flags.
  // If succeed, return true. else, return false and canceled.
  // Detailed error message is stored in error_msg_
  bool Save();
  bool SaveAs(const std::string& newpath);

  // Unload all Directory and allocated memory.
  // You can save data with setting parameter flush=true.
  bool Clear(bool flush = false);

  // Just unload currently loaded files.
  void UnloadFiles();

  // Just close directory handle.
  bool Close(bool flush = false);

  // if Closed, then readonly state.
  virtual bool IsReadOnly();

  // Change file saving path
  void SetPath(const std::string& path);

  // Set destination path to save
  // Don't check existence for given path.
  const std::string &GetPath() const;
  std::string GetRelativePath(const std::string &orgpath) const;
  std::string GetAbsolutePath(const std::string &relpath) const;
  const char* GetErrorMsg() const;
  bool IsLoaded();

  // TODO:
  // Lock while Add/Delete/Renaming object (addr may changed due to vector realloc)
  bool Delete(const std::string &name);
  bool Rename(const std::string &prev_name, const std::string &new_name);
  bool Read(const std::string& filename, bool force = false);
  bool ReadAll(bool force = false);
  bool AddExternalFile(const std::string &path, const std::string& new_name);

  struct File
  {
    std::string filename;
    char* p;
    size_t len;
  };
  typedef std::vector<File>::iterator data_iter;
  typedef std::vector<File>::const_iterator data_constiter;
  data_iter begin() noexcept;
  data_iter end() noexcept;
  const data_constiter begin() const noexcept;
  const data_constiter end() const noexcept;
  size_t count() const noexcept;

  bool Exist(const std::string &name) const;

  // @warn
  // It internally calls doRead() method to fill file object,
  // but it may null size file return (but should not happen ..?).
  // be sure to check returning file pointer.
  bool GetFile(const std::string& filename, const char** out, size_t &len) const noexcept;

  void SetFile(const std::string& filename, const char* p, size_t len) noexcept;

  /**
   * @brief
   * turn on or off whether to use alternative file search
   * alternative file search: find file without directory & extension
   * (only with filename)
   */
  void SetAlternativeSearch(bool use_alternative_search = true);

  friend class DirectoryManager;

private:
  std::string dirpath_;
  ERROR error_code_;
  bool use_alternative_search_;

  /* @brief reference count. used by DirectoryManager class internally. */
  int ref_cnt_;

  /**
   * @brief marking for open status.
   * 0: not opened
   * 1: directory opened (now re-opening skipped without force flag)
   */
  int open_status_;

  virtual bool doRead(File &f);
  virtual bool doWritePrepare();
  virtual bool doWrite(File &f);
  virtual bool doRename(const std::string& oldname, const std::string& newname);
  virtual bool doOpen();
  virtual bool doClose();
  virtual bool doDelete(const std::string& filename);
  virtual bool doCreate(const std::string& newpath);

protected:
  std::vector<File> files_;
  void ClearStatus();
  void SetError(ERROR error);
  void CreateEmptyFile(const std::string& filename);
};

class DirectoryFolder : public Directory
{
public:
  DirectoryFolder();
  DirectoryFolder(const std::string& path);
  virtual bool IsReadOnly();

private:
  virtual bool doRead(File &f);
  virtual bool doWritePrepare();
  virtual bool doWrite(File &f);
  virtual bool doRename(const std::string& oldname, const std::string& newname);
  virtual bool doOpen();
  virtual bool doDelete(const std::string& filename);
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

private:
  virtual bool doRead(File &f);
  virtual bool doWritePrepare();
  virtual bool doWrite(File &f);
  virtual bool doRename(const std::string& oldname, const std::string& newname);
  virtual bool doOpen();
  virtual bool doClose();
  virtual bool doDelete(const std::string& filename);
  virtual bool doCreate(const std::string& newpath);

  std::mutex mutex_;

#ifdef USE_ZLIB
  zip_t *archive_;
  int zip_error_;
#endif
};


/* @brief A user customizeable directory creator */
typedef Directory* (*dir_constructor)(const char*);

/**
 * @brief
 * A singleton class open / close directory and read files in need.
 * Thread-safe.
 */
class DirectoryManager
{
public:
  /* @brief Open directory - read all file lists, not reading them. */
  static bool OpenDirectory(const std::string& dirpath);

  /* @brief Create directory object. */
  static bool CreateDirectory(const std::string& dirpath);

  /* @brief Read all files in directory (kind of caching) */
  static bool ReadDirectoryFiles(const std::string& dir_or_filepath);

  /* @brief Save all files in directory */
  static bool SaveDirectory(const std::string& dirpath);

  /* @brief Check specified directory is already opened */
  static bool IsDirectoryOpened(const std::string& dirpath);

  /**
   * @brief Closes and returns resource occupied by directory
   * @param force close directory without checking reference count.
   */
  static void CloseDirectory(const std::string& dirpath, bool force = false);

  /* @brief Get directory object. Should already opened, or nullptr. */
  static std::shared_ptr<Directory> GetDirectory(const std::string& dirpath);

  /* @brief Get file data for read-only */
  static bool GetFile(const std::string& filepath,
    const char** out, size_t &len);

  /* @brief Copy file data. */
  static bool CopyFile(const std::string& filepath,
    char** out, size_t &len);

  /* @brief Set file data */
  static void SetFile(const std::string& filepath,
    const char* p, size_t len);

  /* @brief Open directory & file at both time. (add directory ref_cnt) */
  static bool OpenAndGetFile(const std::string& filepath,
    const char** out, size_t &len);

  /* @brief Open directory & file at both time. (add directory ref_cnt) */
  static bool OpenAndCopyFile(const std::string& filepath,
    char** out, size_t &len);

  /* @brief Close directory & file at both time. (reducing directory ref_cnt) */
  static void CloseFile(const std::string& filepath);

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
