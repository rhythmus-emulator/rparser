#include "Song.h"
#include "MetaData.h"


// ------ class BasicDirectory ------

int rparser::ArchiveDirectory::Open(const std::string &path)
{
	// parse all files to full depth
	return 0;
}

int rparser::ArchiveDirectory::Write(const FileData &fd)
{
	return 0;
}

int rparser::ArchiveDirectory::Read(FileData &fd)
{
	return 0;
}

int rparser::ArchiveDirectory::ReadFiles(std::vector<FileData>& fd)
{
	return 0;
}

int rparser::ArchiveDirectory::Flush()
{
	// do nothing
	return 0;
}

int rparser::ArchiveDirectory::Create(const std::string& path)
{
	return 0;
}

int rparser::ArchiveDirectory::Close()
{
	return 0;
}

static int Test(const std::string &path)
{
	return 0;
}

// ------ class ArchiveDirectory ------

int rparser::ArchiveDirectory::Open(const std::string& path)
{
    Close();
	m_sPath = path;
    m_Archive = zip_open(path.c_str(), ZIP_RDONLY, &error);
    if (error)
    {
        printf("Zip Reading Error occured - code %d\nstr (%s)\n", error, zip_error_to_str(error));
        return error;
    }
    // get all file lists from zip
    // TODO: should detect encoding
    int iEntries = zip_get_num_entries(m_Archive);
    zip_stat zStat;
    for (int i=0; i<iEntries; i++)
    {
        zip_stat_index(m_Archive, i, ZIP_FL_UNCHANGED, &zStat);
        m_vFilename.push_back(zStat.name);
    }
    // read 
    return 0;
}

int rparser::ArchiveDirectory::Read(FileData &fd)
{
    ASSERT(fd.p == 0 && m_Archive);
    zip_file_t *zfp = zip_fopen(m_Archive, fd.fn.c_str(), ZIP_FL_UNCHANGED);
    if (!zfp)
    {
        printf("Failed to read file(%s) from zip\n", fd.fn.c_str());
        return -1;
    }
    zip_stat zStat;
    zip_stat(m_Archive, fd.fn.c_str(), 0, &zStat);
    fd.iLen = zStat.size;
    fd.p = (unsigned char*)malloc(fd.iLen);
    zip_fread(zfp, (void*)fd.p, fd.iLen);
    zip_fclose(zfp);
    return 0;
}

int rparser::ArchiveDirectory::Write(const FileData &fd)
{
    ASSERT(m_Archive);
    zip_source_t *s;
    s = zip_source_buffer(m_Archive, fd.p, fd.iLen, 0);
    if (!s)
    {
        printf("Zip source buffer creation failed!\n");
        return -1;
    }
    error = zip_file_add(m_Archive, fd.fn.c_str(), s, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
    zip_source_free(s);
    if (error)
    {
        printf("Zip file appending failed! (code %d)\n", error);
    }
    return 0;
}

int rparser::ArchiveDirectory::Flush()
{
    // do nothing?
    // maybe need to call Close() ...
    return 0;
}

int rparser::ArchiveDirectory::Create(const std::string& path)
{
    Close();
    m_Archive = zip_open(path.c_str(), ZIP_CREATE | ZIP_EXCL, &error);
    if (error)
    {
        printf("Zip Creating Error occured - code %d\nstr (%s)\n", error, zip_error_to_str(error));
        return error;
    }
    return 0;
}

int rparser::ArchiveDirectory::Close()
{
    if (m_Archive)
    {
        return zip_close(m_Archive);
    }
    return 0;
}

int rparser::ArchiveDirectory::Test(const std::string& path)
{
    return endsWith(path, ".zip") || endsWith(path, ".osz");
}

void rparser::ArchiveDirectorySetEncoding(const std::string& sEncoding)
{
	m_sEncoding = sEncoding;
}

// ------ class Song ------

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

bool rparser::Song::OpenDirectory()
{
	if (BasicDirectory::Test(m_sPath))
		m_pDir = new BasicDirectory();
	else if (ArchiveDirectory::Test(m_sPath))
		m_pDir = new ArchiveDirectory();
	else return false;
	ASSERT(m_pDir);
	m_Songtype = m_pDir->Open(path);
	return m_Songtype==0;
}
bool rparser::Song::Open(const std::string & path, SONGTYPE songtype)
{
	// initialize
	Close();
	m_errorcode = 0;
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
		m_sPath = getParentFolder(path);
		if (!OpenDirectory())
		{
			return false;
		}
		std::string m_sChartPath = getFilename(path);
		if (m_Songtype == SONGTYPE::UNKNOWN)
			m_Songtype = rparser::TestSongTypeExtension(m_sChartPath);
		else if (m_Songtype != rparser::TestSongTypeExtension(m_sChartPath))
		{
			Close();
			printf("Cannot detect valid song file.\n");
			m_errorcode = 4;
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
		m_errorcode = 4;
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

bool rparser::Song::LoadChart(const std::string& path)
{
	Chart *c = 0;
	switch (m_Songtype) {
	case SONGTYPE::BMS:
		r = LoadSongFromBms(path);
		break;
	case SONGTYPE::BMSON:
		r = LoadSongFromBmsOn(path);
		break;
	case SONGTYPE::VOS:
		r = LoadSongFromVos(path);
		break;
	default:
		ASSERT(m_Songtype != SONGTYPE::UNKNOWN);
	}
	if (c) m_vCharts.push_back(c);
	return c;
}

bool rparser::Song::LoadSongMetadata()
{
	ASSERT(songtype != SONGTYPE::UNKNOWN);

	// TODO
	switch (songtype) {
	case SONGTYPE::OSU:
		return false;
	}
	return true;
}

bool rparser::Song::LoadSongFromBms(const std::string & path)
{
	return false;
}
bool rparser::Song::LoadSongFromBmsOn(const std::string & path)
{
	return false;
}
bool rparser::Song::LoadSongFromOsu(const std::string & path)
{
	return false;
}
bool rparser::Song::LoadSongFromVos(const std::string & path)
{
	return false;
}

static bool ReadCharts(const std::string &path, std::vector<Chart*>& charts)
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

void rparser::Song::Close()
{
	m_IsSongDir = false;
	m_SongType = SONGTYPE::UNKNOWN;
	m_SongDir.clear();
	m_SongFilename.clear();
	m_SongBaseDir.clear();
	if (m_pDir)
	{
		delete m_pDir;
		m_pDir = 0;
	}
	for (Chart *c: m_vCharts)
	{
		delete c;
	}
	m_Charts.clear();
	m_errorcode = 0;
}

rparser::Song::Song()
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
