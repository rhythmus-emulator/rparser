#include "Chart.h"
#include "rutil.h"

using namespace rutil;

namespace rparser
{

Chart::Chart(MetaData *md, TimingObjVec *td)
	: metadata_(md), tobjs_(td)
{
}

Chart::Chart(Chart &nd)
{
  Chart(&nd.GetMetaData(), &nd.GetTimingObjs());
	for (auto note : nd.notes_)
	{
		notes_.push_back(Note(note));
	}
	for (auto stmt : nd.stmts_)
	{
		stmts_.push_back(ConditionStatement(stmt));
	}
	for (auto action : nd.actions_)
	{
		actions_.push_back(Action(action));
	}
}

Chart::~Chart()
{
}

void Chart::swap(Chart& c)
{
	notes_.swap(c.notes_);
	actions_.swap(c.actions_);
	stmts_.swap(c.stmts_);
	tobjs_->swap(*c.tobjs_);
	metadata_->swap(*c.metadata_);
}


void Chart::Clear()
{
	notes_.clear();
	actions_.clear();
	stmts_.clear();
	metadata_->Clear();
	tobjs_->clear();
}

void Chart::Merge(const Chart &cd, rowid rowFrom)
{
	notes_.insert(notes_.end(), cd.notes_.begin(), cd.notes_.end());
	actions_.insert(actions_.end(), cd.actions_.begin(), cd.actions_.end());
}

void Chart::AppendStmt(ConditionStatement& stmt)
{
	stmts_.push_back(stmt);
}

void Chart::EvaluateStmt(int seed)
{
	for (ConditionStatement stmt : stmts_)
	{
		Chart *c = stmt.EvaluateSentence(seed);
		if (c)
			Merge(*c);
	}
}

NoteVec& Chart::GetNotes()
{
	return notes_;
}

ActionVec& Chart::GetActions()
{
	return actions_;
}

TimingObjVec& Chart::GetTimingObjs()
{
	return *tobjs_;
}

MetaData& Chart::GetMetaData()
{
	return *metadata_;
}

const NoteVec& Chart::GetNotes() const
{
	return notes_;
}

const ActionVec& Chart::GetActions() const
{
	return actions_;
}

const TimingObjVec& Chart::GetTimingObjs() const
{
	return *tobjs_;
}

const MetaData& Chart::GetMetaData() const
{
	return *metadata_;
}

ChartBMS::ChartBMS() : Chart(new MetaData(), new TimingObjVec())
{
	tobjs_saved_ = &GetTimingObjs();
	metadata_saved_ = &GetMetaData();
}

ChartBMS::ChartBMS(const Chart& c)
  : Chart(new MetaData(c.GetMetaData()), new TimingObjVec(c.GetTimingObjs()))
{
	tobjs_saved_ = &GetTimingObjs();
	metadata_saved_ = &GetMetaData();
}

ChartBMS::~ChartBMS()
{
	delete tobjs_saved_;
	delete metadata_saved_;
}

void ConditionStatement::AddSentence(unsigned int cond, Chart* chartdata)
{
	sentences_[cond] = chartdata;
}

Chart* ConditionStatement::EvaluateSentence(int seed)
{
	unsigned int cur_seed = seed;
	if (cur_seed < 0)
	{
		cur_seed = rand() % value_;
	}
	auto it = sentences_.find(cur_seed);
	if (it == sentences_.end())
		return 0;
	return it->second;
}

ConditionStatement::ConditionStatement(int value, bool israndom, bool isswitchstmt)
	: value_(value), israndom_(israndom), isswitchstmt_(isswitchstmt)
{
	assert(value_ > 0);
}

ConditionStatement::~ConditionStatement()
{
	for (auto p : sentences_)
	{
		delete p.second;
	}
}

MixingData::MixingData() : metadata_alloced_(0)
{
	timingdata_ = new TimingData();
	metadata_ = new MetaData();
}

MixingData::MixingData(const Chart& c, bool deepcopy)
	: metadata_alloced_(0)
{
	MixingNote mn;

	timingdata_ = new TimingData();
	metadata_ = &c.GetMetaData();
  FillTimingData(*timingdata_, c);
	for (auto n : c.GetNotes())
	{
		// TODO: fill more note data
		mn.n = &n;
		mixingnotes_.push_back(mn);
	}
	if (deepcopy)
	{
		Note *n;
		metadata_ = metadata_alloced_ = new MetaData(metadata_);
		for (auto mn : mixingnotes_)
		{
			n = new Note(mn.n->second);
			notes_alloced_.push_back(n);
			mn.n = n;
		}
	}
	// TODO: fill more note data (timing based)
}

std::vector<MixingNote>& MixingData::GetNotes()
{
	return mixingnotes_;
}

const TimingData& MixingData::GetTimingdata() const
{
	return *timingdata_;
}

const MetaData& MixingData::GetMetadata() const
{
	return *metadata_;
}

MixingData::~MixingData()
{
	for (auto n : notes_alloced_)
		delete n;
	delete metadata_alloced_;
	delete timingdata_;
}

} /* namespace rparser */
