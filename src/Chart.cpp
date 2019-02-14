#include "Chart.h"
#include "rutil.h"

using namespace rutil;

namespace rparser
{

namespace chart
{

Chart::Chart(const MetaData *md, const TempoData *td)
	: metadata_shared_(md), tempodata_shared_(td)
{
}

Chart::Chart(const Chart &nd)
{
  Chart(&nd.GetSharedMetaData(), &nd.GetSharedTempoData());
	for (auto note : nd.notedata_)
	{
		notedata_.push_back(Note(note));
	}
	for (auto stmt : nd.stmtdata_)
	{
		stmtdata_.push_back(ConditionStatement(stmt));
	}
	for (auto action : nd.actiondata_)
	{
		actiondata_.push_back(Action(action));
	}
}

Chart::~Chart()
{
}

void Chart::swap(Chart& c)
{
	notedata_.swap(c.notedata_);
	actiondata_.swap(c.actiondata_);
	stmtdata_.swap(c.stmtdata_);
	tempodata_.swap(c.tempodata_);
	metadata_.swap(c.metadata_);
}


void Chart::Clear()
{
	notedata_.clear();
	actiondata_.clear();
	stmtdata_.clear();
  tempodata_.clear();
	metadata_.Clear();
}

void Chart::Merge(const Chart &cd, rowid_t rowFrom)
{
	notedata_.insert(notedata_.end(), cd.notedata_.begin(), cd.notedata_.end());
	actiondata_.insert(actiondata_.end(), cd.actiondata_.begin(), cd.actiondata_.end());
}

void Chart::AppendStmt(ConditionStatement& stmt)
{
	stmtdata_.push_back(stmt);
}

void Chart::EvaluateStmt(int seed)
{
	for (ConditionStatement stmt : stmtdata_)
	{
		Chart *c = stmt.EvaluateSentence(seed);
		if (c)
			Merge(*c);
	}
}

NoteData& Chart::GetNoteData()
{
	return notedata_;
}

ActionData& Chart::GetActionData()
{
	return actiondata_;
}

TempoData& Chart::GetTempoData()
{
	return tempodata_;
}

MetaData& Chart::GetMetaData()
{
	return metadata_;
}

const NoteData& Chart::GetNoteData() const
{
	return notedata_;
}

const ActionData& Chart::GetActionData() const
{
	return actiondata_;
}

const TempoData& Chart::GetTempoData() const
{
	return tempodata_;
}

const MetaData& Chart::GetMetaData() const
{
	return metadata_;
}

const TempoData& Chart::GetSharedTempoData() const
{
	return tempodata_;
}

const MetaData& Chart::GetSharedMetaData() const
{
	return metadata_;
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
		mixingnotedata_.push_back(mn);
	}
	if (deepcopy)
	{
		Note *n;
		metadata_ = metadata_alloced_ = new MetaData(metadata_);
		for (auto mn : mixingnotedata_)
		{
			n = new Note(mn.n->second);
			notedata_alloced_.push_back(n);
			mn.n = n;
		}
	}
	// TODO: fill more note data (timing based)
}

std::vector<MixingNote>& MixingData::GetNotes()
{
	return mixingnotedata_;
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
	for (auto n : notedata_alloced_)
		delete n;
	delete metadata_alloced_;
	delete timingdata_;
}

} /* namespace chart */

} /* namespace rparser */
