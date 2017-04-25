#include "Song.h"

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
	switch (m_SongType) {
	case SONGTYPE::BMS:
		return SaveSongAsBms(path);
		break;
	case SONGTYPE::BMSON:
		return SaveSongAsBmsOn(path);
		break;
	case SONGTYPE::OSU:
		return SaveSongAsOsu(path);
		break;
	default:
		m_errorcode = 1;
		return false;
	}
	m_errorcode = 0;
	return true;
}

bool rparser::Song::LoadSong(const std::string & path, SONGTYPE songtype)
{
	// clear song before load
	Clear();

	// first check out if it's Archive or Directory
	if (IsDirectory(path))
		m_IsSongDir = true;
	else
		m_IsSongDir = false;

	// if it's archive, attempt to open it
	// if it's invalid: errorcode 2
	// if it's has password: errorcode 3
	// attempt to find base directory

	// TODO

	return true;
	return false;
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
}

rparser::Song::~Song()
{
	Clear();
}
