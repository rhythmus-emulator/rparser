/*
 * by @lazykuna, MIT License.
 */

#ifndef RUTIL_SOUND_H
#define RUTIL_SOUND_H

#include <string>

namespace rutil
{

class SoundData
{
public:
	int bit;
    // TODO: replace samplesize -> size.
    // TODO: create GetSampleCount() method.
	// total sample size: channels * freq * duration(in sec)
	int samplesize;
	int freq;
	int channels;
	// @description memory data
    // size: samplesize * formatsize
	void* p;
    // @description
    // just detail info during parsing...
    int format;

	SoundData();
	~SoundData();
};

// @description
// load sound in WAV form to sounddata.
// if bit size is not specified (bit=0), then load as default bit size.
// if not, then load as 32-bit unsigned.
int LoadSound(const std::string& path, SoundData& dat);
int LoadSound(const unsigned char* p, long long iLen, SoundData& dat);


// you don't need to call these functions directly.
int LoadSound_MP3(const uint8_t* p, size_t iLen, SoundData& dat);
int LoadSound_FLAC(const uint8_t* p, size_t iLen, SoundData& dat);
int LoadSound_OGG(const uint8_t* p, size_t iLen, SoundData& dat);
int LoadSound_WAV(const uint8_t* p, size_t iLen, SoundData& dat);

// ------ midi support part ------
// uses PortAudio here

// ------ mixer part ------

}

#endif