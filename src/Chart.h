/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "MetaData.h"
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

/*
 * \detail
 * ChartData consists with track / timingdata / metadatas, parsed for each section.
 */
class Chart
{
/* iterators */
public:
	Chart(MetaData *md, TimingData *td);
	Chart(const Chart &nd);
	~Chart();

    typedef std::vector<Note>::iterator noteiter;
    typedef std::vector<Note>::const_iterator const_noteiter;
	noteiter begin() { return notes_.begin(); };
	noteiter end() { return notes_.end(); };
	const_noteiter begin() const { return notes_.begin(); };
	const_noteiter end() const { return notes_.end(); };
	noteiter lower_bound(int row);
	noteiter upper_bound(int row);
	noteiter lower_bound(Note &n) { return std::lower_bound(notes_.begin(), notes_.end(), n); };
	noteiter upper_bound(Note &n) { return std::upper_bound(notes_.begin(), notes_.end(), n); };


	typedef std::vector<TimingObject>::iterator tobjiter;
	typedef std::vector<TimingObject>::const_iterator const_tobjiter;
	tobjiter tobj_begin() { return timingobjs_.begin(); };
	tobjiter tobj_end() { return timingobjs_.end(); };
	const_tobjiter tobj_begin() const { return timingobjs_.begin(); };
	const_tobjiter tobj_end() const { return timingobjs_.end(); };
	tobjiter tobj_lower_bound(int row);
	tobjiter tobj_upper_bound(int row);
	tobjiter tobj_lower_bound(Note &n) { return std::lower_bound(timingobjs_.begin(), timingobjs_.end(), n); };
	tobjiter tobj_upper_bound(Note &n) { return std::upper_bound(timingobjs_.begin(), timingobjs_.end(), n); };


	typedef std::vector<Action>::iterator actiter;
	typedef std::vector<Action>::const_iterator const_actiter;
	actiter act_begin() { return actions_.begin(); };
	actiter act_end() { return actions_.end(); };
	const_actiter act_begin() const { return actions_.begin(); };
	const_actiter act_end() const { return actions_.end(); };
	actiter act_lower_bound(int row);
	actiter act_upper_bound(int row);
	actiter act_lower_bound(Note &n) { return std::lower_bound(actions_.begin(), actions_.end(), n); };
	actiter act_upper_bound(Note &n) { return std::upper_bound(actions_.begin(), actions_.end(), n); };


	const std::vector<Note>* GetNotes() const;
	const std::vector<TimingObject>* GetTimingObjects() const;
	const std::vector<Action>* GetActions() const;
	const MetaData* GetMetaData() const;
	MetaData* GetMetaData();


	// Flush objects at once and sort it rather using AddXX() method for each of them.
	// NOTE: original vector object will be swapped(std::move).
	void Flush(std::vector<Note>& notes,
		std::vector<TimingObject>& tobjs,
		std::vector<Action>& actions);
	// clear all objects. (optional: case statements)
	void Clear(bool removeStmt = true);
	// copy and merge objects from other ChartData (without stmt)
	void Merge(const Chart &cd, bool overwrite = true, rowid rowFrom = 0);
	// evaluate all stmt and generate objects (process control flow)
	void EvaluateStmt();

	void swap(Chart& c);

	virtual std::string toString() const;
public:
	// row scanning utilities (fast-scan)
    bool IsRangeEmpty(unsigned long long startrow, unsigned long long endrow);
	bool IsRowEmpty(unsigned long long row, TRACKTYPE type = TRACKTYPE::TRACK_EMPTY, unsigned int subtype = 0);
	bool IsTrackEmpty(trackinfo track);
	bool IsHoldNoteAtRow(int row, int track = -1);

	// full scan utilities
	bool IsTypeEmpty(TRACKTYPE type, unsigned int subtype = 0);
	bool IsLaneEmpty(unsigned int lane);

	// others
    bool IsEmpty();

private:
	/*
	 * Contains all note objects
	 * (ROW based position; mostly exists lane/channel and not duplicable.)
	 * Includes not only sound/rendering but also special/timing object.
	 * note of same idx + subtype + type + row should be overwritten.
	 */
	std::vector<Note> notes_;
	/*
	 * Contains all timing related objects
	 * (BEAT based position; mostly no lane/channel and duplicable.)
	 * NOTE: do not need to attempt render TimingObject using this data ...
	 *       Row position is already calculated at TimingData.
	 */
	std::vector<TimingObject> timingobjs_;
	/*
	 * Contains all time-reserved objects
	 * (TIME based objects; mostly used for special action [undefined])
	 */
	std::vector<Action> actions_;
	/*
	 * Metadata of this chart section.
	 */
	MetaData metadata_;
	/*
	 * Conditional flow
	 */
	std::vector<ConditionStatement> stmts_;
};

class ChartBMS : public Chart
{
public:
	ChartBMS();
	~ChartBMS();

	TimingData* GetTimineData();
	MetaData* GetMetaData();
private:
	TimingData* timingdata_;
	MetaData* metadata_;
};


/*
 * @description
 * Conditional Flow Statement
 */
class ConditionStatement
{
public:
	void AddSentence(unsigned int cond, Chart* chartdata);
	Chart* EvaluateSentence(int seed = -1);

	ConditionStatement(int value, bool israndom, bool isswitchstmt);
	~ConditionStatement();
private:
	int value_;
	bool israndom_;
	bool isswitchstmt_;
	std::map<unsigned int, Chart*> sentences_;
};

}

#endif
