﻿#include <iostream>
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
  // Directory list test
  BasicDirectory dir;
  EXPECT_TRUE(dir.Open(BASE_DIR + "rutil/test/test3") == 0);
  dir.ReadAll();
  EXPECT_EQ(1, dir.size());
  // delete test
  EXPECT_TRUE(DeleteDirectory(BASE_DIR + "rutil/test"));
}

TEST(RUTIL, ARCHIVE)
{
  using namespace rutil;
#ifdef USE_ZLIB
  ArchiveDirectory dir;
  ASSERT_TRUE(dir.Open(BASE_DIR + "bms_sample_angelico.zip") == 0);
  dir.ReadAll();
  EXPECT_EQ(7, dir.size());
  const FileData* fd = dir.Get("back_980.bmp");
  ASSERT_TRUE(fd);
  EXPECT_TRUE(memcmp(fd->GetPtr(), "BM", 2) == 0);
#endif
}

TEST(RPARSER, DIRECTORY)
{
  using namespace rparser;
  Directory* d;

  d = DirectoryFactory::Open(BASE_DIR + "chart_sample");
  ASSERT_TRUE(d);
  EXPECT_TRUE(DIRECTORY_TYPE::FOLDER, d->GetDirectoryType());
  d.Close();
  d.Clear();
  delete d;
  d = nullptr;

  d = DirectoryFactory::Open(BASE_DIR + "bms_sample_angelico.zip");
  ASSERT_TRUE(d);
  EXPECT_TRUE(DIRECTORY_TYPE::FOLDER, d->GetDirectoryType());
  d.Close();
  d.Clear();
  delete d;
  d = nullptr;
}

TEST(RPARSER, TEMPODATA)
{
  // TODO
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
