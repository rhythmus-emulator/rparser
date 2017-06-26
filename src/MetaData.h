/*
 * by @lazykuna, MIT License.
 *
 * in this file, we don't care about row position or similar thing.
 * (All placeable objects are located in NoteData.h)
 */

#ifndef RPARSER_METADATA_H
#define RPARSER_METADATA_H

//#define MAX_CHANNEL_COUNT 10240
#include <map>

namespace rparser {


struct SoundChannel {
    // channelno, filename
    std::map<int, std::string> fn;
};


struct BGAHeader {
    std::string fn;
    int sx,sy,sw,sh;
    int dx,dy,dw,dh;
};
struct BGAChannel {
    // channelno, bgainfo(filename)
    std::map<int, BGAHeader> bga;
};


// @description depreciated, only for Bms file type.
struct BPMChannel {
	std::map<int, int> bpm;
	bool GetBpm(int channel, float &out) const;
};
struct STOPChannel {
	std::map<int, int> stop;	// #STOP command (key, value)
	std::map<float, int> STP;	// #STP command
	bool GetStop(int channel, float &out) const;
};

class ChartSummaryData;

/*
 * @description
 * contains metadata(header) part for song
 */
class MetaData {
public:
    // @description supports int, string, float, double types.
    template<typename T>
    T GetAttribute(const std::string& key) const;
    template<typename T>
    T GetAttribute(const std::string& key, T fallback) const;

    void SetAttribute(const std::string& key, int value);
    void SetAttribute(const std::string& key, const std::string& value);
    void SetAttribute(const std::string& key, double value);
    bool IsAttributeExist(const std::string& key);
    bool SetEncoding(int from_codepage, int to_codepage);

    // @description
    // It returns valid Channel object pointer always (without exception)
	SoundChannel* GetSoundChannel() { return &m_SoundChannel; };
	BGAChannel* GetBGAChannel() { return &m_BGAChannel; };
	BPMChannel* GetBPMChannel() { return &m_BPMChannel; };
	STOPChannel* GetSTOPChannel() { return &m_STOPChannel; };
	const SoundChannel* GetSoundChannel() const { return &m_SoundChannel; };
	const BGAChannel* GetBGAChannel() const { return &m_BGAChannel; };
	const BPMChannel* GetBPMChannel() const { return &m_BPMChannel; };
	const STOPChannel* GetSTOPChannel() const { return &m_STOPChannel; };

    // @description
    // general attributes, mainly used in playing
    std::string sTitle;
    std::string sSubTitle;
    std::string sArtist;
    std::string sSubArtist;
    std::string sGenre;
    std::string sChartName;         // NORMAL, HYPER, ANOTHER, ONI, ...
    int iPlayer;                    // player type (TODO: comment)
    int iLNType;                    // used for BMS
    unsigned long iDifficulty;      // Chart Difficulty (Used on BMS, etc...)
    unsigned long iLevel;           // level of the song
    double fBPM;                    // basic BPM (need to be filled automatically if not exists)
    double fJudge;                  // judge difficulty
    double fTotal;                  // guage total
    int iLNObj;                     // used for BMS
    std::string sBackImage;         // BG during playing
    std::string sStageImage;        // loading image
    std::string sBannerImage;
    std::string sPreviewMusic;
    std::string sMusic;             // BGM
    std::string sLyrics;
    std::string sExpand;            // Expand command (like BMS #if~#endif)

    // barlength & notecount is located at TimingData / NoteData.
    void FillSummaryData(ChartSummaryData &csd) const;
    std::string toString();
private:
    SoundChannel m_SoundChannel;
    BGAChannel m_BGAChannel;
	BPMChannel m_BPMChannel;
	STOPChannel m_STOPChannel;
    // other metadata for some other purpose
    std::map<std::string, std::string> m_sAttributes;
};

}

#endif