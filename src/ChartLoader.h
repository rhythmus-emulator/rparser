/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTLOADER_H
#define RPARSER_CHARTLOADER_H

#include "Song.h"
#include "Chart.h"
#include "Directory.h"
#include "rutil.h"

namespace rparser {

class Chart;
class Song;

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
  ChartLoader(Song* song) : song_(song), error_(0), seed_(0) {};
  virtual bool Test(const void* p, unsigned iLen);
  virtual void SetSeed(int seed = -1);

  /* @brief used for chart which song exists in a single binary
   * (e.g. VOS) */
  virtual bool Load(Chart &c, const void* p, unsigned iLen) = 0;

  /* @brief used for files which song existing in directory
   * (e.g. most types of song) 
   * internally calls Load() method. */
  virtual bool LoadFromDirectory() = 0;

  static ChartLoader* Create(Song *song);

  // XXX: A global setting whether to load BMS file in edit mode or not.
  //      It's a kind of internal variable so it's roughly in temp static variable.
  //      Should be fixed later.
  static bool bOpenBmsFileWithoutProcessing;

protected:
  Song *song_;
  int error_;
  int seed_;

  /* @brief preprocess when loading chart. */
  void Preload(Chart &c, const void* p, int iLen);
};


class ChartLoaderBMS : public ChartLoader {
public:
  ChartLoaderBMS(Song* song);
  virtual bool Test(const void* p, unsigned iLen);
  virtual bool Load(Chart &c, const void* p, unsigned iLen);
  virtual bool LoadFromDirectory();

  // Process command to chart data without clearing.
  void ProcessCommand(Chart &c, const char* p, unsigned len);

  void ProcessConditionalStatement(bool do_process = true);
private:
  Chart * chart_context_;
  uint32_t longnote_idx_per_lane[128];
  std::map<int, uint8_t> bgm_column_idx_per_measure_;

  struct CondContext {
    int condblock;
    int condcur;
    int condprocessed;
    int condcheckedcount;
    bool parseable;
  };
  std::vector<CondContext> cond_;

  struct LineContext {
    const char* stmt;
    unsigned stmt_len;
    char command[256];
    const char* value;
    unsigned int value_len;
    unsigned int measure;
    unsigned int bms_channel;
    char terminator_type;

    void clear();
    LineContext();
  };
  std::vector<LineContext*> parsing_buffer_;
  LineContext* current_line_;
  rutil::Random random_;
  bool process_conditional_statement_;

  bool IsCurrentLineIsConditionalStatement();
  bool ParseCurrentLine();
  bool ParseControlFlow();
  bool ParseMetaData();
  bool ParseMeasureLength();
  bool ParseNote();
  bool ParseBgaNote();
  bool ParseBgmNote();
  bool ParseSoundNote();
  bool ParseTimingNote();
  bool ParseEffectNote();

  // Finish parsing by flush parsing buffer to chart data.
  void FlushParsingBuffer();

  struct
  {
    unsigned int measure;
    unsigned int channel;
    unsigned int deno, num;
    unsigned int value_u;
    const char* value;
  } curr_note_syntax_;
};

enum VOS_VERSION {
  VOS_UNKNOWN = 0,
  VOS_V2 = 2,
  VOS_V3 = 3
};

enum class MIDISIG;
struct MIDIProgramInfo;

class ChartLoaderVOS : public ChartLoader {
public:
  ChartLoaderVOS(Song* song);
  virtual bool Load(Chart &c, const void* p, unsigned iLen);
  virtual bool LoadFromDirectory();
private:
  Chart *chart_;
  int vos_version_;
  size_t vos_v3_midi_offset_;
  uint32_t timedivision_;
  bool ParseVersion();
  bool ParseMetaDataV2();
  bool ParseMetaDataV3();
  bool ParseNoteDataV2();
  bool ParseNoteDataV3();
  bool ParseMIDI();

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
    size_t GetOffset();
    int GetMSInt();
    int GetMSFixedInt(uint8_t bytesize=4);
    bool IsEnd();
    MIDISIG GetMidiSignature(MIDIProgramInfo& mprog);
  private:
    const void* p_;
    unsigned len_;
    size_t offset_;
  } stream;
};

}

#endif
