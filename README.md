# RParser

RParser; various Rhythm game chart Parser for BMS/BMSON/OSZ,OSU/VOS/DTXMania(dtx)/SM/OJM files.

May require: zlip, zip, openssl library.

MIT License.

### Feature
- Support reading/writing to most series file format
- Support placing object without loss of resolution
- Support archived file format
- Support various edit/modifying functions(utilities).

### Usage
```cpp
using namespace rparser;
Song song;
EXPECT_TRUE(song.Open("../some/directory/bms.zip"));
for (size_t i=0; i<song.GetChartCount(); i++)
{
	Chart *c = song.GetChart(0);
	ASSERT_TRUE(c);
	auto &md = c->GetMetaData();
	auto &nd = c->GetNoteData();
	auto &td = c->GetTempoData();

	md.SetMetaFromAttribute();
	c->InvalidateTempoData();
	c->InvalidateAllNotePos();

	std::cout << "Total time of chart " << md.title << " : " << c->GetSongLastScorableObjectTime() << std::endl;
	song.CloseChart();
}
song.Close();
```

### Note
- Currently supported chart files are:
  * BMS
  * VOS
  * Archived BMS (.zip)
- Currently under-development; Writing chart file is not available now..

### Limitation
- Parsing Bms file in modification state with conditional statement including longnote may produce wrong parsing result.
  - Bms Longnote uses two object, and two object might placed between conditional statement, which is cannot be expressed in general chart data form.
  - Though it can be edited if chart loaded without processing conditional statement (Use ChartLoaderBMS directly), but it can cause badly parsed note data as written above.