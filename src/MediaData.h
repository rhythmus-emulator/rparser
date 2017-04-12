/*
 * @description
 * manages read-only resource (ex: BMP, WAV)
 */

enum class WAVDataType {
    WAV_8bit,
    WAV_16bit,
    WAV_24bit,
    WAV_32bit
};

struct WAVData {
    void* p;
    WAVDataType type;
};

enum class BMPDataType {
    BMP_RGB;
    BMP_ARGB;
}

struct BMPData {
    void *p;
    BMPDataType type;
};

class MediaData {
    std::map<int, WAVData> wav;
    std::map<int, BMPData> bmp;

    public:
    WAVData* GetWAV(int channel);
    BMPData* GetBMP(int channel);
    void clear();
};