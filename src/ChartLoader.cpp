#include "ChartLoader.h"
#include <time.h>

namespace rparser {

bool ChartLoader::bOpenBmsFileWithoutProcessing = false;

bool ChartLoader::Test(const void* p, int iLen) { return false; }

ChartLoader* ChartLoader::Create(SONGTYPE songtype)
{
  switch (songtype)
  {
  case SONGTYPE::BMS:
  {
    ChartLoaderBMS * cl = new ChartLoaderBMS();
    // also consider for edit mode
    cl->ProcessConditionalStatement(!bOpenBmsFileWithoutProcessing);
    return cl;
  }
  case SONGTYPE::VOS:
    return new ChartLoaderVOS();
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

}