#include <iostream>
#include <gtest/gtest.h>
#include "Song.h"
#include "ChartUtil.h"
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

TEST(RUTIL, IO_MASK)
{
  EXPECT_TRUE(rutil::wild_match("../abc/def.txt", "../abc/*.txt"));
  EXPECT_TRUE(rutil::wild_match("../abc/def9.txt", "../abc/*e*"));
  EXPECT_TRUE(rutil::wild_match("../abc/def.txt", "../*/def.txt"));
  EXPECT_TRUE(rutil::wild_match("../abc/def.txt", "../abc/???.txt"));
  EXPECT_FALSE(rutil::wild_match("../abc/def.txt", "../abc/*23.txt"));
  EXPECT_FALSE(rutil::wild_match("../abc/def.txt", "../abc/????.txt"));
  EXPECT_FALSE(rutil::wild_match("../abc/def.txt", "../*/abc.txt"));

  std::vector<std::string> out;
  // two files expected:
  // ../test/chart_sample/9.vos
  // ../test/chart_sample_bms/L9^.bme
  rutil::GetDirectoryEntriesMasking(BASE_DIR + "chart_sample*/*9*", out);
  EXPECT_EQ(2, out.size());

  rutil::GetDirectoryEntriesMasking(BASE_DIR + "chart_sample_bms/*.bme", out);
  ASSERT_FALSE(out.empty());
  EXPECT_STREQ("../test/chart_sample_bms/L9^.bme", out[0].c_str());
}

TEST(RPARSER, DIRECTORY_FOLDER)
{
  using namespace rparser;
  const std::string fpath(BASE_DIR + "chart_sample_bms");
  Directory *d = new DirectoryFolder(fpath);
  ASSERT_TRUE(d->Open());
  EXPECT_EQ(3, d->count());
  delete d;
}

TEST(RPARSER, DIRECTORY_ARCHIVE)
{
  using namespace rparser;
  const std::string fpath(BASE_DIR + "bms_sample_angelico.zip");
  Directory *d = new DirectoryArchive(fpath);
  ASSERT_TRUE(d->Open());
  EXPECT_EQ(7, d->count());
  d->Close();
  EXPECT_FALSE(d->ReadAll());
  d->Clear();
  delete d;
}

TEST(RPARSER, DIRECTORY_MANAGER)
{
  ASSERT_TRUE(DirectoryManager::OpenDirectory(BASE_DIR + "chart_sample_bms"));
  ASSERT_TRUE(DirectoryManager::OpenDirectory(BASE_DIR + "bms_sample_angelico.zip"));

  const char* p;
  size_t len;
  EXPECT_TRUE(DirectoryManager::GetFile(BASE_DIR + "bms_sample_angelico.zip/4.lead2_fil_d_029.wav", &p, len));
  EXPECT_FALSE(DirectoryManager::GetFile(BASE_DIR + "bms_sample_angelico.zip/0.ct_[MAX].bms.bak", &p, len));
  EXPECT_TRUE(DirectoryManager::GetFile(BASE_DIR + "chart_sample_bms/allnightmokugyo.bms", &p, len));

  /* not opened yet */
  EXPECT_FALSE(DirectoryManager::GetFile(BASE_DIR + "chart_sample/1.vos", &p, len));

  DirectoryManager::CloseDirectory(BASE_DIR + "chart_sample_bms");
  DirectoryManager::CloseDirectory(BASE_DIR + "bms_sample_angelico.zip");
}

