/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_METADATA_H
#define RPARSER_METADATA_H

#define MAX_RESOURCE_COUNT 10240

namespace rparser {

/*
 * @description
 * contains metadata(header) part for song
 */
class MetaData {
    class Resource;

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
    // It returns valid Resource object pointer always (without exception)
    Resource* GetResource(const std::string& key);
    Resource* GetBMPResource() { return GetResource("BMP"); };
    Resource* GetBGAResource() { return GetResource("BGA"); };
    Resource* GetWAVResource() { return GetResource("wAV"); };
    MidiResource* GetMIDIResource() { return GetResource("MIDI"); };
    bool IsResourceExist(const std::string& key);


    
    class Resource {
    private:
        std::string info[MAX_RESOURCE_COUNT];
        std::string name;
        int nResourceCount;
    public:
        // @description is this resource contains any information?
        bool IsEmpty();
        // @description set resource data of index, and internally updates nResourceCount.
        void SetData(int index, const std::string& data);
        // @description set resource data of index to zero, and internally updates resource count.
        void DeleteData(int index);
        int GetDataCount() { return nResourceCount; };
        Resource() { nResourceCount=0; }
    };

    // TODO: should I support BGA resource type? (BM98 format)
    struct BGAResource {
    };

    struct MidiChannel {
        ww
    }

    private:
    // @description
    // internal function; remove unused Resource pool from memory 
    // automatically called when IsResourceExist(); called.
    void UpdateResources();
    std::map<std::string, std::string> m_sAttributes;

    std::string m_BMPFiles[MAX_RESOURCE_COUNT];
    std::string m_WAVFiles[MAX_RESOURCE_COUNT];
    BGAResource m_BGARes[MAX_RESOURCE_COUNT];
};


}

#endif