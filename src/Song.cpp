#include "Song.h"
#include "MetaData.h"

void rparser::Song::RegisterChart(Chart * c)
{
	m_Charts.push_back(c);
}

void rparser::Song::DeleteChart(const Chart * c_)
{
	for (auto p = m_Charts.begin(); p != m_Charts.end(); ++p) {
		if (*p == c_) {
			m_Charts.erase(p);
			return;
		}
	}
}

bool rparser::Song::SaveSong(const std::string & path, SONGTYPE songtype)
{
	m_errorcode = 0;
	bool r = false;

	switch (m_SongType) {
	case SONGTYPE::BMS:
		r = SaveSongAsBms(path);
		break;
	case SONGTYPE::BMSON:
		r = SaveSongAsBmsOn(path);
		break;
	case SONGTYPE::OSU:
		r = SaveSongAsOsu(path);
		break;
	default:
		m_errorcode = 1;
	}

	return r;
}

bool rparser::Song::SaveSongAsBms(const std::string & path)
{
	return false;
}

bool rparser::Song::SaveSongAsBmsOn(const std::string & path)
{
	return false;
}

bool rparser::Song::SaveSongAsOsu(const std::string & path)
{
	return false;
}

bool rparser::Song::LoadSong(const std::string & path, SONGTYPE songtype)
{
	// clear song before load
	Clear();

	m_errorcode = 0;
	bool r = false;

	// use proper method from songtype
	switch (songtype) {
	case SONGTYPE::BMS:
		r = LoadSongFromBms(path);
		break;
	case SONGTYPE::BMSON:
		r = LoadSongFromBms(path);
		break;
	case SONGTYPE::VOS:
		r = LoadSongFromBms(path);
		break;
	case SONGTYPE::UNKNOWN:
		// first try to detect file format automatically
		// then call recursively
		m_SongType = TestSongType(path);
		if (m_SongType == SONGTYPE::UNKNOWN) {
			m_errorcode = 5;
			break;
		}
		r = LoadSong(path, m_SongType);
	default:
		m_errorcode = 5;
	}
	return r;
}

bool rparser::Song::LoadFromArchive(const std::string & path)
{
	// only in case of not directory
	if (m_IsSongDir) {
		m_errorcode = 2;
		return false;
	}

	// if it's invalid: errorcode 2
	int r;
	m_archive = archive_read_new();
	archive_read_support_format_zip(m_archive);
	r = archive_read_open_filename(m_archive, path.c_str(), 10240); // Note 1
	if (r != ARCHIVE_OK) {
		m_errorcode = 2;
		ClearArchiveRead();
		return false;
	}

	// read all archive entries (so get filename)
	// and attempt to find base directory
	// and attempt to find type of song object.
	// TODO: Decode Shift_jis in case of UTF8 decoding failed.
	struct archive_entry* entry;
	int chartcount = 0;
	for (;;) {
		r = archive_read_next_header(m_archive, &entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r != ARCHIVE_OK) {
			// makes error code and break, if any one is bad.
			m_errorcode = 3;
			ClearArchiveRead();
			return false;
		}
		else {
			// find extension
			const char* fname = archive_entry_gname_utf8(entry);
			SONGTYPE songtype = DetectSongTypeExtension(fname);
			if (songtype != SONGTYPE::UNKNOWN) {
				m_SongBaseDir = GetDirectoryname(fname);
				m_SongType = songtype;

				// start to read chart data, based on songtype
				switch (m_SongType) {
				case SONGTYPE::BMS:
					// TODO
					break;
				default:

				}
			} else {
				archive_read_data_skip(m_archive);
			}
		}
	}
		
	// if it has no bms file, then it will have no chart.
	// So we cannot decide this song file's format. reject.
	if (chartcount <= 0) {
		m_errorcode = 4;
		ClearArchiveRead();
		return false;
	}

	// read finished, so close archive once.
	ClearArchiveRead();
}

bool rparser::Song::LoadSongFromBms(const std::string & path)
{
	m_errorcode = 0;
	m_SongType = SONGTYPE::BMS;

	// this format can be archive file
	// if so, attempt to read as archive file
	if (IsDirectory(path)) {
		m_IsSongDir = true;
	}
	else {
		m_IsSongDir = false;
		LoadFromArchive(path);
	}


	// FIN

	return false;
}

