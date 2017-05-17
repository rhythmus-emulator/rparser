/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_METADATA_H
#define RPARSER_METADATA_H

//#define MAX_CHANNEL_COUNT 10240

namespace rparser {

// TODO: for vos format
struct MidiEvent {
    int cmd;
    int value;
    unsigned long y;
};
struct SoundChannel {
    // filename
    std::map<int, std::string> fn;
    // midi event, included program(instrument) setting
    // should be ordered by y.
    std::vector<MidiEvent> programs;
};

// TODO: should I support BGA resource type? (BM98 format)
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
    std::map<int, BGAHeader> bga;
    std::vector<BGAEvent> bga_events;
    std::vector<BGAEvent> layer_events;
    std::vector<BGAEvent> poor_events;
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

    // @description
    // It returns valid Channel object pointer always (without exception)
    SoundChannel m_SoundChannel;
    BGAChannel m_BGAChannel;
    SoundChannel* GetSoundChannel() { return &m_SoundChannel; };
    BGAChannel* GetBGAChannel() { return &m_BGAChannel; };


    private:
    std::map<std::string, std::string> m_sAttributes;
};


}

#endif