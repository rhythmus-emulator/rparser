#include "Song.h"
#include "MetaData.h"
#include "ChartLoader.h"
#include "ChartWriter.h"


// ------ class Song ------

namespace rparser
{

char *SongErrorCode[] = {
    "No Error", // 0
    "No Save file format (or unsupported file format) specified",   // 1
    "The archive file isn't valid or supported one.",
    "Corrupted archive file (or might have password)",
    "This song archive file has no chart.",
    "Unsupported song file format.",    // 5
};
const char* GetSongErrorCode(int i) { return SongErrorCode[i]; }

}

void rparser::Song::RegisterChart(Chart * c)
{
	m_vCharts.push_back(c);
}

void rparser::Song::DeleteChart(const Chart * c_)
{
	for (auto p = m_vCharts.begin(); p != m_vCharts.end(); ++p) {
		if (*p == c_) {
			m_vCharts.erase(p);
			return;
		}
	}
}

bool rparser::Song::SaveSong()
{
	m_iErrorcode = 0;
	bool r = true;

    r &= SaveMetadata();
    for (auto &c: m_vCharts)
        r &= SaveChart(c);

	return r;
}

bool rparser::Song::SaveMetadata()
{
    // TODO
    return false;
}

bool rparser::Song::SaveChart(const Chart* c)
{
    ChartWriter *cWriter;
    switch (m_Songtype) {
        /*
    case SONGTYPE::BMS:
        cWriter = new ChartWriterBMS(c);
        break;
    case SONGTYPE::BMSON:
        r = SaveSongAsBmsOn(path);
        break;
    case SONGTYPE::OSU:
        r = SaveSongAsOsu(path);
        break;
        */
    default:
        m_iErrorcode = 1;
        return false;
    }
    // TODO
    return true;
}

bool rparser::Song::OpenDirectory()
{
	if (BasicDirectory::Test(m_sPath))
		m_pDir = new BasicDirectory();
	else if (ArchiveDirectory::Test(m_sPath))
		m_pDir = new ArchiveDirectory();
	else return false;
	ASSERT(m_pDir);
	int r = m_pDir->Open(m_sPath);
	return r==0;
}
bool rparser::Song::Open(const std::string & path, SONGTYPE songtype)
{
	// initialize
	Close();
	m_iErrorcode = 0;
	bool r = false;
	m_Songtype = songtype;

	// prepare IO (in case of valid song folder)
	m_sPath = path;
	if (!OpenDirectory())
	{
		// if it's not valid song file (neither archive nor folder)
		// then just attempt to open with parent, then load single chart.
		// if parent is also invalid song file, then return error.
		printf("Song path open failed, attempting single chart loading ...\n");
		m_sPath = GetParentDirectory(path);
		if (!OpenDirectory())
		{
			return false;
		}
		std::string m_sChartPath = GetFilename(path);
		if (m_Songtype == SONGTYPE::UNKNOWN)
			m_Songtype = rparser::TestSongTypeExtension(m_sChartPath);
		else if (m_Songtype != rparser::TestSongTypeExtension(m_sChartPath))
		{
			Close();
			printf("Cannot detect valid song file.\n");
            m_iErrorcode = 4;
			return false;
		}
		return LoadChart(path);
	}

	// if folder successfully opened, check songtype & valid
	std::vector<std::string> m_vChartPaths;
	for (std::string &fn: m_pDir->GetFileEntries())
	{
		if (m_Songtype == SONGTYPE::UNKNOWN)
		{
			m_Songtype = rparser::TestSongTypeExtension(fn);
			m_vChartPaths.push_back(fn);
		}
		else if (m_Songtype == rparser::TestSongTypeExtension(fn))
		{
			m_vChartPaths.push_back(fn);
		}
	}
	if (m_Songtype == SONGTYPE::UNKNOWN)
	{
		Close();
		printf("No valid chart file found.\n");
        m_iErrorcode = 4;
		return false;
	}

	// load charts
	for (std::string &fn: m_vChartPaths)
	{
		LoadChart(fn);
	}
	// load song metadata (CHECK RESULT?)
	LoadSongMetadata();

	return true;
}

bool rparser::Song::LoadSongMetadata()
{
    ASSERT(m_Songtype != SONGTYPE::UNKNOWN);

    // TODO
    switch (m_Songtype) {
    case SONGTYPE::OSU:
        return false;
    }
    return true;
}

bool rparser::Song::LoadChart(const std::string& path)
{
    ASSERT(m_pDir);
    ChartLoader* cLoader;
	Chart *c = new Chart();
	switch (m_Songtype) {
	case SONGTYPE::BMS:
        cLoader = new ChartLoaderBMS(c);
		break;
    /*
	case SONGTYPE::BMSON:
		LoadSongFromBmsOn(path);
		break;
	case SONGTYPE::VOS:
		LoadSongFromVos(path);
		break;
    */
	default:
		delete c;
		ASSERT(m_Songtype != SONGTYPE::UNKNOWN);
	}

    FileData fDat;
    if (m_pDir->Read(path, fDat) && cLoader->Load(fDat.p, fDat.iLen))
        m_vCharts.push_back(c);
    delete cLoader;
    DeleteFileData(fDat);
	return c == 0;
}

bool rparser::Song::ReadCharts(const std::string &path, std::vector<Chart*>& charts)
{
	Song* s = new Song();
	if (!s->Open(path))
	{
		delete s;
		return false;
	}
	// copy pointers and clear original pt container
	// to prevent object deletion
	charts = s->m_vCharts;
	s->m_vCharts.clear();
	s->Close();
	delete s;
	return true;
}

void rparser::Song::GetCharts(std::vector<Chart*>& charts)
{
	charts = m_vCharts;
}

int rparser::Song::GetChartCount()
{
	return m_vCharts.size();
}

int rparser::Song::GetError()
{
	return m_iErrorcode;
}

const char * rparser::Song::GetErrorStr()
{
	return SongErrorCode[m_iErrorcode];
}

std::string rparser::Song::toString()
{
	std::stringstream ss;
	ss << "Song path: " << m_sPath << std::endl;
	ss << "Is Archive?: " << m_bIsArchive << std::endl;
	ss << "Song type: " << GetSongTypeExtension(m_Songtype) << std::endl;
	ss << "Chart count: " << m_vCharts.size() << std::endl << std::endl;
	for (auto c : m_vCharts)
	{
		ss << c->toString() << std::endl << std::endl;
	}
	return ss.str();
}

void rparser::Song::Close()
{
    m_Songtype = SONGTYPE::UNKNOWN;
	if (m_pDir)
	{
		delete m_pDir;
		m_pDir = 0;
	}
	for (Chart *c: m_vCharts)
	{
		delete c;
	}
	m_vCharts.clear();
    m_iErrorcode = 0;
}

rparser::Song::Song()
    : m_Songtype(SONGTYPE::UNKNOWN), m_pDir(0), m_iErrorcode(0)
{
}

rparser::Song::~Song()
{
	Close();
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

std::string rparser::GetSongTypeExtension(SONGTYPE iType)
{
    switch (iType)
    {
    case SONGTYPE::BMS:
        return "bms";
    case SONGTYPE::BMSON:
        return "bmson";
    case SONGTYPE::VOS:
        return "vos";
    case SONGTYPE::OSU:
        return "osu";
    case SONGTYPE::SM:
        return "sm";
    case SONGTYPE::OJM:
        return "ojm";
    default:
        return "";
    }
}

