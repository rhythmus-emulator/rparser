#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "Song.h"
#include "ChartUtil.h"

#ifdef _WIN32
# define DIR_SEP '\\'
#else
# define DIR_SEP '/'
#endif

class ArgCmd
{
public:
  ArgCmd() {};
  ArgCmd(bool is_booltype, const std::string& argname, const std::string& desc, const std::string& _default);
  template <typename T> T get() const;
  std::string toString() const;
private:
  bool _is_booltype;
  std::string _argname;
  std::string _val;
  std::string _desc;
  friend class ArgParams;
};

ArgCmd::ArgCmd(bool is_booltype, const std::string& argname, const std::string& desc, const std::string& _default) :
  _is_booltype(is_booltype), _argname(argname), _val(_default), _desc(desc) {}

template <> bool ArgCmd::get() const { return _val == "true"; }

template <> int ArgCmd::get() const { return atoi(_val.c_str()); }

template <> const char* ArgCmd::get() const { return _val.c_str(); }

std::string ArgCmd::toString() const
{
  std::stringstream ss;
  if (_argname.empty())
    ss << " : " << _desc;
  else
    ss << "(" << _argname << ") : " << _desc;
  if (!_val.empty()) {
    ss << "(default:" << _val << ")";
  }
  return ss.str();
}

class ArgParams
{
public:
  ArgParams();
  ArgParams(int argc, char **argv);
  bool Parse(int argc, char **argv);
  void RegisterCommandBoolean(const std::string& command, const std::string& desc);
  void RegisterCommandBoolean(const std::string& command, const std::string& desc, const std::string& _default);
  void RegisterCommandWithArg(const std::string& command, const std::string& argname, const std::string& desc);
  void RegisterCommandWithArg(const std::string& command, const std::string& argname, const std::string& desc, const std::string& _default);
  template <typename T> T Get(const std::string& command) const;
  const std::string& GetParam(unsigned i) const;
  unsigned GetParamCount() const;
  void PrintHelp();
private:
  std::vector<std::string> _params;
  std::map<std::string, ArgCmd> _argcmds;
};

ArgParams::ArgParams() {}

ArgParams::ArgParams(int argc, char **argv) { Parse(argc, argv); }

bool ArgParams::Parse(int argc, char **argv)
{
  for (int i = 1; i < argc; ++i) {
    auto cmd = _argcmds.find(argv[i]);
    if (cmd == _argcmds.end()) {
      _params.push_back(argv[i]);
      continue;
    }
    if (cmd->second._is_booltype) {
      cmd->second._val = "true";
    }
    else {
      if (i == argc - 1)
        return false;
      else
        cmd->second._val = argv[++i];
    }
  }
  return true;
}

void ArgParams::RegisterCommandBoolean(const std::string& command, const std::string& desc)
{
  _argcmds[command] = ArgCmd(true, std::string(), desc, std::string());
}

void ArgParams::RegisterCommandBoolean(const std::string& command, const std::string& desc, const std::string& _default)
{
  _argcmds[command] = ArgCmd(true, std::string(), desc, _default);
}

void ArgParams::RegisterCommandWithArg(const std::string& command, const std::string& argname, const std::string& desc)
{
  _argcmds[command] = ArgCmd(false, argname, desc, std::string());
}

void ArgParams::RegisterCommandWithArg(const std::string& command, const std::string& argname, const std::string& desc, const std::string& _default)
{
  _argcmds[command] = ArgCmd(false, argname, desc, _default);
}

template <> bool ArgParams::Get(const std::string& command) const
{
  auto it = _argcmds.find(command);
  if (it == _argcmds.end()) return false;
  else return it->second.get<bool>();
}

template <> int ArgParams::Get(const std::string& command) const
{
  auto it = _argcmds.find(command);
  if (it == _argcmds.end()) return 0;
  else return it->second.get<int>();
}

template <> const char* ArgParams::Get(const std::string& command) const
{
  auto it = _argcmds.find(command);
  if (it == _argcmds.end()) return nullptr;
  else return it->second.get<const char*>();
}

const std::string& ArgParams::GetParam(unsigned i) const { return _params[i]; }

unsigned ArgParams::GetParamCount() const { return _params.size(); }

