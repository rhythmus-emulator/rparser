#include <stdio.h>
#include <Song.h>

#define DEST_FOLDER "D:/BMS_TEST"

int main()
{
	printf("Hello world!\nDestination folder for test: %s\n", DEST_FOLDER);
	
	rparser::DirFileList dList;
	rparser::GetDirectoryFiles(DEST_FOLDER, dList, 0);

	printf("Found target files: %d\n", dList.size());
	for (auto d : dList)
	{
		printf("- %s\n", d.first.c_str());
		std::string songpath = rparser::GetPathJoin(DEST_FOLDER, d.first);
		rparser::Song song;
		song.Open(songpath);
		printf(song.toString().c_str());
		song.Close();
	}

	return 0;
}