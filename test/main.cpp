#include <iostream>
#include "Song.h"
using namespace std;
using namespace rparser;

int main()
{
	Resource *res;
	Song *song;

	cout << "Starting test for rparser library ..." << endl;

	cout << "Test 1: Loading bms directory" << endl;
	res = new Resource();
	res->Open("./test/bms_sample");
	delete res;

	cout << "Test 2: Loading bms archive" << endl;
#ifdef USE_ZLIB
#else
	cout << "Test skipped due to ZLIB library not found." << endl;
#endif
	
#if 0
	cout << "Test 3: Opening bms directory" << endl;

	song = new Song();
	song->Open("./test/bms_sample");
	delete song;

	cout << "Test 4: Opening bms archive file" << endl;
#ifdef USE_ZLIB
#else
	cout << "Test skipped due to ZLIB library not found." << endl;
#endif

	cout << "Test 3: Opening bms charts" << endl;
	cout << "File refered from: https://hitkey.nekokan.dyndns.info/bmsbench.shtml" << endl;
	auto dest_filepaths = {
		"./test/chart_sample/allnightmokugyo.bme",
		"./test/chart_sample/l-for-nanasi.bms",
		"./test/chart_sample/L9^.bme"
	};

	for (auto d : dest_filepaths)
	{
		// TODO
	}
#endif

	cout << "Test done." << endl;

	return 0;
}