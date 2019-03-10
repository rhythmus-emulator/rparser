/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "common.h"
#include "Note.h"
#include "TempoData.h"
#include "MetaData.h"

namespace rparser
{

class ConditionStatement;

/*
 * \detail
 * Contains all object consisting chart in primitive form, without calculating time and sorting.
 *
 * \params
 * metadata_ metadata to be used (might be shared with other charts)
 * timingdata_ timingdata to be used (might be shared with other charts)
 */
class Chart
{
/* iterators */
public:
  Chart(const MetaData *md, const NoteData *nd);
  Chart(const Chart &nd);
  ~Chart();

  NoteData& GetNoteData();
  MetaData& GetMetaData();
  const NoteData& GetNoteData() const;
  const MetaData& GetMetaData() const;
  const TempoData& GetTempoData() const;
  const NoteData& GetSharedNoteData() const;
  const MetaData& GetSharedMetaData() const;
  
  void Clear();

  ConditionStatement* CreateStmt(int value, bool israndom, bool isswitchstmt);
  ConditionStatement* GetLastStmt();
  void AppendStmt(ConditionStatement& stmt);
  // evaluate all stmt and generate objects (process control flow)
  void EvaluateStmt(int seed = -1);

  void swap(Chart& c);

  virtual std::string toString() const;

  void InvalidateAllNotePos();
  void InvalidateNotePos(Note &n);
  void InvalidateTempoData();

  // others
  bool IsEmpty();

private:
  const NoteData* notedata_shared_;
  const MetaData* metadata_shared_;
  NoteData notedata_;
  MetaData metadata_;
  TempoData tempodata_;
  std::vector<ConditionStatement> stmtdata_;
};

/*
 * \detail
 * Conditional Flow Statements
 */
class ConditionStatement
{
public:
  void AddSentence(unsigned int cond, Chart* chartdata);
  Chart* EvaluateSentence(int seed = -1) const;
  size_t GetSentenceCount();
  Chart* CreateSentence(unsigned int cond);

  ConditionStatement(int value, bool israndom, bool isswitchstmt);
  ConditionStatement(const ConditionStatement& cs);
  ~ConditionStatement();
private:
  mutable int value_;
  bool israndom_;
  bool isswitchstmt_;
  std::map<int, Chart*> sentences_;
};

} /* namespace rparser */

#endif