void ArgParams::PrintHelp()
{
  std::cout << "RParser v0.1 (" << __DATE__ << " " << __TIME__ << ")\n" << std::endl;
  std::cout << "Usage: (program) (args)\n" << std::endl;
  for (auto it = _argcmds.begin(); it != _argcmds.end(); ++it) {
    std::cout << it->first << " " << it->second.toString() << std::endl;
  }
}

std::string GetNewFilename(const std::string& in, const std::string& new_folder, const std::string& ext) {
  std::string folder;
  std::string filename;
  auto sep_pos = in.find_last_of("\\/");
  auto ext_pos = in.find_last_of('.');
  if (sep_pos == std::string::npos) {
    filename = in;
  }
  else {
    if (new_folder.empty())
      folder = in.substr(0, sep_pos + 1);
    else
      folder = new_folder;
    if (ext_pos != std::string::npos && sep_pos < ext_pos)
      filename = in.substr(sep_pos + 1, ext_pos - sep_pos - 1);
    else
      filename = in.substr(sep_pos + 1);
  }
  return folder + filename + ext;
}


int main(int argc, char **argv)
{
  ArgParams args;
  std::vector<std::string> songfiles;
  std::string output_folder;
  unsigned process_count = 0;
  unsigned process_songs = 0;
  bool export_html = false;
  bool export_profile = false;
  bool is_folder = false;
  bool verbose = false;

  args.RegisterCommandBoolean("--html", "Export chart to HTML file.");
  args.RegisterCommandBoolean("--profile", "Export chart profile data.");
  args.RegisterCommandBoolean("--folder", "Consider data from folder.");
  args.RegisterCommandBoolean("--verbose", "Display detailed message.");
  args.RegisterCommandWithArg("--output", "folder", "Set folder for output.");
  args.RegisterCommandBoolean("--help", "Display this message.");

  if (argc == 1 || !args.Parse(argc, argv) || args.Get<bool>("--help")) {
    args.PrintHelp();
    return 0;
  }

  export_html = args.Get<bool>("--html");
  export_profile = args.Get<bool>("--profile");
  is_folder = args.Get<bool>("--folder");
  verbose = args.Get<bool>("--verbose");
  
  if (args.Get<const char*>("--output"))
    output_folder = args.Get<const char*>("--output");

  if (is_folder) {
    for (unsigned i = 0; i < args.GetParamCount(); ++i) {
      rutil::DirFileList filelist;
      std::string folder(args.GetParam(i));
      if (!rutil::GetDirectoryFiles(folder, filelist, 0, false)) {
        if (verbose)
          std::cerr << "Failed to open folder: " << folder << std::endl;
        continue;
      }
      for (auto &f : filelist) {
        songfiles.push_back(folder + DIR_SEP + f.first);
      }
    }
  }
  else {
    for (unsigned i = 0; i < args.GetParamCount(); ++i)
      songfiles.push_back(args.GetParam(i));
  }

  for (const auto& songfile : songfiles) {
    rparser::Song song;
    unsigned chart_count = 0;
    if (!song.Open(songfile)) {
      if (verbose)
        std::cerr << "Failed to open song file: " << songfile << std::endl;
      continue;
    }
    chart_count = song.GetChartCount();
    for (unsigned i = 0; i < chart_count; ++i) {
      rparser::Chart* chart = song.GetChart(i);
      const std::string cfilename = chart->GetFilename();
      if (verbose) {
        std::cout << "[ " << static_cast<int>(process_songs * 100.0 / songfiles.size()) << "% (" << process_count << ") ] "
          << cfilename.c_str() << std::endl;
      }
      if (export_html) {
        rparser::HTMLExporter htmlexporter(*chart);
        std::string html = htmlexporter.toHTML();
        std::ofstream ofs(GetNewFilename(cfilename, output_folder, ".html"));
        ofs << html;
        ofs.close();
      }
      if (export_profile) {
        rparser::ChartProfiler cprof(*chart);
        cprof.Save(GetNewFilename(cfilename, output_folder, "_profile.json"));
      }
      ++process_count;
    }
    ++process_songs;
  }

  if (verbose)
    std::cout << "Processed " << process_count << " Charts." << std::endl;
  return 0;
}