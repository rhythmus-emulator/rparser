/*
 * by @lazykuna, MIT License.
 *
 * in this file, we don't care about row position or similar thing.
 * (All placeable objects are located in NoteData.h)
 */

#ifndef RPARSER_METADATA_H
#define RPARSER_METADATA_H

//#define MAX_CHANNEL_COUNT 10240

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
struct BGAEvent {
    unsigned long id;
    unsigned long y;
};
struct BGAChannel {
    // channelno, bgainfo(filename)
    std::map<int, BGAHeader> bga;
    std::vector<BGAEvent> bga_events;
    std::vector<BGAEvent> layer_events;
    std::vector<BGAEvent> poor_events;
};


// @description depreciated, only for Bms file type.
struct BPMChannel {
    std::map<int, int> bpm;
};


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
    void SetAttribute(const std::string& key, float value);
    void SetAttribute(const std::string& key, double value);
    bool IsAttributeExist(const std::string& key);
    bool SetEncoding(const std::string& from, const std::string& to);

    // @description
    // It returns valid Channel object pointer always (without exception)
    SoundChannel* GetSoundChannel() { return &m_SoundChannel; };
    BGAChannel* GetBGAChannel() { return &m_BGAChannel; };
    BPMChannel* GetBPMChannel() { return &m_BPMChannel; };

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
    std::string sLNObj;             // used for BMS
    std::string sBackImage;         // BG during playing
    std::string sStageImage;        // loading image
    std::string sBannerImage;
    std::string sPreviewMusic;
    std::string sMusic;             // BGM
    std::string sLyrics;
    std::string sExpand;            // Expand command (like BMS #if~#endif)

    // barlength & notecount is located at TimingData / NoteData.

    private:
    SoundChannel m_SoundChannel;
    BGAChannel m_BGAChannel;
    BPMChannel m_BPMChannel;
    // other metadata for some other purpose
    std::map<std::string, std::string> m_sAttributes;
};


}

#endif