TEST(RPARSER, TIMINGDATA)
{
  Chart c;
  auto &td = c.GetTimingData();
  auto &tsd = c.GetTimingSegmentData();
  auto &md = c.GetMetaData();

  /* add timing data manually */
  md.bpm = 120.0;
  {
    TimingObject *t;
    t = new TimingObject();
    t->SetBeatPos(0.0);
    t->SetBpm(180.0);
    td.AddObject(t);

    t = new TimingObject();
    t->SetBeatPos(40.0);
    t->SetBpm(90.0);
    td.AddObject(t);

    t = new TimingObject();
    t->SetBeatPos(48.0);
    t->SetStop(2000.0);
    td.AddObject(t);

    t = new TimingObject();
    t->SetBeatPos(48.0);
    t->SetBpm(180.0);
    td.AddObject(t);

    t = new TimingObject();
    t->SetBeatPos(48.0);
    t->SetMeasure(2.0);
    td.AddObject(t);

    t = new TimingObject();
    t->SetBeatPos(54.0);
    t->SetStop(2000.0);
    td.AddObject(t);

    t = new TimingObject();
    t->SetBeatPos(1000.0);
    t->SetWarp(2.0);
    td.AddObject(t);
  }

  /* invalidation */
  c.GetTimingSegmentData().Invalidate(c.GetMetaData());

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

  /* check */
  /*
  EXPECT_EQ(120, tsd.GetMinBpm());
  EXPECT_EQ(120, tsd.GetMaxBpm());
  */
  EXPECT_EQ(90.0, tsd.GetMinBpm());
  EXPECT_EQ(180.0, tsd.GetMaxBpm());
  EXPECT_FALSE(tsd.HasBpmChange());
  EXPECT_FALSE(tsd.HasStop());
  EXPECT_FALSE(tsd.HasWarp());

  EXPECT_EQ(60'000.0, tsd.GetTimeFromBeat(120.0));
  EXPECT_EQ(120.0, tsd.GetBeatFromTime(60'000.0));

  EXPECT_NEAR(2'000.0, tsd.GetTimeFromBeat(48.0) - tsd.GetTimeFromBeat(47.99), 10.0);
  EXPECT_NEAR(0.0, tsd.GetBeatFromTime(20'000.0) - tsd.GetBeatFromTime(19'000.0), 0.01);
  EXPECT_NEAR(0.0, tsd.GetBeatFromTime(52'000.0) - tsd.GetBeatFromTime(50'000.0), 0.01);
  EXPECT_NEAR(180.0, tsd.GetBeatFromTime(110'000.0 /* 60+2 sec */) - tsd.GetBeatFromTime(48'000.0), 0.01);
  EXPECT_NEAR(tsd.GetBarFromBeat(13 + 2.0 / 4), 48.0 + 3.0, 0.01);

  const double warp_time = tsd.GetTimeFromBeat(1000.0);
  EXPECT_NEAR(2.0, tsd.GetBeatFromTime(warp_time + 0.01) - tsd.GetBeatFromTime(warp_time), 0.01);

  EXPECT_NEAR(2'000.0, tsd.GetTimeFromBeat(48.0) - tsd.GetTimeFromBeat(47.99), 10.0);
  EXPECT_NEAR(0.0, tsd.GetBeatFromTime(20'000.0) - tsd.GetBeatFromTime(19'000.0), 0.01);
  EXPECT_NEAR(0.0, tsd.GetBeatFromTime(52'000.0) - tsd.GetBeatFromTime(50'000.0), 0.01);
  EXPECT_NEAR(180.0, tsd.GetBeatFromTime(110'000.0 /* 60+2 sec */) - tsd.GetBeatFromTime(48'000.0), 0.01);
  EXPECT_NEAR(tsd.GetBarFromBeat(13 + 2.0/4), 48.0+3.0, 0.01);
}

TEST(RPARSER, CHART)
{
  /**
   * Chart testing without conditional segment
   */

  Chart c;
  auto &nd = c.GetNoteData();
  auto &md = c.GetMetaData();
  nd.set_track_count(7);

  {
    Note *n;
    n = new Note();
    n->SetBeatPos(0.0);
    n->set_track(0);
    nd.AddObject(n);

    n = new Note();
    n->SetBeatPos(0.5);
    n->set_track(1);
    nd.AddObject(n);

    n = new Note();
    n->SetBeatPos(1.0);
    n->set_track(2);
    nd.AddObject(n);

    n = new Note();
    n->SetBeatPos(1.5);
    n->set_track(3);
    nd.AddObject(n);

    n = new Note();
    n->SetBeatPos(2.0);
    n->set_track(4);
    nd.AddObject(n);
  }

  md.bpm = 90.0;
  md.title = "ABCD";
  md.subtitle = "EFG";
  md.artist = "TEST";

  c.Invalidate();

  EXPECT_EQ(5, nd.size());
  EXPECT_EQ(2, nd.back()->GetBeatPos());
  EXPECT_NEAR(1333.33, nd.back()->GetTimePos(), 0.01);
}

TEST(RPARSER, LONGNOTE)
{
  Chart c;
  Note *n;
  auto &nd = c.GetNoteData();
  nd.set_track_count(7);
  EXPECT_FALSE(c.HasLongnote());

  // long note object count
  n = new Note();
  n->SetBeatPos(.0);
  n->set_track(2);
  n->NewChain()->SetBeatPos(1.0);
  nd.AddObject(n);
  EXPECT_TRUE(c.HasLongnote());

  // TODO: long note song length test
  //c.GetSongLength();

  // long note duplication test
  n = new Note();
  n->SetBeatPos(.5);
  n->set_track(2);
  nd.AddObject(n);
  EXPECT_TRUE(nd.size() == 1);
  EXPECT_FALSE(c.HasLongnote());
}

TEST(RPARSER, CHARTLIST)
{
  Song s;
  s.SetSongType(SONGTYPE::BMS);
  Chart *c = nullptr;
  Note *n = nullptr;

  c = s.NewChart();
  c->GetMetaData().bpm = 120;
  n = new Note();
  n->set_track(0);
  c->GetNoteData().AddObject(n);
  c->Invalidate();

  c = s.NewChart();
  c->GetMetaData().bpm = 121;
  n = new Note();
  n->set_track(1);
  c->GetNoteData().AddObject(n);
  c->Invalidate();

  c = s.NewChart();
  c->GetMetaData().bpm = 122;
  n = new Note();
  n->set_track(2);
  c->GetNoteData().AddObject(n);
  c->Invalidate();

  c = s.GetChart(1);
  EXPECT_EQ(c->GetNoteData().back()->get_track(), 1);
  EXPECT_EQ(c->GetTimingSegmentData().GetMinBpm(), 121);
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
    ASSERT_TRUE(song.Open(BASE_DIR + songpath));
    Chart *c = song.GetChart(0);
    auto &md = c->GetMetaData();
    c->Invalidate();
    std::cout << "Total time of song " << md.title << " : " << c->GetSongLastObjectTime() << std::endl;
    song.Close();
  }
}

TEST(RPARSER, VOSFILE_V3)
{
  Song song;
  const auto songlist = {
    "chart_sample/23.vos",  // lastnote (LN) 2'05 = 125000 ms
    "chart_sample/24.vos",  // lastnote (LN) 1'23 = 83000ms
    "chart_sample/109.vos"  // lastnote 4'00 = 240000ms
  };
  for (auto& songpath : songlist)
  {
    ASSERT_TRUE(song.Open(BASE_DIR + songpath));
    Chart *c = song.GetChart(0);
    auto &md = c->GetMetaData();
    c->Invalidate();
    std::cout << "Total time of song " << md.title << " : " << c->GetSongLastObjectTime() << std::endl;
    song.Close();
  }
}

TEST(RPARSER, BMSARCHIVE)
{
  Song song;
  ASSERT_TRUE(song.Open(BASE_DIR + "bms_sample_angelico.zip"));

  Chart *c = song.GetChart(0);
  ASSERT_TRUE(c);
  auto &md = c->GetMetaData();
  c->Invalidate();

  // metadata, note count (including longnote) check
  ASSERT_STREQ("Angelico [Max]", md.title.c_str());
  EXPECT_EQ(1088, c->GetScoreableNoteCount());
  EXPECT_TRUE(c->HasLongnote());

  // time check
  std::cout << "Total time of song " << md.title.c_str() << " is: " << c->GetSongLastObjectTime() << std::endl;
  EXPECT_EQ(140, c->GetTimingSegmentData().GetMaxBpm());
  EXPECT_EQ(70, c->GetTimingSegmentData().GetMinBpm());
  EXPECT_NEAR(95'000, c->GetSongLastObjectTime(), 1'500);  // nearly 1m'35s

  song.Close();
}

TEST(RPARSER, BMS_SINGLE)
{
  using namespace rparser;
  const std::string fpath(BASE_DIR + "chart_sample_bms/l-for-nanasi.bms");
  Song song;

  ASSERT_TRUE(song.Open(fpath));

  Chart *c = song.GetChart();
  ASSERT_TRUE(c);

  c->Invalidate();
  std::cout << c->GetMetaData().title << std::endl;

  song.Close();
}

TEST(RPARSER, BMS_STRESS)
{
  EXPECT_EQ(255, rutil::atoi_16("FF"));

  Song song;
  Chart *c;
  EXPECT_TRUE(song.Open(BASE_DIR + "chart_sample_bms"));
  ASSERT_TRUE(song.GetChartCount() == 3);
  ASSERT_TRUE(song.GetSongType() == SONGTYPE::BMS);

  Chart *c_test_mokugyo = nullptr;
  Chart *c_test_l_nanasi = nullptr;
  Chart *c_test_l99 = nullptr;

  for (int i=0; i<song.GetChartCount(); i++)
  {
    c = song.GetChart(i);
    auto &md = c->GetMetaData();
    md.SetMetaFromAttribute();
    if (md.title == "Mokugyo AllnightMIX") c_test_mokugyo = c;
    else if (md.title == "L") c_test_l_nanasi = c;
    else if (md.title == "L9999999999999^99999999999") c_test_l99 = c;
  }


  /** TEST for Mokugyo (currently not Longlong mix due to too big size to upload ...) */
  c = c_test_mokugyo;
  ASSERT_TRUE(c);
  {
    auto &md = c->GetMetaData();
    auto &nd = c->GetNoteData();
    c->Invalidate();
    std::cout << "Total time of song " << md.title.c_str() << " is: " << c->GetSongLastObjectTime() << std::endl;
    EXPECT_STREQ("Mokugyo AllnightMIX", md.title.c_str());
    // 30566 sec = 510m = 8h 30m
    EXPECT_NEAR(3.05559e+07, c->GetSongLastObjectTime(), 10'000);
  }


  /** TEST for l-for-nanasi (ConditionalStmt) TODO */
  c = c_test_l_nanasi;
  ASSERT_TRUE(c);
  {
    auto &md = c->GetMetaData();
    auto &nd = c->GetNoteData();
    c->Invalidate();
  }


  /** TEST for L99^9 */
  c = c_test_l99;
  ASSERT_TRUE(c);
  {
    auto &md = c->GetMetaData();
    auto &nd = c->GetNoteData();
    c->Invalidate();

    // is timingobj is in order
    double m = -1;
    size_t idx = 0;
    auto iter = c->GetTimingData().GetAllTrackIterator();
    while (!iter.is_end())
    {
      ++idx;
      auto &n = *iter;
      if (m > n.beat)
      {
        std::cout << "error occured on idx " << idx << std::endl;
      }
      else m = n.beat;
      ++iter;
    }

    std::cout << "Total time of song " << md.title.c_str() << " is: " << c->GetSongLastObjectTime() << std::endl;
    EXPECT_EQ(32678, c->GetScoreableNoteCount());
    /** Comment: Temponote idx 801 is bpm 1 obj */
    /** Comment: Beat of last note is nearly 179.5 ~= 180 */
    EXPECT_NEAR(78'000, c->GetSongLastObjectTime(), 1'000);    // about 1m'18s
  }

  song.Close();
}

TEST(RPARSER, VOS_HTML_EXPORT)
{
  const auto songpath = "chart_sample/24.vos";
  Song song;
  ASSERT_TRUE(song.Open(BASE_DIR + songpath));
  Chart *c = song.GetChart(0);
  ASSERT_TRUE(c);
  c->Invalidate();

  std::string html;
  ExportToHTML(*c, html);

  FILE *fp = rutil::fopen_utf8(BASE_DIR + "out_to_html.html", "wb");
  ASSERT_TRUE(fp);

  std::string t;
  t = "<html><head>"\
    "<link rel='stylesheet' href='rhythmus.css' type='text/css'>"\
    "<script src='http://code.jquery.com/jquery-latest.min.js'></script>"\
    "<script src='rhythmus.js'></script>"\
    "</head><body>";
  fwrite(t.c_str(), 1, t.size(), fp);

  fwrite(html.c_str(), 1, html.size(), fp);

  t = "</body></html>";
  fwrite(t.c_str(), 1, t.size(), fp);

  fclose(fp);

  song.Close();
}

TEST(RPARSER, BMS_HTML_EXPORT)
{
  const auto songpath = "bms_sample_angelico.zip";
  Song song;
  ASSERT_TRUE(song.Open(BASE_DIR + songpath));
  Chart *c = song.GetChart(0);
  ASSERT_TRUE(c);
  c->Invalidate();

  std::string html;
  ExportToHTML(*c, html);

  FILE *fp = rutil::fopen_utf8(BASE_DIR + "out_to_html_bms.html", "wb");
  ASSERT_TRUE(fp);

  std::string t;
  t = "<html><head>"\
    "<link rel='stylesheet' href='rhythmus.css' type='text/css'>"\
    "<script src='http://code.jquery.com/jquery-latest.min.js'></script>"\
    "<script src='rhythmus.js'></script>"\
    "</head><body>";
  fwrite(t.c_str(), 1, t.size(), fp);

  fwrite(html.c_str(), 1, html.size(), fp);

  t = "</body></html>";
  fwrite(t.c_str(), 1, t.size(), fp);

  fclose(fp);

  song.Close();
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
  ::testing::FLAGS_gtest_catch_exceptions = 0;  // ASSERT to DEBUGGER
  return RUN_ALL_TESTS();
}
