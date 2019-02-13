/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "common.h"
#include "MetaData.h"
#include "ChartData.h"

/*
 * NOTE:
 * ChartData stores note object in semantic objects, not raw object.
 * These modules written below does not contain status about current modification.
 */


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
	Chart(MetaData *md, TimingObjVec *td);
	Chart(Chart &nd);
	~Chart();

	NoteVec& GetNotes();
	ActionVec& GetActions();
	TimingObjVec& GetTimingObjs();
	MetaData& GetMetaData();
	const NoteVec& GetNotes() const;
	const ActionVec& GetActions() const;
	const TimingObjVec& GetTimingObjs() const;
	const MetaData& GetMetaData() const;

	void Clear();
	// copy and merge objects from other ChartData (without stmt)
	void Merge(const Chart &cd, rowid rowFrom = 0);

	void AppendStmt(ConditionStatement& stmt);
	// evaluate all stmt and generate objects (process control flow)
	void EvaluateStmt(int seed = -1);

	void swap(Chart& c);

	virtual std::string toString() const;

	// others
    bool IsEmpty();

private:
	mutable TimingObjVec* tobjs_;
	mutable MetaData* metadata_;
	/*
	 * Contains all note objects
	 * (ROW based position; mostly exists lane/channel and not duplicable.)
	 * Includes not only sound/rendering but also special/timing object.
	 * note of same idx + subtype + type + row should be overwritten.
	 */
	std::vector<Note> notes_;
	/*
	 * Contains all time-reserved objects
	 * (TIME based objects; mostly used for special action [undefined])
	 */
	std::vector<Action> actions_;
	/*
	 * Conditional flow
	 */
	std::vector<ConditionStatement> stmts_;
};

class ChartBMS : public Chart
{
public:
	ChartBMS();
	ChartBMS(const Chart& c);
	~ChartBMS();
private:
	TimingObjVec* tobjs_saved_;
	MetaData* metadata_saved_;
};


/*
 * \detail
 * Conditional Flow Statements
 */
class ConditionStatement
{
public:
	void AddSentence(unsigned int cond, Chart* chartdata);
	Chart* EvaluateSentence(int seed = -1);

	ConditionStatement(int value, bool israndom, bool isswitchstmt);
	ConditionStatement(ConditionStatement& cs);
	~ConditionStatement();
private:
	int value_;
	bool israndom_;
	bool isswitchstmt_;
	std::map<int, Chart*> sentences_;
};



}

#endif
