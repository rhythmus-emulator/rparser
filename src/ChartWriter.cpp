#include "ChartWriter.h"
#include "Song.h"

namespace rparser
{

ChartWriter::ChartWriter()
{}

bool ChartWriter::Write(Song* song)
{
  if (!WriteMeta(song)) return false;
  Chart *c;
  for (unsigned i = 0; i < song->GetChartCount(); ++i)
  {
    c = song->GetChart(i);
    bool r = WriteChart(c);
    if (!r) return false;
  }
  return true;
}

bool ChartWriter::WriteMeta(const Song* song)
{
  return false;
}

bool ChartWriter::WriteChart(const Chart* chart)
{
  // TODO: update hash value of the chart.
  return false;
}

std::string ChartWriter::GetFilename()
{
  return std::string();
}

void ChartWriter::AddData(rutil::FileData& d)
{

}

ChartWriter* CreateChartWriter(SONGTYPE songtype)
{
  return new ChartWriter();
}



}