bool rparser::Song::LoadSongFromBmsOn(const std::string & path)
{
	m_errorcode = 0;
	m_SongType = SONGTYPE::BMSON;

	// this format can be archive file
	// if so, attempt to read as archive file
	if (IsDirectory(path)) {
		m_IsSongDir = true;
	}
	else {
		m_IsSongDir = false;
		LoadFromArchive(path);
	}

	return false;
}

bool rparser::Song::LoadSongFromOsu(const std::string & path)
{
	m_errorcode = 0;
	m_SongType = SONGTYPE::OSU;

	// this format can be archive file
	// if so, attempt to read as archive file
	if (IsDirectory(path)) {
		m_IsSongDir = true;
	}
	else {
		m_IsSongDir = false;
		LoadFromArchive(path);
	}

	return false;
}

bool rparser::Song::LoadSongFromVos(const std::string & path)
{
	m_errorcode = 0;
	m_SongType = SONGTYPE::VOS;

	// TODO directly read file

	return false;
}

bool rparser::Song::LoadChart(const std::string & path, SONGTYPE songtype)
{
	bool r = false;
	switch (songtype) {
	case SONGTYPE::BMS:
		break;
	default:
		// do recursive work
		songtype = DetectSongTypeExtension(path);
		if (songtype == SONGTYPE::UNKNOWN) {
			m_errorcode = 5;
			break;
		}
		r = LoadChart(path, songtype);
	}
	return r;
}

void rparser::Song::GetCharts(std::vector<Chart*>& charts)
{
	charts = m_Charts;
}

int rparser::Song::GetChartCount()
{
	return m_Charts.size();
}

int rparser::Song::GetError()
{
	return m_errorcode;
}

const char * rparser::Song::GetErrorStr()
{
	return SongErrorCode[m_errorcode];
}

char * rparser::Song::ReadResourceFile(const std::string & rel_path)
{
	return nullptr;
}

void rparser::Song::Clear()
{
	m_IsSongDir = false;
	m_SongType = SONGTYPE::UNKNOWN;
	m_SongDir.clear();
	m_SongFilename.clear();
	m_SongBaseDir.clear();
	// TODO: clear zlib handle
	for (auto p = m_Charts.begin(); p != m_Charts.end(); ++p) {
		delete *p;
	}
	m_Charts.clear();
	m_errorcode = 0;
	// archive must be in cleared state!
	assert(m_archive == 0);
}

rparser::Song::~Song()
{
	Clear();
}

bool rparser::Song::PrepareArchiveRead(const std::string& path)
{
	// If it's extension is .osz, then we don't need to open it.
	// because file format is already decided.
	std::string song_ext = GetExtension(path);
	lower(song_ext);
	if (song_ext == "osz") {
		m_SongType = SONGTYPE::OSU;
		return true;
	}

	return true;
}

void rparser::Song::ClearArchiveRead()
{
	if (m_archive) {
		// clear handle
		//archive_read_finish(m_archive);
		archive_read_free(m_archive);
		m_archive = 0;
	}
}

rparser::SONGTYPE rparser::TestSongTypeExtension(const std::string & fname)
{
	std::string ext = GetExtension(fname);
	lower(ext);
	const char* ext_c = ext.c_str();
	if (strcmp(ext_c, "bms") == 0 ||
		strcmp(ext_c, "bme") == 0 ||
		strcmp(ext_c, "bml") == 0)
		return SONGTYPE::BMS;
	else if (strcmp(ext_c, "bmson") == 0)
		return SONGTYPE::BMSON;
	else if (strcmp(ext_c, "vos") == 0)
		return SONGTYPE::VOS;
	else if (strcmp(ext_c, "osu") == 0)
		return SONGTYPE::OSU;
	else if (strcmp(ext_c, "sm") == 0)
		return SONGTYPE::SM;
	else if (strcmp(ext_c, "ojm") == 0)
		return SONGTYPE::OJM;
	return SONGTYPE::UNKNOWN;
}

rparser::SONGTYPE rparser::TestSongType(const std::string& path)
{
	// TODO: in case of zip file, open it and test them all
	return TestSongTypeExtension(path);
}