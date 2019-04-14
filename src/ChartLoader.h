/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTELOADER_H
#define RPARSER_NOTELOADER_H

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
 */
class ChartLoader {
public:
  ChartLoader(): chart_(0), error_(0) {};
  void SetChartList(ChartListBase *chartlist);
  void SetChart(Chart *c);
  virtual bool Test( const void* p, int iLen ) = 0;
  virtual bool TestName( const char *fn ) = 0;
  virtual bool Load( const void* p, int iLen ) = 0;
  virtual bool LoadCommonData( ChartListBase& chartlist, const Directory& dir ) = 0;
protected:
  ChartListBase *chartlist_;
  Chart *chart_;
  int error_;
};


class ChartLoaderBMS : public ChartLoader {
private:
  Chart* chart_context_;
  std::vector<Chart*> chart_context_stack_;
  ConditionStatement* condstmt_;
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
public:
  ChartLoaderBMS();
  bool Test( const void* p, int iLen );
  bool TestName( const char *fn );
  bool Load( const void* p, int iLen );
};

enum VOS_VERSION {
  VOS_UNKNOWN = 0,
  VOS_V2 = 2,
  VOS_V3 = 3
};

class ChartLoaderVOS : public ChartLoader {
private:
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
public:
  ChartLoaderVOS();
  bool Test( const void* p, int iLen );
  bool TestName( const char *fn );
  bool Load( const void* p, int iLen );
};

ChartLoader* CreateChartLoader(SONGTYPE songtype);
int LoadChart( const std::string& fn, SONGTYPE songtype = SONGTYPE::NONE );
int LoadChart( const void* p, int iLen, SONGTYPE songtype = SONGTYPE::NONE );

}

#endif