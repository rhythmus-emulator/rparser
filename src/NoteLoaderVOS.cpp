/*
 * VOS has quite weird format; it contains MIDI file,
 * and it just make user available to play, following MIDI's beat.
 * So, All tempo change/beat/instrument information is contained at MIDI file.
 * VOS only contains key & timing data; NO BEAT DATA.
 * That means we need to generate beat data from time data,
 * So we have to crawl BPM(Tempo) data from MIDI. That may be a nasty thing. 
 */

#include "NoteLoaderVOS.h"
#include "util.h"
#include <stdlib.h>
#define MAX_READ_SIZE 512000
 
namespace rparser {

namespace NoteLoaderVOS {

// structure used for parsing data
class ParseArg {
    bool autorelease;
public:
    int iLen;
    const char *p_org;
    const char *p;
    ParseArg(const char *p, int iLen, bool autorelease=true)
    : p(p), p_org(p), iLen(iLen), autorelease(autorelease) {}
    ~ParseArg() { if (autorelease) free(p); }
    void Reset() { p=p_org; }
}

struct VOSHeader {
    int len;
    char desc[256];
}

struct VOSNoteDataV2 {
    unsigned char dummy;
    unsigned int time;
    unsigned char pitch;
    unsigned char dummy2;   // always 00; WAVID?
    unsigned char volume;
    unsigned char isplayable; // Note or BGM?
    unsigned char issoundable;  // soundable; always 1?
    unsigned char islongnote;
    unsigned int duration;
    unsigned char dummy3; // 00|FF, note or BGM?
}

struct VOSNoteDataV3 {
    unsigned int time;
    unsigned short duration;
    short dummy;
    unsigned char fan;
    unsigned char pitch;
    unsigned char volume;
    unsigned char channel; // playable note?
    unsigned char dummy2;
}

char* MIDISIG_BYTE[] = [
    [0xFF, 0x21],
    [0xFF, 0x03],
    [0xFF, 0x2F]
]

enum MIDISIG = [
    MIDISIG_DUMMY, // output port
    MIDISIG_TRACKNAME,
    MIDISIG_END,
    MIDISIG_DATA // includes all unknown other signatures
];

#define MIDISIG_TYPES 3
#define MIDISIG_LENGTH 3

int ReadMSInt( const char **p );
int GetMidiSignature(const char **_p, int *delta) {
    const char *p = *_p;
    *delta = ReadMSInt(&p);
    int r = MIDISIG_DATA;
    for (int i=0; i<MIDISIG_TYPES; i++) {
        if (memcmp(p, MIDISIG_BYTE[i], MIDISIG_LENGTH) == 0) {
            r = i;
            break;
        }
    }
    p += MIDISIG_LENGTH;
    *_p = p;
    return r;
}


bool LoadChart( const std::string& fpath, Chart& chart ) {
    FILE *fp = fopen_utf8(fpath.c_str(), "rb");
    if (!fp) {
        ErrorString = "Failed to open file. (VOS)";
        return false;
    }
    // don't return from now on, as we allocated memory.
    // we have to clean up.
    bool success = true;
    char *data = (char*)malloc(MAX_READ_SIZE);
    int iLen = fread(data, 1, MAX_READ_SIZE, fp);
    fclose(fp);
    if (iLen == MAX_READ_SIZE) {
        ErrorString = "File is too big, maybe truncated. (VOS)";
        success = false;
    }

    // load data
    success = (success && ParseChart(data, iLen, chart));

    // clean up
    free(data);
    return success;
}

bool ParseChart( const char* p, int iLen, Chart& chart ) {
    MetaData *md;
    NoteData *nd;
    TimingData *td;
    md = chart.GetMetaData();
    nd = chart.GetNoteData();
    td = chart.GetTimingData();

    if (!ParseMetaData(p, iLen, md)) {
        //ErrorString = "Failed to parse MetaData during reading vos file.";
        return false;
    }
    if (!ParseNoteData(p, iLen, nd)) {
        ErrorString = "Failed to parse NoteData during reading vos file.";
        return false;
    }
    if (!ParseTimingData(p, iLen, td)) {
        ErrorString = "Failed to parse TimingData during reading vos file.";
        return false;
    }
    return true;
}

// redeclaration
bool ParseMetaDataV2( const char* p, int iLen, MetaData& md );
bool ParseMetaDataV3( const char* p, int iLen, MetaData& md );
bool ParseNoteDataV2( const char* p, int iLen, NoteData& nd );
bool ParseNoteDataV3( const char* p, int iLen, NoteData& nd );
int DetectVersion( const char *p ) { return (int*)p; }

bool ParseNoteData( const char* p, int iLen, NoteData& nd ) {
    int version = DetectVersion(p);
    if (version == 2) return ParseNoteDataV2(p, iLen, nd);
    else if (version == 3) return ParseNoteDataV3(p, iLen, nd);
    else {
        ErrorString = "Unknown version";
        return false;
    }
}

bool LoadMetaData( const std::string& fpath, MetaData& md ) {
    return 0;
}

bool ParseMetaData( const char* p, int iLen, MetaData& md ) {
    int version = DetectVersion(p);
    if (version == 2) return ParseMetaDataV2(p, iLen, md);
    else if (version == 3) return ParseMetaDataV3(p, iLen, md);
    else return false;
}

// parse MIDI data to get timing data
bool ParseTimingData( const char* p, int iLen, TimingData& td ) {
    const char* p_end = p+iLen;
    // MUST start from MThd signature
    if (memcmp(p, "MThd", 4) != 0) {
        ErrorString = "Invalid MIDI start signature";
        return false;
    }
    p+=4;
    p+=6; // len: 00 00 00 06
    short format = *(short*)p; p+=2; // always 1
    ASSERT(format == 1);
    short trackcount = *(short*)p; p+=2;
    short timedivision = *(short*)p; p+=2;

    int trackidx = 0;
    while (p<p_end && memcmp(p, "Mtrk", 4) == 0) {
        p += 4;
        int tracksize = *(int*)p;
        const char *p_track_end = p+tracksize;

        int midisig = 0;
        int delta = 0;
        int ticks = 0;  // 480 tick: 1 beat
        while (midisig = GetMidiSignature(p, &delta) != MIDISIG_END) {
            ticks += delta;
            if (midisig == MIDISIG_DATA) {
                // TODO: register midi signature with data in timeloader datas.
            }
        }
        p++; // process 0x00 of MIDISIG_END
        ASSERT(p < p_track_end);
        trackidx ++;
    }

    return true;
}

// V2
bool ParseMetaDataV2( const char* p, int iLen, MetaData& md )
{
    // 4 byte: version 02
    p += 4;
    // 4 byte: filename length
    int fnamelen = (int*)p;
    p += 4;
    p += fnamelen;
    // 4 byte: VOS track file length
    int vosflen = (int*)p;
    p += 4;
    // parse metadatas [title, songartist, chartmaker, genre, unknown]
    // metadata consisted with frames
    // frame: length(short) + body
    int i = 0;
    char meta_attrs[][32] = ["TITLE", "ARTIST", "SUBARTIST", "GENRE"]
    while (i < 5) {
        short len = (short*)p;
        p += 2;
        if (len == 0) break;
        char body[256];
        memset(body, 0, sizeof(body));
        memcpy(body, p, len);
        p += len;
        md.SetAttribute(meta_attrs[i], body);
        i += 1;
    }
    // parse second metadatas (26bytes)
    char meta_binary[26];
    memcpy(meta_binary, p, 26);
    // TODO
    p += 26;
    // if flag is VOS009, then seek 1013 bytes
    // else, seek 1017 bytes
    if (memcmp("VOS009", p, 6) == 0) {
        p += 1013;
    } else {
        p += 1017;
    }

    //
    // NoteData
    //
    int cnt_inst = *(int*)p;     p += 4;
    int cnt_chart = *(int*)p;    p += 4;     // mostly 1, means there're only one chart
    // read basic instrument info for note data filling
    int midichannel_per_track[20];
    int playmode = 0;   // SP or DP ...
    int level = 0;
    std::string title;
    for (int i=0; i<cnt_inst; i++) {
        p += 1;
        midichannel_per_track[i] = (int*)p;
        p += 4;
    }
    for (int i=0; i<cnt_chart; i++) {
        playmode = *p;  // SP=0, DP=1
        p++;
        level = *p; level++;
        p++;
        short len = *(short*)p;
        char buf[256];
        memcpy(buf, p, len);    p += len;
        title = buf;
        // track index?
        p += 4;
    }
    // parse note objects per each channels.
    // (includes BGM/autoplay objects)
    // TODO: make it as structure
    for (int i=0; i<7; i++) {
        int channel = i;
        int notecnt = *(int*)p;
        p += 4;
        for (int j=0; j<notecnt; j++) {
            VOSNoteDataV2 note;
            memcpy(&note, p, sizeof(VOSNoteDataV2));
            p += sizeof(VOSNoteDataV2);
        }
    }

    // done. we'll ignore other metadatas.
    return true;
}

bool ParseNoteDataV2( const char* p, int iLen, NoteData& nd )
{
    return true;

}

// V3
bool ParseMetaDataV3( const char* p, int iLen, MetaData& md )
{
    int version = *(int*)p; p+=4;
    int headersize = *(int*)p; p+=4;
    p += 4; // inf; static flag
    p += 12; // unknown null bytes
    int infpos = *(int*)p; p+=4; // end of inf file pos (start of MID file)
    p += 4; // mid; static flag
    p += 12; // unknown null bytes (maybe 64bit long long int?)
    int midpos = *(int*)p; p+=4;
    p += 4; // EOF flag
    p += 12; // unknown null bytes
    // now position is 64(0x40), which means starting pos of header.
    // start to read METADATAS
    char metadatas[][10] = ["TITLE", "ARTIST", "CHARTMAKER", "GENRE"];
    VOSHeader header;
    for (int i=0; i<4; i++) {
        header.len = *(int*)p; p+=4;
        memcpy(header.desc, p, header.len); p+=header.len;
        header.desc[header.len] = 0;
    }

    int genre = *p; p++;
    p++;    // unknown
    int songlength = *(int*)p; p+=4;
    int level = *p; level++; p++;
    p += 4;
    p++;
    p  += 1018; // null; unknown

    // start to read note data
    while (true) {
        int midiinstrument = *(int*)p; p+=4;
        short cnt = *(short*)p; p+=4;
        if (cnt == 0) break;
        p += 16;
        for (int i=0; i<cnt; i++) {
            VOSNoteDataV3 note;
            memcpy(&note, p, sizeof(VOSNoteDataV3));
            p += sizeof(VOSNoteDataV3);
        }
    }

    // done. we'll ignore other metadatas.
    return true;
}

bool ParseNoteDataV3( const char* p, int iLen, NoteData& nd )
{    
    return true;
}

int ReadMSInt( const char **_p ) {
    const char *p = *_p;
    int r = 0;
    int i = 0;
    while (true) {
        r = (r << 7) | (*(int*)p & 0x00FFFFFF);
        char sign = *p;
        p += 4;
        if (sign == 0) break;
        ASSERT(++i < 4);
    }

    *_p = p;
    return r;
}

} /* end NoteLoaderVOS */

} /* end rparser */