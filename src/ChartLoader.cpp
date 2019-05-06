#include "ChartLoader.h"

namespace rparser {

bool ChartLoader::Test(const void* p, int iLen) { return false; }

ChartLoader* ChartLoader::Create(SONGTYPE songtype)
{
  switch (songtype)
  {
  case SONGTYPE::BMS:
    return new ChartLoaderBMS();
  case SONGTYPE::VOS:
    return new ChartLoaderVOS();
  default:
    ASSERT(0);
  }

  return nullptr;
}

}