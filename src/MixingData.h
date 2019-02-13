#ifndef RPARSER_MIXINGDATA_H
#define RPARSER_MIXINGDATA_H

#include "common.h"
#include "Chart.h"
#include "TimingData.h"

namespace rparser
{

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
class MixingData
{
public:
	MixingData();
	MixingData(const Chart& c, bool deepcopy = false);
	~MixingData();

	std::vector<MixingNote>& GetNotes();
	const TimingData& GetTimingdata() const;
	const MetaData& GetMetadata() const;
private:
	std::vector<MixingNote> mixingnotes_;
	TimingData* timingdata_;
	const MetaData* metadata_;

	std::vector<Note*> notes_alloced_;
	MetaData* metadata_alloced_;
};

}

#endif
