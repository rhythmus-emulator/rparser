/*
 * by @lazykuna, MIT License.
 */

#include "NoteData.h"
#include "MetaData.h"
#include "TimingData.h"

namespace rparser {

/*
 * @description
 * chart data contains: notedata, timingdata, metadata
 */
class Chart {
public:

    const MetaData* GetMetaData() const;
    const NoteData* GetNoteData() const;
    const TimingData* GetTimingData() const;
    MetaData* GetMetaData();
    NoteData* GetNoteData();
    TimingData* GetTimingData();
    void SetFilePath(const std::string& fPath);
    std::string GetFilePath() const;

    // @description copy from other chart data
    void copy(const Chart& chart);

    void ReadFromFile(const std::string& path);
    void Read(const char* p, int iLen);
private:
    std::string sFilePath;
    MetaData m_Metadata;
    NoteData m_Notedata;
    TimingData m_Timingdata;
};




/*
 * @description
 * only contains brief chart metadata, which may be useful for visualizing status.
 */
struct ChartMetaData {
    std::string sFilePath;

    int iNoteCount;
    float fLastNoteTime;

    // @description basc BPM of the song
    int iBPM;
    // @description is bpm changes during song playing?
    bool isBPMChanges;
    int iMaxBPM;
    int iMinBPM;
    // @description is bomb object exists?
    bool isBomb;
    // @description is wrap object(negative bpm) exists?
    bool isWrap;
    bool isSTOP;
    // @description is backspin scratch exists?
    bool isBSS;
    bool isCharge;
    bool isHellCharge;
    // @description is command exists/processed? (in case of BMS)
    bool isCommand;

    std::string sTitle;
    std::string sArtist;
    std::string sGenre;
    std::string sBannerFile;
    std::string sBackgroundFile;
    std::string sStageFile;
    std::string sMusicFile;
    std::string sPreviewFile;
    
    void Fill(const Chart& chart);
};


}