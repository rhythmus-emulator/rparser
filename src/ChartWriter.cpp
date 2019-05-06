#include "ChartWriter.h"

namespace rparser
{

ChartWriter::ChartWriter()
{}

bool ChartWriter::Serialize(const Chart& c)
{
  return false;
}

bool ChartWriter::SerializeCommonData(const ChartListBase& chartlist)
{
  return false;
}

rutil::FileData& ChartWriter::GetData()
{
  return data_cache_;
}

std::string ChartWriter::GetFilename()
{
  return std::string();
}

bool ChartWriter::IsWritable()
{
  return false;
}

void ChartWriter::AddData(rutil::FileData& d)
{

}

ChartWriter* CreateChartWriter(SONGTYPE songtype)
{
  return new ChartWriter();
}



}