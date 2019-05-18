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
  uint32_t longnote_idx_per_lane[36];
  struct LineContext {
    const char* stmt;
    size_t stmt_len;
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
  bool ParseMeasureLength();
  bool ParseNote();
};

enum VOS_VERSION {
  VOS_UNKNOWN = 0,
  VOS_V2 = 2,
  VOS_V3 = 3
};

enum class MIDISIG;

class ChartLoaderVOS : public ChartLoader {
public:
  ChartLoaderVOS();
  virtual bool LoadFromDirectory(ChartListBase& chartlist, Directory& dir);
  virtual bool Load(Chart &c, const void* p, int iLen);
private:
  Chart *chart_;
  int vos_version_;
  size_t vos_v3_midi_offset_;
  bool ParseVersion();
  bool ParseMetaDataV2();
  bool ParseMetaDataV3();
  bool ParseNoteDataV2();
  bool ParseNoteDataV3();
  bool ParseMIDI();

  struct MIDIProgramInfo {
    uint8_t cmdtype;
    char cmd[2];
  };

  class BinaryStream {
  public:
    BinaryStream();
    void SetSource(const void* p, size_t len);
    void SeekSet(size_t cnt);
    void SeekCur(size_t cnt);
    int32_t ReadInt32();
    uint8_t ReadUInt8();
    int32_t GetInt32();
    uint32_t GetUInt32();
    uint16_t GetUInt16();
    uint8_t GetUInt8();
    void GetChar(char *out, size_t cnt);
    int GetOffset();
    int GetMSInt();
    int GetMSFixedInt(uint8_t bytesize=4);
    bool IsEnd();
    MIDISIG GetMidiSignature(MIDIProgramInfo& mprog);
  private:
    const void* p_;
    int len_;
    size_t offset_;
  } stream;
};

}

#endif