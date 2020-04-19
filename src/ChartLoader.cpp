#include "ChartLoader.h"
#include "common.h"

namespace rparser {

bool ChartLoader::bOpenBmsFileWithoutProcessing = false;

bool ChartLoader::Test(const void* p, unsigned iLen) { return false; }

ChartLoader* ChartLoader::Create(Song *song)
{
  switch (song->GetSongType())
  {
  case SONGTYPE::BMS:
  {
    ChartLoaderBMS * cl = new ChartLoaderBMS(song);
    // also consider for edit mode
    cl->ProcessConditionalStatement(!bOpenBmsFileWithoutProcessing);
    return cl;
  }
  case SONGTYPE::VOS:
    return new ChartLoaderVOS(song);
  default:
    ASSERT(0);
  }

  return nullptr;
}

void ChartLoader::SetSeed(int seed)
{
  if (seed < 0) seed_ = (int)time(0);
  else seed_ = seed;
}

void ChartLoader::Preload(Chart &c, const void* p, int iLen)
{
  c.Clear();
  c.hash_ = rutil::md5_str(p, iLen);
  c.seed_ = seed_;
}

}
