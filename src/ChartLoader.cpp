#include "ChartLoader.h"

namespace rparser {

void ChartLoader::SetFilename(const std::string& filename)
{
    m_sFilename = filename;
}

void ChartLoader::SetSeed(int seed)
{
    m_iSeed = seed;
}

}