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
  bool ParseTrimmedLine(const char* p, unsigned int len);
  bool ParseControlFlow(const char* command, const char* value, unsigned int len);
  bool ParseMetaData(const char* command, const char* value, unsigned int len);
  bool ParseNote(unsigned int measure, unsigned int bms_channel, const char* value, unsigned int len);
public:
  ChartLoaderBMS(Chart* c);
  bool Test( const void* p, int iLen );
  bool TestName( const char *fn );
  bool Load( const void* p, int iLen );
};


class ChartLoaderVOS : public ChartLoader {
public:
  ChartLoaderVOS(Chart* c): ChartLoader(c) {};
  bool Test( const void* p, int iLen );
  bool TestName( const char *fn );
  bool Load( const void* p, int iLen );
};

ChartLoader* CreateChartLoader(SONGTYPE songtype);
int LoadChart( const std::string& fn, SONGTYPE songtype = SONGTYPE::NONE );
int LoadChart( const void* p, int iLen, SONGTYPE songtype = SONGTYPE::NONE );

}

#endif