/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "MetaData.h"
#include "TimingData.h"
#include "ChartData.h"
#include <map>
#include <vector>
#include <algorithm>

/*
 * NOTE:
 * ChartData stores note object in semantic objects, not raw object.
 * These modules written below does not contain status about current modification.
 */


namespace rparser
{

class ConditionStatement;

typedef std::vector<TimingObject> TimingObjVec;
typedef std::vector<Action> ActionVec;
typedef std::vector<Note> NoteVec;

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
	Chart(const Chart &nd);
	~Chart();

	NoteVec& GetNotes();
	ActionVec& GetActions();
	TimingObjVec& GetTimingObjs();
	MetaData& GetMetaData();

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
	/*
	 * Contains all timing related objects
	 */
	TimingObjVec* tobjs_;
	/*
	 * Metadata of this chart section.
	 */
	MetaData* metadata_;
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



/*
 * \detail
 * Mixable/Playable (timing info appended) chart object.
 *
 * \warn
 * Dependent to original chart data so don't change it when refering ChartMixing class.
 *
 * \params
 * ...
 */
class ChartMixing
{
public:
	ChartMixing();
	ChartMixing(const Chart& c, bool deepcopy = false);
	~ChartMixing();

	std::vector<MixingNote>& GetNotes();
	TimingData& GetTimingdata();
	MetaData& GetMetadata();
private:
	std::vector<MixingNote> mixingnotes_;
	TimingData* timingdata_;
	MetaData* metadata_;

	std::vector<Note*> notes_alloced_;
	MetaData* metadata_alloced_;
};

}

#endif
