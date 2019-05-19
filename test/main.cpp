#include <iostream>
#include <gtest/gtest.h>
#include "Song.h"
using namespace std;
using namespace rparser;

#define BASE_DIR std::string("../test/")

TEST(RUTIL, BASIC)
{
  using namespace rutil;
  EXPECT_STREQ(upper("abcd").c_str(), "ABCD");
  EXPECT_STREQ(lower("ABCD").c_str(), "abcd");
  EXPECT_STREQ(trim("\tABCD  \n").c_str(), "ABCD");
}

TEST(RUTIL, ENCODING)
{
  using namespace rutil;
  // encoding detecting
  std::string text_euckr = ReadFileText(BASE_DIR + "rutil/ENCODING_EUCKR.txt");
  EXPECT_EQ(E_EUC_KR, DetectEncoding(text_euckr));
  std::string text_shiftjis = ReadFileText(BASE_DIR + "rutil/ENCODING_SHIFTJIS.txt");
  EXPECT_EQ(E_SHIFT_JIS, DetectEncoding(text_shiftjis));
  std::string text_utf8 = ReadFileText(BASE_DIR + "rutil/ENCODING_UTF8.txt");
  EXPECT_EQ(E_UTF8, DetectEncoding(text_utf8));
  // encoding converting (to UTF8)
  EXPECT_STREQ(u8"가나다라あえいおう12345", text_utf8.c_str());
  EXPECT_STREQ(u8"七色ゆえんじ", ConvertEncoding(text_shiftjis, E_UTF8, E_SHIFT_JIS).c_str());
  EXPECT_STREQ(u8"가나다라마바사12345", ConvertEncoding(text_euckr, E_UTF8, E_EUC_KR).c_str());
}

TEST(RUTIL, IO)
{
  using namespace rutil;

  // init
  DeleteDirectory(BASE_DIR + "rutil/test");
  FILE *fp = 0;

  // file open, seek, read test
  // TODO: seek, read ptr returning.
  // TODO: test with jap file separator.
  fp = fopen_utf8(BASE_DIR + u8"rutil\\가나다라唖あえいお.txt", "r");
  ASSERT_TRUE(fp);
  fclose(fp);
  // create, rename, exists test
  EXPECT_TRUE(CreateDirectory(BASE_DIR + "rutil/test"));
  EXPECT_TRUE(CreateDirectory(BASE_DIR + "rutil/test/test2"));
  fp = fopen_utf8(BASE_DIR + "rutil/test/test2/a.txt", "w");
  ASSERT_TRUE(fp);
  fwrite("asdf", 1, 4, fp);
  fclose(fp);
  EXPECT_TRUE (IsDirectory(BASE_DIR + "rutil/test/test2"));
  EXPECT_FALSE(IsDirectory(BASE_DIR + "rutil/test/test2/a.txt"));
  EXPECT_FALSE(IsFile(BASE_DIR + "rutil/test/test2"));
  EXPECT_TRUE (IsFile(BASE_DIR + "rutil/test/test2/a.txt"));
  EXPECT_TRUE (Rename(BASE_DIR + "rutil/test/test2", BASE_DIR + "rutil/test/test3"));
  EXPECT_TRUE (IsDirectory(BASE_DIR + "rutil/test/test3"));
  // TODO Directory list test
  // delete test
  EXPECT_TRUE(DeleteDirectory(BASE_DIR + "rutil/test"));
}

TEST(RPARSER, DIRECTORY_FOLDER)
{
  using namespace rparser;
  const std::string fpath(BASE_DIR + "chart_sample_bms");
  DirectoryFactory &df = DirectoryFactory::Create(fpath);
  ASSERT_TRUE(df.Open());
  Directory &d = *df.GetDirectory();
  EXPECT_EQ(DIRECTORY_TYPE::FOLDER, d.GetDirectoryType());
  EXPECT_EQ(3, d.count());
  d.Clear();
}

TEST(RPARSER, DIRECTORY_ARCHIVE)
{
  using namespace rparser;
  const std::string fpath(BASE_DIR + "bms_sample_angelico.zip");
  DirectoryFactory &df = DirectoryFactory::Create(fpath);
  ASSERT_TRUE(df.Open());
  Directory &d = *df.GetDirectory();
  EXPECT_EQ(DIRECTORY_TYPE::ARCHIVE, d.GetDirectoryType());
  EXPECT_EQ(7, d.count());
  d.Close();
  EXPECT_FALSE(d.ReadAll());
  d.Clear();
}

