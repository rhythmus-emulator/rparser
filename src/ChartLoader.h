/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTLOADER_H
#define RPARSER_CHARTLOADER_H

#include "Song.h"
#include "Chart.h"
#include "Directory.h"

namespace rparser {

class Chart;

/**
 * @params
 * SetChartList Set chartlist context to load charts into.
 *              If Chart context is set, then it will be cleared.
 * SetChart Set chart context to load chart.
 *          If ChartList context is set, then it will be cleared.
 * \warn    Each ChartLoader can be used for single thread.
 */
class ChartLoader {
public:
  ChartLoader(): error_(0) {};
  virtual bool LoadFromDirectory(ChartListBase& chartlist, Directory& dir) = 0;
  virtual bool Load(Chart &c, const void* p, int iLen) = 0;
  virtual bool Test(const void* p, int iLen);

  static ChartLoader* Create(SONGTYPE songtype);
protected:
  int error_;
};


class ChartLoaderBMS : public ChartLoader {
public:
  ChartLoaderBMS();
  virtual bool LoadFromDirectory(ChartListBase& chartlist, Directory& dir);
  virtual bool Load(Chart &c, const void* p, int iLen);
  virtual bool Test(const void* p, int iLen);
private:
  Chart * chart_context_;
  std::vector<Chart*> chart_context_stack_;
  ConditionalChart* condstmt_;
  struct LineContext {
    const char* stmt;
    unsigned int stmt_len;
    char command[256];
    const char* value;
    unsigned int value_len;
    unsigned int measure;
    unsigned int bms_channel;

    void clear();
    LineContext();
  } current_line_;

  bool ParseCurrentLine();
  bool ParseControlFlow();
  bool ParseMetaData();
  bool ParseNote();
};

enum VOS_VERSION {
  VOS_UNKNOWN = 0,
  VOS_V2 = 2,
  VOS_V3 = 3
};

class ChartLoaderVOS : public ChartLoader {
public:
  ChartLoaderVOS();
  virtual bool LoadFromDirectory(ChartListBase& chartlist, Directory& dir);
  virtual bool Load(Chart &c, const void* p, int iLen);
private:
  Chart *chart_;
  const unsigned char* p_;
  const unsigned char *note_p_;
  const unsigned char* midi_p_;
  int vos_version_;
  int len_;
  bool ParseVersion();
  bool ParseMetaDataV2();
  bool ParseMetaDataV3();
  bool ParseNoteDataV2();
  bool ParseNoteDataV3();
  bool ParseMIDI();
};

}

#endif