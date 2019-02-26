/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTELOADER_H
#define RPARSER_NOTELOADER_H

#include "Song.h"
#include "Chart.h"

namespace rparser {

class Chart;

class ChartLoader {
protected:
  Chart* chart_;
  int error_;
  std::string filename_;
public:
  ChartLoader(Chart* c): chart_(c), error_(0) {};
  // @description sometimes chart loading process is dependent with filename ...
  void SetFilename(const std::string& filename);
  virtual bool Test( const void* p, int iLen ) = 0;
  virtual bool TestName( const char *fn ) = 0;
  virtual bool Load( const void* p, int iLen ) = 0;
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
  ChartLoaderBMS(Chart* c);
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
  ChartLoaderVOS(Chart* c);
  bool Test( const void* p, int iLen );
  bool TestName( const char *fn );
  bool Load( const void* p, int iLen );
};

ChartLoader* CreateChartLoader(SONGTYPE songtype);
int LoadChart( const std::string& fn, SONGTYPE songtype = SONGTYPE::NONE );
int LoadChart( const void* p, int iLen, SONGTYPE songtype = SONGTYPE::NONE );

}

#endif