TEST(RPARSER, DIRECTORY_BINARY)
{
  using namespace rparser;
  const std::string fpath(BASE_DIR + "chart_sample/1.vos");
  DirectoryFactory &df = DirectoryFactory::Create(fpath);
  ASSERT_TRUE(df.Open());
  Directory &d = *df.GetDirectory();
  EXPECT_EQ(DIRECTORY_TYPE::BINARY, d.GetDirectoryType());
  EXPECT_EQ(1, d.count());
  d.Close();
  d.Clear();
}

TEST(RPARSER, TEMPODATA)
{
  Chart c;
  TempoData &td = c.GetTempoData();

  // default value check
  td.SetBPMChange(120.0);
  EXPECT_EQ(120, td.GetMinBpm());
  EXPECT_EQ(120, td.GetMaxBpm());
  EXPECT_FALSE(td.HasBpmChange());
  EXPECT_FALSE(td.HasStop());
  EXPECT_FALSE(td.HasWarp());

  // add some time signature (default)
  td.SetBPMChange(120.0);
  EXPECT_EQ(60'000.0, td.GetTimeFromBeat(120.0));
  EXPECT_EQ(120.0, td.GetBeatFromTime(60'000.0));
  td.clear();

  // add some time signature
  td.SetBPMChange(180.0);
  td.SeekByBeat(40.0);
  td.SetBPMChange(90.0);
  td.SeekByBeat(48.0);
  td.SetSTOP(2000.0);
  td.SetBPMChange(180.0);
  td.SetMeasureLengthChange(2.0);   // default is 4 beat per measure
  // SeekByTime and add STOP
  // -- Excluding these line may fail test below. do it for advanced test.
  td.SeekByTime(50'000.0);
  td.SetSTOP(2000.0);
  EXPECT_EQ(90.0, td.GetMinBpm());
  EXPECT_EQ(180.0, td.GetMaxBpm());

  // add warp test
  td.SeekByBeat(1000.0);
  td.SetWarp(2.0);

  // test time
#if 0
  std::cout << "40Beat time: " << td.GetTimeFromBeat(40.0) << std::endl;
  std::cout << "47.99Beat time: " << td.GetTimeFromBeat(47.99) << std::endl;
  std::cout << "48Beat time: " << td.GetTimeFromBeat(48.0) << std::endl;
  std::cout << "48.01Beat time: " << td.GetTimeFromBeat(48.01) << std::endl;
  std::cout << "18sec beat: " << td.GetBeatFromTime(18'000.0) << std::endl;
  std::cout << "19sec beat: " << td.GetBeatFromTime(19'000.0) << std::endl;
  std::cout << "20sec beat: " << td.GetBeatFromTime(20'000.0) << std::endl;
  std::cout << "50sec beat: " << td.GetBeatFromTime(50'000.0) << std::endl;
  std::cout << "52sec beat: " << td.GetBeatFromTime(52'000.0) << std::endl;
  std::cout << "60sec beat: " << td.GetBeatFromTime(60'000.0) << std::endl;
  std::cout << "120sec beat: " << td.GetBeatFromTime(120'000.0) << std::endl;
#endif
  EXPECT_NEAR(2'000.0, td.GetTimeFromBeat(48.0) - td.GetTimeFromBeat(47.99), 10.0);
  EXPECT_NEAR(0.0, td.GetBeatFromTime(20'000.0) - td.GetBeatFromTime(19'000.0), 0.01);
  EXPECT_NEAR(0.0, td.GetBeatFromTime(52'000.0) - td.GetBeatFromTime(50'000.0), 0.01);
  EXPECT_NEAR(180.0, td.GetBeatFromTime(110'000.0 /* 60+2 sec */) - td.GetBeatFromTime(48'000.0), 0.01);
  EXPECT_NEAR(td.GetBeatFromRow(13 + 2.0/4), 48.0+3.0, 0.01);

  const double warp_time = td.GetTimeFromBeat(1000.0);
  EXPECT_NEAR(2.0, td.GetBeatFromTime(warp_time + 0.01) - td.GetBeatFromTime(warp_time), 0.01);

  td.clear();

  // now use Invalidate method
  auto &n = td.GetTempoNoteData();
  MetaData md;
  TempoNote tn;

  tn.SetBeatPos(0);
  tn.SetBpm(180);
  n.AddNote(tn);
  tn.SetBeatPos(40);
  tn.SetBpm(90);
  n.AddNote(tn);
  tn.SetBeatPos(48);
  tn.SetStop(2000);
  n.AddNote(tn);
  tn.SetBpm(180);
  n.AddNote(tn);
  tn.SetMeasure(2.0);
  n.AddNote(tn);
  tn.SetTimePos(50'000.0);
  tn.SetStop(2000);
  n.AddNote(tn);

  td.Invalidate(md);

  // do same test
  EXPECT_NEAR(2'000.0, td.GetTimeFromBeat(48.0) - td.GetTimeFromBeat(47.99), 10.0);
  EXPECT_NEAR(0.0, td.GetBeatFromTime(20'000.0) - td.GetBeatFromTime(19'000.0), 0.01);
  EXPECT_NEAR(0.0, td.GetBeatFromTime(52'000.0) - td.GetBeatFromTime(50'000.0), 0.01);
  EXPECT_NEAR(180.0, td.GetBeatFromTime(110'000.0 /* 60+2 sec */) - td.GetBeatFromTime(48'000.0), 0.01);
  EXPECT_NEAR(td.GetBeatFromRow(13 + 2.0/4), 48.0 + 3.0, 0.01);
}

TEST(RPARSER, CHART)
{
  /**
   * Chart testing without conditional segment
   */

  Chart c;
  auto &nd = c.GetNoteData();
  auto &md = c.GetMetaData();
  SoundNote n;

  n.SetBeatPos(0.0);
  n.SetAsTapNote(0, 0);
  nd.AddNote(n);
  n.SetBeatPos(0.5);
  n.SetAsTapNote(0, 1);
  nd.AddNote(n);
  n.SetBeatPos(1.0);
  n.SetAsTapNote(0, 2);
  nd.AddNote(n);
  n.SetBeatPos(1.5);
  n.SetAsTapNote(0, 3);
  nd.AddNote(n);
  n.SetBeatPos(2.0);
  n.SetAsTapNote(0, 4);
  nd.AddNote(n);

  md.bpm = 90.0;
  md.title = "ABCD";
  md.subtitle = "EFG";
  md.artist = "TEST";

  c.InvalidateTempoData();
  c.InvalidateAllNotePos();

  EXPECT_EQ(2, nd.back().GetBeatPos());
  EXPECT_NEAR(1333.33, nd.back().GetTimePos(), 0.01);
}

TEST(RPARSER, CHART_CONDITIONAL_SEGMENT)
{
  Chart c;
  c.GetMetaData().title = "Hi";
  auto &RandomStmt = *c.CreateStmt(5, true, false);
  Chart *c2 = RandomStmt.CreateSentence(2);
  c2->GetMetaData().SetAttribute("TITLE", "Hello");
  SoundNote n;
  n.SetBeatPos(0);
  n.SetAsTapNote(0, 1);
  c2->GetNoteData().AddNote(n);
  n.SetAsTapNote(0, 3);
  c2->GetNoteData().AddNote(n);
  TempoNote tn;
  tn.SetBeatPos(0);
  tn.SetBpm(90);
  c2->GetTempoData().GetTempoNoteData().AddNote(tn);
  tn.SetBeatPos(10);
  tn.SetBpm(180);
  c2->GetTempoData().GetTempoNoteData().AddNote(tn);

  rutil::Random random;
  random.SetSeed(14);
  c.EvaluateStmt(random);
  c.GetMetaData().SetMetaFromAttribute();
  c.InvalidateTempoData();

  EXPECT_STREQ("Hello", c.GetMetaData().title.c_str());
  EXPECT_EQ(2, c.GetNoteData().size());
  EXPECT_EQ(180, c.GetTempoData().GetMaxBpm());
  EXPECT_EQ(90, c.GetTempoData().GetMinBpm());
}

TEST(RPARSER, LONGNOTE)
{
  Chart c;
  //EXPECT_FALSE(c.GetNoteData().HasLongnote());

  // long note object count
  SoundNote note;
  note.SetBeatPos(0);
  note.SetAsTapNote(0, 2);
  note.SetLongnoteLength(1);
  
  c.GetNoteData().AddNote(note);
  //EXPECT_TRUE(c.GetNoteData().HasLongnote());

  // long note song length test
  //c.GetSongLength();
}

TEST(RPARSER, CHARTLIST)
{
  ChartList clist;
  Chart *c = nullptr;

  clist.AddNewChart();
  clist.AddNewChart();
  clist.AddNewChart();

  SoundNote n;
  n.SetBeatPos(1.0);

  c = clist.GetChart(0);
  c->GetTempoData().SetBPMChange(120);
  n.SetAsTapNote(0, 0);
  c->GetNoteData().AddNote(n);

  c = clist.GetChart(1);
  c->GetTempoData().SetBPMChange(121);
  n.SetAsTapNote(0, 1);
  c->GetNoteData().AddNote(n);

  c = clist.GetChart(2);
  c->GetTempoData().SetBPMChange(122);
  n.SetAsTapNote(0, 2);
  c->GetNoteData().AddNote(n);

  c = clist.GetChart(1);
  EXPECT_EQ(c->GetNoteData().back().GetLane(), 1);
  EXPECT_EQ(c->GetTempoData().GetMinBpm(), 121);
}

TEST(RPARSER, CHARTNOTELIST)
{
  ChartNoteList clist;
  Chart *c = nullptr;

  clist.AddNewChart();
  clist.AddNewChart();
  clist.AddNewChart();

  SoundNote n;
  n.SetBeatPos(1.0);

  c = clist.GetChartData(0);
  c->GetTempoData().SetBPMChange(120);
  n.SetAsTapNote(0, 0);
  c->GetNoteData().AddNote(n);
  clist.CloseChartData();

  c = clist.GetChartData(1);
  c->GetTempoData().SetBPMChange(121);
  n.SetAsTapNote(0, 1);
  c->GetNoteData().AddNote(n);
  clist.CloseChartData();

  c = clist.GetChartData(2);
  c->GetTempoData().SetBPMChange(122);
  n.SetAsTapNote(0, 2);
  c->GetNoteData().AddNote(n);
  clist.CloseChartData();

  c = clist.GetChartData(1);
  EXPECT_EQ(c->GetNoteData().back().GetLane(), 1);
  EXPECT_EQ(c->GetTempoData().GetMinBpm(), 122);
  clist.CloseChartData();
}

TEST(RPARSER, VOSFILE_V2)
{
  Song song;
  const auto songlist = {
    "chart_sample/1.vos",
    "chart_sample/103.vos"
  };
  for (auto& songpath : songlist)
  {
    EXPECT_TRUE(song.Open(BASE_DIR + songpath));
    Chart *c = song.GetChart(0);
    auto &md = c->GetMetaData();
    auto &nd = c->GetNoteData();
    auto &td = c->GetTempoData();
    song.CloseChart();
    song.Close();
  }
}

TEST(RPARSER, VOSFILE_V3)
{
  Song song;
  const auto songlist = {
    "chart_sample/23.vos",
    "chart_sample/24.vos",
    "chart_sample/109.vos"
  };
  for (auto& songpath : songlist)
  {
    EXPECT_TRUE(song.Open(BASE_DIR + songpath));
    Chart *c = song.GetChart(0);
    ASSERT_TRUE(c);
    auto &md = c->GetMetaData();
    auto &nd = c->GetNoteData();
    auto &td = c->GetTempoData();
    song.CloseChart();
    song.Close();
  }
}

TEST(RPARSER, BMSARCHIVE)
{
  Song song;
  EXPECT_TRUE(song.Open(BASE_DIR + "bms_sample_angelico.zip"));

  Chart *c = song.GetChart(0);
  ASSERT_TRUE(c);
  auto &md = c->GetMetaData();
  auto &nd = c->GetNoteData();
  auto &td = c->GetTempoData();

  // metadata, note count (including longnote) check
  md.SetMetaFromAttribute();
  ASSERT_STREQ("Angelico [Max]", md.title.c_str());
  EXPECT_EQ(1088, c->GetScoreableNoteCount());
  EXPECT_TRUE(c->HasLongnote());

  // time check
  c->InvalidateTempoData();
  c->InvalidateAllNotePos();
  std::cout << "Total time of song " << md.title.c_str() << " is: " << c->GetSongLastScorableObjectTime() << std::endl;
  EXPECT_EQ(140, c->GetTempoData().GetMaxBpm());
  EXPECT_EQ(70, c->GetTempoData().GetMinBpm());
  EXPECT_NEAR(95'000, c->GetSongLastScorableObjectTime(), 1'500);  // nearly 1m'35s

  song.CloseChart();
  song.Close();
}

TEST(RPARSER, BMS_STRESS)
{
#if 0
  Song song;
  const auto songlist = {
    "chart_sample_bms"
  };
  for (auto& songpath : songlist)
  {
    EXPECT_TRUE(song.Open(BASE_DIR + songpath));
    Chart *c = song.GetChart(0);
    ASSERT_TRUE(c);
    auto &md = c->GetMetaData();
    auto &nd = c->GetNoteData();
    auto &td = c->GetTempoData();
    song.CloseChart();
    song.Close();
  }
#endif
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
