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
  const std::string fpath(BASE_DIR + "chart_sample");
  DirectoryFactory &df = DirectoryFactory::Create(fpath);
  ASSERT_TRUE(df.Open());
  Directory &d = *df.GetDirectory();
  EXPECT_EQ(DIRECTORY_TYPE::FOLDER, d.GetDirectoryType());
  EXPECT_EQ(8, d.count());
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
  EXPECT_EQ(120, td.GetMinBpm());
  EXPECT_EQ(120, td.GetMaxBpm());
  EXPECT_FALSE(td.HasBpmChange());
  EXPECT_FALSE(td.HasStop());
  EXPECT_FALSE(td.HasWarp());

  // add some data
}

TEST(RPARSER, NOTEDATA)
{
  // TODO : time marking from tempodata
}

TEST(RPARSER, CHARTLIST)
{
  // TODO
}

TEST(RPARSER, VOSFILE)
{
  // TODO
}

TEST(RPARSER, BMS)
{
  // TODO
}

TEST(RPARSER, BMSARCHIVE)
{
  // TODO
}

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
