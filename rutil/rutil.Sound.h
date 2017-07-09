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
	int size;
	int rate;
	int channels;
	// @description
	// memory occupied size: bit * size * rate
	void* p;

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
int LoadSound_MP3(const unsigned char* p, long long iLen, SoundData& dat);
int LoadSound_FLAC(const unsigned char* p, long long iLen, SoundData& dat);
int LoadSound_OGG(const unsigned char* p, long long iLen, SoundData& dat);
int LoadSound_WAV(const unsigned char* p, long long iLen, SoundData& dat);

// ------ midi support part ------
// uses PortAudio here

// ------ mixer part ------

}

#endif