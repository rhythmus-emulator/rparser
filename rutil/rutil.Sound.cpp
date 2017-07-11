#include "rutil.Sound.h"
#include "rutil.h"
#include <stdint.h>

// ogg
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
// mp3
//#include <lame.h>
// flac
#include <FLAC/stream_decoder.h>
// midi
// TODO


/**
*  \name Audio format flags
*
*  Defaults to LSB byte order.
*/
/* @{ */
#define AUDIO_U8        0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /**< Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /**< As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /**< As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB
/* @} */

// CUSTOMIZED FORMAT
#define AUDIO_U4    0x0004  /**< Unsigned 4-bit samples */
#define AUDIO_S4    0x8004  /**< Unsigned 4-bit samples */
#define AUDIO_U24    0x0018  /**< Unsigned 24-bit samples */
#define AUDIO_S24    0x8018  /**< Signed 24-bit samples */

/**
*  \name int32 support
*/
/* @{ */
#define AUDIO_S32LSB    0x8020  /**< 32-bit integer samples */
#define AUDIO_S32MSB    0x9020  /**< As above, but big-endian byte order */
#define AUDIO_S32       AUDIO_S32LSB
/* @} */

/**
*  \name float32 support
*/
/* @{ */
#define AUDIO_F32LSB    0x8120  /**< 32-bit floating point samples */
#define AUDIO_F32MSB    0x9120  /**< As above, but big-endian byte order */
#define AUDIO_F32       AUDIO_F32LSB
/* @} */


#define SDL_AUDIO_MASK_BITSIZE       (0xFF)
#define SDL_AUDIO_MASK_DATATYPE      (1<<8)
#define SDL_AUDIO_MASK_ENDIAN        (1<<12)
#define SDL_AUDIO_MASK_SIGNED        (1<<15)
#define SDL_AUDIO_BITSIZE(x)         (x & SDL_AUDIO_MASK_BITSIZE)



namespace rutil
{

SoundData::SoundData()
{
	bit = 32;
	freq = 44100;
	samplesize = 0;
	channels = 2;
    format = AUDIO_S32;
	p = 0;
}

SoundData::~SoundData()
{
	if (p) free(p);
}

// ------ LoadSound utilities ------


void safe_free(void *p)
{
	if (p)
	{
		free(p);
	}
}



// ------ LoadSound ------

int LoadSound(const std::string& path, SoundData& dat)
{
	FILE *fp = fopen_utf8(path.c_str(), "r");
	if (!fp) return -1;
	fseek(fp, 0, SEEK_END);
	long long iLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	unsigned char *p = (unsigned char*)malloc(iLen);
	fread(p, 1, iLen, fp);
	fclose(fp);
	int r = LoadSound(p, iLen, dat);
	free(p);
	return r;
}

const char sig_mp3[] = { 0xFF, 0xFB };
const char sig_mp3_id3[] = "ID3";
int LoadSound(const unsigned char* p, long long iLen, SoundData& dat)
{
	// too short to even check signature
	if (iLen < 4)
		return -1;

	if (memcmp(p, "WAVE", 4) == 0 || memcmp(p, "RIFF", 4) == 0)
	{
		return LoadSound_WAV(p, iLen, dat);
	}
	else if (memcmp(p, sig_mp3, 2) == 0 || memcmp(p, sig_mp3_id3, 3) == 0)
	{
		return LoadSound_MP3(p, iLen, dat);
	}
	else if (memcmp(p, "OggS", 4) == 0)
	{
		return LoadSound_OGG(p, iLen, dat);
	}
	else if (memcmp(p, "fLaC", 4) == 0)
	{
		return LoadSound_FLAC(p, iLen, dat);
	}

	// invalid sound file format (even cannot recognize)
	return -1;
}




// *** WAVE ***
// code borrowed from SDL_Audio
#define Uint8 uint8_t
#define Uint16 uint16_t
#define Uint32 uint32_t
#define Sint8 int8_t
#define Sint16 int16_t
#define Sint32 int32_t

#define RIFF            0x46464952      /* "RIFF" */
#define WAVE            0x45564157      /* "WAVE" */
#define FACT            0x74636166      /* "fact" */
#define LIST            0x5453494c      /* "LIST" */
#define BEXT            0x74786562      /* "bext" */
#define JUNK            0x4B4E554A      /* "JUNK" */
#define FMT             0x20746D66      /* "fmt " */
#define DATA            0x61746164      /* "data" */
#define PCM_CODE        0x0001
#define MS_ADPCM_CODE   0x0002
#define IEEE_FLOAT_CODE 0x0003
#define IMA_ADPCM_CODE  0x0011
#define MP3_CODE        0x0055
#define WAVE_MONO       1
#define WAVE_STEREO     2



/* Normally, these three chunks come consecutively in a WAVE file */
typedef struct WaveFMT
{
	/* Not saved in the chunk we read:
	Uint32  FMTchunk;
	Uint32  fmtlen;
	*/
	Uint16 encoding;
	Uint16 channels;            /* 1 = mono, 2 = stereo */
	Uint32 frequency;           /* One of 11025, 22050, or 44100 Hz */
	Uint32 byterate;            /* Average bytes per second */
	Uint16 blockalign;          /* Bytes per sample block */
	Uint16 bitspersample;       /* One of 8, 12, 16, or 4 for ADPCM */
} WaveFMT;

/* The general chunk found in the WAVE file */
typedef struct Chunk
{
	Uint32 magic;
	Uint32 length;
	Uint8 *data;
} Chunk;

struct MS_ADPCM_decodestate
{
	Uint8 hPredictor;
	Uint16 iDelta;
	Sint16 iSamp1;
	Sint16 iSamp2;
};
static struct MS_ADPCM_decoder
{
	WaveFMT wavefmt;
	Uint16 wSamplesPerBlock;
	Uint16 wNumCoef;
	Sint16 aCoeff[7][2];
	/* * * */
	struct MS_ADPCM_decodestate state[2];
} MS_ADPCM_state;

#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)
#define SDL_SetError(x, ...) printf(x, ##__VA_ARGS__);
#define SDL_free(x) safe_free(x)

static int
InitMS_ADPCM(WaveFMT * format)
{
	Uint8 *rogue_feel;
	int i;

	/* Set the rogue pointer to the MS_ADPCM specific data */
	MS_ADPCM_state.wavefmt.encoding = SDL_SwapLE16(format->encoding);
	MS_ADPCM_state.wavefmt.channels = SDL_SwapLE16(format->channels);
	MS_ADPCM_state.wavefmt.frequency = SDL_SwapLE32(format->frequency);
	MS_ADPCM_state.wavefmt.byterate = SDL_SwapLE32(format->byterate);
	MS_ADPCM_state.wavefmt.blockalign = SDL_SwapLE16(format->blockalign);
	MS_ADPCM_state.wavefmt.bitspersample =
		SDL_SwapLE16(format->bitspersample);
	rogue_feel = (Uint8 *)format + sizeof(*format);
	if (sizeof(*format) == 16) {
		/* const Uint16 extra_info = ((rogue_feel[1] << 8) | rogue_feel[0]); */
		rogue_feel += sizeof(Uint16);
	}
	MS_ADPCM_state.wSamplesPerBlock = ((rogue_feel[1] << 8) | rogue_feel[0]);
	rogue_feel += sizeof(Uint16);
	MS_ADPCM_state.wNumCoef = ((rogue_feel[1] << 8) | rogue_feel[0]);
	rogue_feel += sizeof(Uint16);
	if (MS_ADPCM_state.wNumCoef != 7) {
		SDL_SetError("Unknown set of MS_ADPCM coefficients");
		return (-1);
	}
	for (i = 0; i < MS_ADPCM_state.wNumCoef; ++i) {
		MS_ADPCM_state.aCoeff[i][0] = ((rogue_feel[1] << 8) | rogue_feel[0]);
		rogue_feel += sizeof(Uint16);
		MS_ADPCM_state.aCoeff[i][1] = ((rogue_feel[1] << 8) | rogue_feel[0]);
		rogue_feel += sizeof(Uint16);
	}
	return (0);
}

static Sint32
MS_ADPCM_nibble(struct MS_ADPCM_decodestate *state,
	Uint8 nybble, Sint16 * coeff)
{
	const Sint32 max_audioval = ((1 << (16 - 1)) - 1);
	const Sint32 min_audioval = -(1 << (16 - 1));
	const Sint32 adaptive[] = {
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};
	Sint32 new_sample, delta;

	new_sample = ((state->iSamp1 * coeff[0]) +
		(state->iSamp2 * coeff[1])) / 256;
	if (nybble & 0x08) {
		new_sample += state->iDelta * (nybble - 0x10);
	}
	else {
		new_sample += state->iDelta * nybble;
	}
	if (new_sample < min_audioval) {
		new_sample = min_audioval;
	}
	else if (new_sample > max_audioval) {
		new_sample = max_audioval;
	}
	delta = ((Sint32)state->iDelta * adaptive[nybble]) / 256;
	if (delta < 16) {
		delta = 16;
	}
	state->iDelta = (Uint16)delta;
	state->iSamp2 = state->iSamp1;
	state->iSamp1 = (Sint16)new_sample;
	return (new_sample);
}

static int
MS_ADPCM_decode(Uint8 ** audio_buf, Uint32 * audio_len)
{
	struct MS_ADPCM_decodestate *state[2];
	Uint8 *freeable, *encoded, *decoded;
	Sint32 encoded_len, samplesleft;
	Sint8 nybble;
	Uint8 stereo;
	Sint16 *coeff[2];
	Sint32 new_sample;

	/* Allocate the proper sized output buffer */
	encoded_len = *audio_len;
	encoded = *audio_buf;
	freeable = *audio_buf;
	*audio_len = (encoded_len / MS_ADPCM_state.wavefmt.blockalign) *
		MS_ADPCM_state.wSamplesPerBlock *
		MS_ADPCM_state.wavefmt.channels * sizeof(Sint16);
	*audio_buf = (Uint8 *)malloc(*audio_len);
	if (*audio_buf == NULL) {
		return -1;
	}
	decoded = *audio_buf;

	/* Get ready... Go! */
	stereo = (MS_ADPCM_state.wavefmt.channels == 2);
	state[0] = &MS_ADPCM_state.state[0];
	state[1] = &MS_ADPCM_state.state[stereo];
	while (encoded_len >= MS_ADPCM_state.wavefmt.blockalign) {
		/* Grab the initial information for this block */
		state[0]->hPredictor = *encoded++;
		if (stereo) {
			state[1]->hPredictor = *encoded++;
		}
		state[0]->iDelta = ((encoded[1] << 8) | encoded[0]);
		encoded += sizeof(Sint16);
		if (stereo) {
			state[1]->iDelta = ((encoded[1] << 8) | encoded[0]);
			encoded += sizeof(Sint16);
		}
		state[0]->iSamp1 = ((encoded[1] << 8) | encoded[0]);
		encoded += sizeof(Sint16);
		if (stereo) {
			state[1]->iSamp1 = ((encoded[1] << 8) | encoded[0]);
			encoded += sizeof(Sint16);
		}
		state[0]->iSamp2 = ((encoded[1] << 8) | encoded[0]);
		encoded += sizeof(Sint16);
		if (stereo) {
			state[1]->iSamp2 = ((encoded[1] << 8) | encoded[0]);
			encoded += sizeof(Sint16);
		}
		coeff[0] = MS_ADPCM_state.aCoeff[state[0]->hPredictor];
		coeff[1] = MS_ADPCM_state.aCoeff[state[1]->hPredictor];

		/* Store the two initial samples we start with */
		decoded[0] = state[0]->iSamp2 & 0xFF;
		decoded[1] = state[0]->iSamp2 >> 8;
		decoded += 2;
		if (stereo) {
			decoded[0] = state[1]->iSamp2 & 0xFF;
			decoded[1] = state[1]->iSamp2 >> 8;
			decoded += 2;
		}
		decoded[0] = state[0]->iSamp1 & 0xFF;
		decoded[1] = state[0]->iSamp1 >> 8;
		decoded += 2;
		if (stereo) {
			decoded[0] = state[1]->iSamp1 & 0xFF;
			decoded[1] = state[1]->iSamp1 >> 8;
			decoded += 2;
		}

		/* Decode and store the other samples in this block */
		samplesleft = (MS_ADPCM_state.wSamplesPerBlock - 2) *
			MS_ADPCM_state.wavefmt.channels;
		while (samplesleft > 0) {
			nybble = (*encoded) >> 4;
			new_sample = MS_ADPCM_nibble(state[0], nybble, coeff[0]);
			decoded[0] = new_sample & 0xFF;
			new_sample >>= 8;
			decoded[1] = new_sample & 0xFF;
			decoded += 2;

			nybble = (*encoded) & 0x0F;
			new_sample = MS_ADPCM_nibble(state[1], nybble, coeff[1]);
			decoded[0] = new_sample & 0xFF;
			new_sample >>= 8;
			decoded[1] = new_sample & 0xFF;
			decoded += 2;

			++encoded;
			samplesleft -= 2;
		}
		encoded_len -= MS_ADPCM_state.wavefmt.blockalign;
	}
	free(freeable);
	return (0);
}

struct IMA_ADPCM_decodestate
{
	Sint32 sample;
	Sint8 index;
};
static struct IMA_ADPCM_decoder
{
	WaveFMT wavefmt;
	Uint16 wSamplesPerBlock;
	/* * * */
	struct IMA_ADPCM_decodestate state[2];
} IMA_ADPCM_state;

static int
InitIMA_ADPCM(WaveFMT * format)
{
	Uint8 *rogue_feel;

	/* Set the rogue pointer to the IMA_ADPCM specific data */
	IMA_ADPCM_state.wavefmt.encoding = SDL_SwapLE16(format->encoding);
	IMA_ADPCM_state.wavefmt.channels = SDL_SwapLE16(format->channels);
	IMA_ADPCM_state.wavefmt.frequency = SDL_SwapLE32(format->frequency);
	IMA_ADPCM_state.wavefmt.byterate = SDL_SwapLE32(format->byterate);
	IMA_ADPCM_state.wavefmt.blockalign = SDL_SwapLE16(format->blockalign);
	IMA_ADPCM_state.wavefmt.bitspersample =
		SDL_SwapLE16(format->bitspersample);
	rogue_feel = (Uint8 *)format + sizeof(*format);
	if (sizeof(*format) == 16) {
		/* const Uint16 extra_info = ((rogue_feel[1] << 8) | rogue_feel[0]); */
		rogue_feel += sizeof(Uint16);
	}
	IMA_ADPCM_state.wSamplesPerBlock = ((rogue_feel[1] << 8) | rogue_feel[0]);
	return (0);
}

static Sint32
IMA_ADPCM_nibble(struct IMA_ADPCM_decodestate *state, Uint8 nybble)
{
	const Sint32 max_audioval = ((1 << (16 - 1)) - 1);
	const Sint32 min_audioval = -(1 << (16 - 1));
	const int index_table[16] = {
		-1, -1, -1, -1,
		2, 4, 6, 8,
		-1, -1, -1, -1,
		2, 4, 6, 8
	};
	const Sint32 step_table[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
		34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
		143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408,
		449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282,
		1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
		3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
		9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
		22385, 24623, 27086, 29794, 32767
	};
	Sint32 delta, step;

	/* Compute difference and new sample value */
	if (state->index > 88) {
		state->index = 88;
	}
	else if (state->index < 0) {
		state->index = 0;
	}
	/* explicit cast to avoid gcc warning about using 'char' as array index */
	step = step_table[(int)state->index];
	delta = step >> 3;
	if (nybble & 0x04)
		delta += step;
	if (nybble & 0x02)
		delta += (step >> 1);
	if (nybble & 0x01)
		delta += (step >> 2);
	if (nybble & 0x08)
		delta = -delta;
	state->sample += delta;

	/* Update index value */
	state->index += index_table[nybble];

	/* Clamp output sample */
	if (state->sample > max_audioval) {
		state->sample = max_audioval;
	}
	else if (state->sample < min_audioval) {
		state->sample = min_audioval;
	}
	return (state->sample);
}

/* Fill the decode buffer with a channel block of data (8 samples) */
static void
Fill_IMA_ADPCM_block(Uint8 * decoded, Uint8 * encoded,
	int channel, int numchannels,
	struct IMA_ADPCM_decodestate *state)
{
	int i;
	Sint8 nybble;
	Sint32 new_sample;

	decoded += (channel * 2);
	for (i = 0; i < 4; ++i) {
		nybble = (*encoded) & 0x0F;
		new_sample = IMA_ADPCM_nibble(state, nybble);
		decoded[0] = new_sample & 0xFF;
		new_sample >>= 8;
		decoded[1] = new_sample & 0xFF;
		decoded += 2 * numchannels;

		nybble = (*encoded) >> 4;
		new_sample = IMA_ADPCM_nibble(state, nybble);
		decoded[0] = new_sample & 0xFF;
		new_sample >>= 8;
		decoded[1] = new_sample & 0xFF;
		decoded += 2 * numchannels;

		++encoded;
	}
}

#define SDL_arraysize(array) (sizeof(array)/sizeof(array[0]))
#define 	SDL_zerop(x)   memset((x), 0, sizeof(*(x)))
static int
IMA_ADPCM_decode(Uint8 ** audio_buf, Uint32 * audio_len)
{
	struct IMA_ADPCM_decodestate *state;
	Uint8 *freeable, *encoded, *decoded;
	Sint32 encoded_len, samplesleft;
	unsigned int c, channels;

	/* Check to make sure we have enough variables in the state array */
	channels = IMA_ADPCM_state.wavefmt.channels;
	if (channels > SDL_arraysize(IMA_ADPCM_state.state)) {
		SDL_SetError("IMA ADPCM decoder can only handle %u channels",
			(unsigned int)SDL_arraysize(IMA_ADPCM_state.state));
		return (-1);
	}
	state = IMA_ADPCM_state.state;

	/* Allocate the proper sized output buffer */
	encoded_len = *audio_len;
	encoded = *audio_buf;
	freeable = *audio_buf;
	*audio_len = (encoded_len / IMA_ADPCM_state.wavefmt.blockalign) *
		IMA_ADPCM_state.wSamplesPerBlock *
		IMA_ADPCM_state.wavefmt.channels * sizeof(Sint16);
	*audio_buf = (Uint8 *)malloc(*audio_len);
	if (*audio_buf == NULL) {
		return -1;
	}
	decoded = *audio_buf;

	/* Get ready... Go! */
	while (encoded_len >= IMA_ADPCM_state.wavefmt.blockalign) {
		/* Grab the initial information for this block */
		for (c = 0; c < channels; ++c) {
			/* Fill the state information for this block */
			state[c].sample = ((encoded[1] << 8) | encoded[0]);
			encoded += 2;
			if (state[c].sample & 0x8000) {
				state[c].sample -= 0x10000;
			}
			state[c].index = *encoded++;
			/* Reserved byte in buffer header, should be 0 */
			if (*encoded++ != 0) {
				/* Uh oh, corrupt data?  Buggy code? */;
			}

			/* Store the initial sample we start with */
			decoded[0] = (Uint8)(state[c].sample & 0xFF);
			decoded[1] = (Uint8)(state[c].sample >> 8);
			decoded += 2;
		}

		/* Decode and store the other samples in this block */
		samplesleft = (IMA_ADPCM_state.wSamplesPerBlock - 1) * channels;
		while (samplesleft > 0) {
			for (c = 0; c < channels; ++c) {
				Fill_IMA_ADPCM_block(decoded, encoded,
					c, channels, &state[c]);
				encoded += 4;
				samplesleft -= 8;
			}
			decoded += (channels * 8 * 2);
		}
		encoded_len -= IMA_ADPCM_state.wavefmt.blockalign;
	}
	free(freeable);
	return (0);
}

static int
ReadChunk(FileData &src, Chunk * chunk)
{
    chunk->magic = src.ReadLE32();
    chunk->length = src.ReadLE32();
    chunk->data = (Uint8 *)malloc(chunk->length);
    if (chunk->data == NULL) {
        // malloc failed?
        return -1;
    }
    if (!src.Read(chunk->data, chunk->length)) {
        SDL_free(chunk->data);
        chunk->data = NULL;
        printf("ERROR copying chunk data\n");
    }
    return (chunk->length);
}


int LoadSound_WAV(const uint8_t* p, size_t iLen, SoundData& dat)
{
    // Should only use Read method
    FileData src(const_cast<uint8_t*>(p), iLen);

	int was_error;
	Chunk chunk;
	int lenread;
	int IEEE_float_encoded, MS_ADPCM_encoded, IMA_ADPCM_encoded;
	int samplesize;
	memset(&chunk, 0, sizeof(chunk));

	/* FMT chunk */
	WaveFMT *format = NULL;

	uint32_t WAVEmagic = 0;
    uint32_t RIFFchunk = src.ReadLE32();
	uint32_t wavelen = src.ReadLE32();
	uint32_t headerDiff = 0;
	if (wavelen == WAVE) {
		WAVEmagic = wavelen;
		wavelen = RIFFchunk;
		RIFFchunk = RIFF;
	}
	else {
		WAVEmagic = src.ReadLE32();
	}
	if ((RIFFchunk != RIFF) || (WAVEmagic != WAVE)) {
		SDL_SetError("Unrecognized file type (not WAVE)");
		was_error = 1;
		goto done_WAV;
	}
	headerDiff += sizeof(Uint32);       /* for WAVE */

    /* Read the audio data format chunk */
	chunk.data = NULL;
	do {
		SDL_free(chunk.data);
		chunk.data = NULL;
		lenread = ReadChunk(src, &chunk);
		if (lenread < 0) {
			was_error = 1;
			goto done_WAV;
		}
		/* 2 Uint32's for chunk header+len, plus the lenread */
		headerDiff += lenread + 2 * sizeof(Uint32);
	} while ((chunk.magic == FACT) || (chunk.magic == LIST) || (chunk.magic == BEXT) || (chunk.magic == JUNK));

	/* Decode the audio data format */
	format = (WaveFMT *)chunk.data;
	if (chunk.magic != FMT) {
		SDL_SetError("Complex WAVE files not supported");
		was_error = 1;
		goto done_WAV;
	}
	IEEE_float_encoded = MS_ADPCM_encoded = IMA_ADPCM_encoded = 0;
	switch (SDL_SwapLE16(format->encoding)) {
	case PCM_CODE:
		/* We can understand this */
		break;
	case IEEE_FLOAT_CODE:
		IEEE_float_encoded = 1;
		/* We can understand this */
		break;
	case MS_ADPCM_CODE:
		/* Try to understand this */
		if (InitMS_ADPCM(format) < 0) {
			was_error = 1;
			goto done_WAV;
		}
		MS_ADPCM_encoded = 1;
		break;
	case IMA_ADPCM_CODE:
		/* Try to understand this */
		if (InitIMA_ADPCM(format) < 0) {
			was_error = 1;
			goto done_WAV;
		}
		IMA_ADPCM_encoded = 1;
		break;
	case MP3_CODE:
		SDL_SetError("MPEG Layer 3 data not supported");
		was_error = 1;
		goto done_WAV;
	default:
		SDL_SetError("Unknown WAVE data format: 0x%.4x",
			SDL_SwapLE16(format->encoding));
		was_error = 1;
		goto done_WAV;
	}

    // fill audio format
	SDL_zerop(&dat);
	dat.freq = SDL_SwapLE32(format->frequency);

	if (IEEE_float_encoded) {
		if ((SDL_SwapLE16(format->bitspersample)) != 32) {
			was_error = 1;
		}
		else {
			dat.format = AUDIO_F32;
		}
	}
	else {
		switch (SDL_SwapLE16(format->bitspersample)) {
		case 4:
			if (MS_ADPCM_encoded || IMA_ADPCM_encoded) {
				dat.format = AUDIO_S16;
			}
			else {
                // 8 bit (or lower) WAV files are always unsigned.
                // higher -> signed.
                printf("WARNING: 4bit WAV audio file found.\n");
                dat.format = AUDIO_U4;
				//was_error = 1;
			}
			break;
		case 8:
			dat.format = AUDIO_U8;
			break;
		case 16:
		    dat.format = AUDIO_S16;
			break;
        case 24:
            dat.format = AUDIO_S24;
            printf("WARNING: 4bit WAV audio file found.\n");
            break;
		case 32:
			dat.format = AUDIO_S32;
			break;
		default:
			was_error = 1;
			break;
		}
	}
	


	if (was_error) {
		SDL_SetError("Unknown %d-bit PCM data format",
			SDL_SwapLE16(format->bitspersample));
		goto done_WAV;
	}
	dat.channels = (Uint8)SDL_SwapLE16(format->channels);
	//dat.samples = 4096;       /* Good default buffer size */
								/* Read the audio data chunk */
	uint8_t *audio_buf = NULL;
    uint32_t audio_len = 0;
	do {
		SDL_free(audio_buf);
		audio_buf = NULL;
		lenread = ReadChunk(src, &chunk);
		if (lenread < 0) {
			was_error = 1;
			goto done_WAV;
		}
		audio_len = lenread;
		audio_buf = chunk.data;
		if (chunk.magic != DATA)
			headerDiff += lenread + 2 * sizeof(Uint32);
	} while (chunk.magic != DATA);
	headerDiff += 2 * sizeof(Uint32);   /* for the data chunk and len */

	if (MS_ADPCM_encoded) {
		if (MS_ADPCM_decode(&audio_buf, &audio_len) < 0) {
			was_error = 1;
			goto done_WAV;
		}
	}
	if (IMA_ADPCM_encoded) {
		if (IMA_ADPCM_decode(&audio_buf, &audio_len) < 0) {
			was_error = 1;
			goto done_WAV;
		}
	}

	/* Don't return a buffer that isn't a multiple of samplesize */
	samplesize = ((SDL_AUDIO_BITSIZE(dat.format)) / 8) * dat.channels;
	audio_len &= ~(samplesize - 1);

    dat.p = audio_buf;
    dat.samplesize = audio_len / (SDL_AUDIO_BITSIZE(dat.format));

    // TODO: requires convert to S16?
    
done_WAV:
	SDL_free(format);
	if (was_error) {
        // nothing to do?
        return was_error;
	}

    return 0;
}



// ------ OGG ------

typedef struct {
    int loaded;
    void *handle;
    int(*ov_clear)(OggVorbis_File *vf);
    vorbis_info *(*ov_info)(OggVorbis_File *vf, int link);
    int(*ov_open_callbacks)(void *datasource, OggVorbis_File *vf, const char *initial, long ibytes, ov_callbacks callbacks);
    ogg_int64_t(*ov_pcm_total)(OggVorbis_File *vf, int i);
#ifdef OGG_USE_TREMOR
    long(*ov_read)(OggVorbis_File *vf, char *buffer, int length, int *bitstream);
#else
    long(*ov_read)(OggVorbis_File *vf, char *buffer, int length, int bigendianp, int word, int sgned, int *bitstream);
#endif
#ifdef OGG_USE_TREMOR
    int(*ov_time_seek)(OggVorbis_File *vf, ogg_int64_t pos);
#else
    int(*ov_time_seek)(OggVorbis_File *vf, double pos);
#endif
} vorbis_loader;

vorbis_loader vorbis = {
    0, NULL
};

#ifdef OGG_DYNAMIC
int Mix_InitOgg()
{
    if (vorbis.loaded == 0) {
        vorbis.handle = SDL_LoadObject(OGG_DYNAMIC);
        if (vorbis.handle == NULL) {
            return -1;
        }
        vorbis.ov_clear =
            (int(*)(OggVorbis_File *))
            SDL_LoadFunction(vorbis.handle, "ov_clear");
        if (vorbis.ov_clear == NULL) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_info =
            (vorbis_info *(*)(OggVorbis_File *, int))
            SDL_LoadFunction(vorbis.handle, "ov_info");
        if (vorbis.ov_info == NULL) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_open_callbacks =
            (int(*)(void *, OggVorbis_File *, const char *, long, ov_callbacks))
            SDL_LoadFunction(vorbis.handle, "ov_open_callbacks");
        if (vorbis.ov_open_callbacks == NULL) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_pcm_total =
            (ogg_int64_t(*)(OggVorbis_File *, int))
            SDL_LoadFunction(vorbis.handle, "ov_pcm_total");
        if (vorbis.ov_pcm_total == NULL) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_read =
#ifdef OGG_USE_TREMOR
        (long(*)(OggVorbis_File *, char *, int, int *))
#else
            (long(*)(OggVorbis_File *, char *, int, int, int, int, int *))
#endif
            SDL_LoadFunction(vorbis.handle, "ov_read");
        if (vorbis.ov_read == NULL) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
        vorbis.ov_time_seek =
#ifdef OGG_USE_TREMOR
        (long(*)(OggVorbis_File *, ogg_int64_t))
#else
            (int(*)(OggVorbis_File *, double))
#endif
            SDL_LoadFunction(vorbis.handle, "ov_time_seek");
        if (vorbis.ov_time_seek == NULL) {
            SDL_UnloadObject(vorbis.handle);
            return -1;
        }
    }
    ++vorbis.loaded;

    return 0;
}
void Mix_QuitOgg()
{
    if (vorbis.loaded == 0) {
        return;
    }
    if (vorbis.loaded == 1) {
        SDL_UnloadObject(vorbis.handle);
    }
    --vorbis.loaded;
}
#else
int Mix_InitOgg()
{
    if (vorbis.loaded == 0) {
#ifdef __MACOSX__
        extern int ov_open_callbacks(void*, OggVorbis_File*, const char*, long, ov_callbacks) __attribute__((weak_import));
        if (ov_open_callbacks == NULL)
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing Vorbis.framework");
            return -1;
        }
#endif // __MACOSX__

        vorbis.ov_clear = ov_clear;
        vorbis.ov_info = ov_info;
        vorbis.ov_open_callbacks = ov_open_callbacks;
        vorbis.ov_pcm_total = ov_pcm_total;
        vorbis.ov_read = ov_read;
        vorbis.ov_time_seek = ov_time_seek;
    }
    ++vorbis.loaded;

    return 0;
}
void Mix_QuitOgg()
{
    if (vorbis.loaded == 0) {
        return;
    }
    if (vorbis.loaded == 1) {
        SDL_UnloadObject(vorbis.handle);
    }
    --vorbis.loaded;
}
#endif /* OGG_DYNAMIC */

/*
 * OGG Init helper object
 */
class OGG_Init_helper
{
    OGG_Init_helper()
    {
        ASSERT(Mix_InitOgg() == 0);
    }

    ~OGG_Init_helper()
    {
        Mix_QuitOgg();
    }
};
OGG_Init_helper _ogg_init_helper;

static size_t sdl_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    return ((FileData*)datasource)->Read((uint8_t*)ptr, size * nmemb);
}

static int sdl_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    return (int)((FileData*)datasource)->Seek(offset, whence);
}

static int sdl_close_func_freesrc(void *datasource)
{
    // nothing to do
    return 0;
    //return SDL_RWclose((SDL_RWops*)datasource);
}

static int sdl_close_func_nofreesrc(void *datasource)
{
    ((FileData*)datasource)->SeekSet();
    return 0;
}

static long sdl_tell_func(void *datasource)
{
    return (long)((FileData*)datasource)->GetPos();
}

int LoadSound_OGG(const uint8_t* p, size_t iLen, SoundData &spec)
{
	OggVorbis_File vf;
	ov_callbacks callbacks;
	vorbis_info *info;
	Uint8 *buf;
	int bitstream = -1;
	long samplesize;
	long samples;
	int read, to_read;
	int must_close = 1;
	int was_error = 1;

	if ((!p) || (!iLen))   /* sanity checks. */
		goto done_OGG;

	callbacks.read_func = sdl_read_func;
	callbacks.seek_func = sdl_seek_func;
	callbacks.tell_func = sdl_tell_func;
	callbacks.close_func = sdl_close_func_nofreesrc;    // reset pos when finished

	if (vorbis.ov_open_callbacks((void*)p, &vf, NULL, 0, callbacks) != 0)
	{
		SDL_SetError("OGG bitstream is not valid Vorbis stream!");
		goto done_OGG;
	}

	must_close = 0;

	info = vorbis.ov_info(&vf, -1);

	uint8_t* audio_buf = NULL;
	uint32_t audio_len = 0;
    SDL_zerop(&spec);

	spec.format = AUDIO_S16;
	spec.channels = info->channels;
	spec.freq = info->rate;
	//spec.samples = 4096; /* buffer size */

	samples = (long)vorbis.ov_pcm_total(&vf, -1);

	audio_len = spec.samplesize = samples * spec.channels * 2;
	audio_buf = (Uint8 *)malloc(audio_len);
	if (audio_buf == NULL)
		goto done_OGG;

	buf = audio_buf;
	to_read = audio_len;
#ifdef OGG_USE_TREMOR
	for (read = vorbis.ov_read(&vf, (char *)buf, to_read, &bitstream);
		read > 0;
		read = vorbis.ov_read(&vf, (char *)buf, to_read, &bitstream))
#else
	for (read = vorbis.ov_read(&vf, (char *)buf, to_read, 0/*LE*/, 2/*16bit*/, 1/*signed*/, &bitstream);
		read > 0;
		read = vorbis.ov_read(&vf, (char *)buf, to_read, 0, 2, 1, &bitstream))
#endif
	{
		if (read == OV_HOLE || read == OV_EBADLINK)
			break; /* error */

		to_read -= read;
		buf += read;
	}

	vorbis.ov_clear(&vf);
	was_error = 0;

	/* Don't return a buffer that isn't a multiple of samplesize */
	samplesize = ((spec.format & 0xFF) / 8)*spec.channels;
	audio_len &= ~(samplesize - 1);
    spec.p = audio_buf;

done_OGG:

	if (was_error) {
        // what to do on error?
        return was_error;
	}

    return 0;
}





// ------ FLAC ------

typedef struct {
    int loaded;
    void *handle;
    FLAC__StreamDecoder *(*FLAC__stream_decoder_new)();
    void(*FLAC__stream_decoder_delete)(FLAC__StreamDecoder *decoder);
    FLAC__StreamDecoderInitStatus(*FLAC__stream_decoder_init_stream)(
        FLAC__StreamDecoder *decoder,
        FLAC__StreamDecoderReadCallback read_callback,
        FLAC__StreamDecoderSeekCallback seek_callback,
        FLAC__StreamDecoderTellCallback tell_callback,
        FLAC__StreamDecoderLengthCallback length_callback,
        FLAC__StreamDecoderEofCallback eof_callback,
        FLAC__StreamDecoderWriteCallback write_callback,
        FLAC__StreamDecoderMetadataCallback metadata_callback,
        FLAC__StreamDecoderErrorCallback error_callback,
        void *client_data);
    FLAC__bool(*FLAC__stream_decoder_finish)(FLAC__StreamDecoder *decoder);
    FLAC__bool(*FLAC__stream_decoder_flush)(FLAC__StreamDecoder *decoder);
    FLAC__bool(*FLAC__stream_decoder_process_single)(
        FLAC__StreamDecoder *decoder);
    FLAC__bool(*FLAC__stream_decoder_process_until_end_of_metadata)(
        FLAC__StreamDecoder *decoder);
    FLAC__bool(*FLAC__stream_decoder_process_until_end_of_stream)(
        FLAC__StreamDecoder *decoder);
    FLAC__bool(*FLAC__stream_decoder_seek_absolute)(
        FLAC__StreamDecoder *decoder,
        FLAC__uint64 sample);
    FLAC__StreamDecoderState(*FLAC__stream_decoder_get_state)(
        const FLAC__StreamDecoder *decoder);
} flac_loader;

flac_loader flac = {
    0, NULL
};

#ifdef FLAC_DYNAMIC
int Mix_InitFLAC()
{
    if (flac.loaded == 0) {
        flac.handle = SDL_LoadObject(FLAC_DYNAMIC);
        if (flac.handle == NULL) {
            return -1;
        }
        flac.FLAC__stream_decoder_new =
            (FLAC__StreamDecoder *(*)())
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_new");
        if (flac.FLAC__stream_decoder_new == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_delete =
            (void(*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_delete");
        if (flac.FLAC__stream_decoder_delete == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_init_stream =
            (FLAC__StreamDecoderInitStatus(*)(
                FLAC__StreamDecoder *,
                FLAC__StreamDecoderReadCallback,
                FLAC__StreamDecoderSeekCallback,
                FLAC__StreamDecoderTellCallback,
                FLAC__StreamDecoderLengthCallback,
                FLAC__StreamDecoderEofCallback,
                FLAC__StreamDecoderWriteCallback,
                FLAC__StreamDecoderMetadataCallback,
                FLAC__StreamDecoderErrorCallback,
                void *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_init_stream");
        if (flac.FLAC__stream_decoder_init_stream == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_finish =
            (FLAC__bool(*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_finish");
        if (flac.FLAC__stream_decoder_finish == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_flush =
            (FLAC__bool(*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_flush");
        if (flac.FLAC__stream_decoder_flush == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_process_single =
            (FLAC__bool(*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle,
                "FLAC__stream_decoder_process_single");
        if (flac.FLAC__stream_decoder_process_single == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_process_until_end_of_metadata =
            (FLAC__bool(*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle,
                "FLAC__stream_decoder_process_until_end_of_metadata");
        if (flac.FLAC__stream_decoder_process_until_end_of_metadata == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_process_until_end_of_stream =
            (FLAC__bool(*)(FLAC__StreamDecoder *))
            SDL_LoadFunction(flac.handle,
                "FLAC__stream_decoder_process_until_end_of_stream");
        if (flac.FLAC__stream_decoder_process_until_end_of_stream == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_seek_absolute =
            (FLAC__bool(*)(FLAC__StreamDecoder *, FLAC__uint64))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_seek_absolute");
        if (flac.FLAC__stream_decoder_seek_absolute == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
        flac.FLAC__stream_decoder_get_state =
            (FLAC__StreamDecoderState(*)(const FLAC__StreamDecoder *decoder))
            SDL_LoadFunction(flac.handle, "FLAC__stream_decoder_get_state");
        if (flac.FLAC__stream_decoder_get_state == NULL) {
            SDL_UnloadObject(flac.handle);
            return -1;
        }
    }
    ++flac.loaded;

    return 0;
}
void Mix_QuitFLAC()
{
    if (flac.loaded == 0) {
        return;
    }
    if (flac.loaded == 1) {
        SDL_UnloadObject(flac.handle);
    }
    --flac.loaded;
}
#else
int Mix_InitFLAC()
{
    if (flac.loaded == 0) {
#ifdef __MACOSX__
        extern FLAC__StreamDecoder *FLAC__stream_decoder_new(void) __attribute__((weak_import));
        if (FLAC__stream_decoder_new == NULL)
        {
            /* Missing weakly linked framework */
            Mix_SetError("Missing FLAC.framework");
            return -1;
        }
#endif // __MACOSX__

        flac.FLAC__stream_decoder_new = FLAC__stream_decoder_new;
        flac.FLAC__stream_decoder_delete = FLAC__stream_decoder_delete;
        flac.FLAC__stream_decoder_init_stream =
            FLAC__stream_decoder_init_stream;
        flac.FLAC__stream_decoder_finish = FLAC__stream_decoder_finish;
        flac.FLAC__stream_decoder_flush = FLAC__stream_decoder_flush;
        flac.FLAC__stream_decoder_process_single =
            FLAC__stream_decoder_process_single;
        flac.FLAC__stream_decoder_process_until_end_of_metadata =
            FLAC__stream_decoder_process_until_end_of_metadata;
        flac.FLAC__stream_decoder_process_until_end_of_stream =
            FLAC__stream_decoder_process_until_end_of_stream;
        flac.FLAC__stream_decoder_seek_absolute =
            FLAC__stream_decoder_seek_absolute;
        flac.FLAC__stream_decoder_get_state =
            FLAC__stream_decoder_get_state;
    }
    ++flac.loaded;

    return 0;
}
void Mix_QuitFLAC()
{
    if (flac.loaded == 0) {
        return;
    }
    if (flac.loaded == 1) {
    }
    --flac.loaded;
}
#endif /* FLAC_DYNAMIC */

class FLAC_init_helper
{
    FLAC_init_helper()
    {
        Mix_InitFLAC();
    }
    ~FLAC_init_helper()
    {
        Mix_QuitFLAC();
    }
};
FLAC_init_helper _flac_init_helper;


typedef struct {
    FileData* src;
    SoundData* spec;
	Uint8** sdl_audio_buf;
	Uint32* sdl_audio_len;
	int sdl_audio_read;
	FLAC__uint64 flac_total_samples;
	unsigned flac_bps;
} FLAC_SDL_Data;

static FLAC__StreamDecoderReadStatus flac_read_load_cb(
	const FLAC__StreamDecoder *decoder,
	FLAC__byte buffer[],
	size_t *bytes,
	void *client_data)
{
	// make sure there is something to be reading
	if (*bytes > 0) {
		FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

		*bytes = data->src->Read(buffer, sizeof(FLAC__byte) * (*bytes));

		if (*bytes == 0) { // error or no data was read (EOF)
			return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
		}
		else { // data was read, continue
			return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
		}
	}
	else {
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}
}

static FLAC__StreamDecoderSeekStatus flac_seek_load_cb(
	const FLAC__StreamDecoder *decoder,
	FLAC__uint64 absolute_byte_offset,
	void *client_data)
{
	FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

	if (data->src->Seek(absolute_byte_offset, SEEK_SET) < 0) {
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}
	else {
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}
}

static FLAC__StreamDecoderTellStatus flac_tell_load_cb(
	const FLAC__StreamDecoder *decoder,
	FLAC__uint64 *absolute_byte_offset,
	void *client_data)
{
	FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

    int64_t pos = data->src->GetPos();

	if (pos < 0) {
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

static FLAC__StreamDecoderLengthStatus flac_length_load_cb(
	const FLAC__StreamDecoder *decoder,
	FLAC__uint64 *stream_length,
	void *client_data)
{
	FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

	int64_t pos = data->src->GetPos();
    int64_t length = data->src->Seek(0, SEEK_END);

	if (data->src->Seek(pos, SEEK_SET) != 0 || length < 0) {
		/* there was an error attempting to return the stream to the original
		* position, or the length was invalid. */
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	}
	else {
		*stream_length = (FLAC__uint64)length;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	}
}

static FLAC__bool flac_eof_load_cb(const FLAC__StreamDecoder *decoder,
	void *client_data)
{
	FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;

    //int64_t pos = data->src->GetPos();
    //int64_t end = data->src->Seek(0, SEEK_END);
    //bool isEOF = pos == end;
    bool isEOF = data->src->IsEOF();

	// was the original position equal to the end (a.k.a. the seek didn't move)?
	if (isEOF) {
		// must be EOF
		return true;
	}
	else {
		// not EOF, return to the original position
        //data->src->Seek(pos, SEEK_SET);
		return false;
	}
}

static FLAC__StreamDecoderWriteStatus flac_write_load_cb(
	const FLAC__StreamDecoder *decoder,
	const FLAC__Frame *frame,
	const FLAC__int32 *const buffer[],
	void *client_data)
{
	FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;
	size_t i;
	Uint8 *buf;

	if (data->flac_total_samples == 0) {
		SDL_SetError("Given FLAC file does not specify its sample count.");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	if (data->spec->channels != 2 || data->flac_bps != 16) {
		SDL_SetError("Current FLAC support is only for 16 bit Stereo files.");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	// check if it is the first audio frame so we can initialize the output
	// buffer
	if (frame->header.number.sample_number == 0) {
		*(data->sdl_audio_len) = data->spec->size;
		data->sdl_audio_read = 0;
		*(data->sdl_audio_buf) = (uint8_t*)malloc(*(data->sdl_audio_len));

		if (*(data->sdl_audio_buf) == NULL) {
			SDL_SetError
			("Unable to allocate memory to store the FLAC stream.");
			return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
		}
	}

	buf = *(data->sdl_audio_buf);

	for (i = 0; i < frame->header.blocksize; i++) {
		FLAC__int16 i16;
		FLAC__uint16 ui16;

		i16 = (FLAC__int16)buffer[0][i];
		ui16 = (FLAC__uint16)i16;

		*(buf + (data->sdl_audio_read++)) = (char)(ui16);
		*(buf + (data->sdl_audio_read++)) = (char)(ui16 >> 8);

		i16 = (FLAC__int16)buffer[1][i];
		ui16 = (FLAC__uint16)i16;

		*(buf + (data->sdl_audio_read++)) = (char)(ui16);
		*(buf + (data->sdl_audio_read++)) = (char)(ui16 >> 8);
	}

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void flac_metadata_load_cb(
	const FLAC__StreamDecoder *decoder,
	const FLAC__StreamMetadata *metadata,
	void *client_data)
{
	FLAC_SDL_Data *data = (FLAC_SDL_Data *)client_data;
	FLAC__uint64 total_samples;
	unsigned bps;

	if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		// save the metadata right now for use later on
		*(data->sdl_audio_buf) = NULL;
		*(data->sdl_audio_len) = 0;
		memset(data->spec, '\0', sizeof(SoundData));

		data->spec->format = AUDIO_S16;
		data->spec->freq = (int)(metadata->data.stream_info.sample_rate);
		data->spec->channels = (Uint8)(metadata->data.stream_info.channels);
		//data->spec->samples = 8192; /* buffer size */

		total_samples = metadata->data.stream_info.total_samples;
		bps = metadata->data.stream_info.bits_per_sample;

		data->spec->size = (Uint32)(total_samples * data->spec->channels * (bps / 8));
		data->flac_total_samples = total_samples;
		data->flac_bps = bps;
	}
}

static void flac_error_load_cb(
	const FLAC__StreamDecoder *decoder,
	FLAC__StreamDecoderErrorStatus status,
	void *client_data)
{
	// print an SDL error based on the error status
	switch (status) {
	case FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC:
		SDL_SetError("Error processing the FLAC file [LOST_SYNC].");
		break;
	case FLAC__STREAM_DECODER_ERROR_STATUS_BAD_HEADER:
		SDL_SetError("Error processing the FLAC file [BAD_HEADER].");
		break;
	case FLAC__STREAM_DECODER_ERROR_STATUS_FRAME_CRC_MISMATCH:
		SDL_SetError("Error processing the FLAC file [CRC_MISMATCH].");
		break;
	case FLAC__STREAM_DECODER_ERROR_STATUS_UNPARSEABLE_STREAM:
		SDL_SetError("Error processing the FLAC file [UNPARSEABLE].");
		break;
	default:
		SDL_SetError("Error processing the FLAC file [UNKNOWN].");
		break;
	}
}

/* don't call this directly; use Mix_LoadWAV_RW() for now. */
int LoadSound_FLAC(const uint8_t* p, size_t iLen, SoundData& spec)
{
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;
	int was_error = 1;
	int was_init = 0;
	Uint32 samplesize;

	// create the client data passing information
	FLAC_SDL_Data* client_data;
	client_data = (FLAC_SDL_Data *)malloc(sizeof(FLAC_SDL_Data));

	if ((!p) || (!iLen))   /* sanity checks. */
		goto done;

	if (!Mix_Init(MIX_INIT_FLAC))
		goto done;

	if ((decoder = flac.FLAC__stream_decoder_new()) == NULL) {
		SDL_SetError("Unable to allocate FLAC decoder.");
		goto done;
	}

	init_status = flac.FLAC__stream_decoder_init_stream(decoder,
		flac_read_load_cb, flac_seek_load_cb,
		flac_tell_load_cb, flac_length_load_cb,
		flac_eof_load_cb, flac_write_load_cb,
		flac_metadata_load_cb, flac_error_load_cb,
		client_data);

	if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		SDL_SetError("Unable to initialize FLAC stream decoder.");
		goto done;
	}

	was_init = 1;

    uint8_t *audio_buf = 0;
    uint32_t audio_len = 0;

	client_data->src = new FileData((uint8_t*)p, iLen);
	client_data->spec = &spec;
	client_data->sdl_audio_buf = &audio_buf;
	client_data->sdl_audio_len = &audio_len;

	if (!flac.FLAC__stream_decoder_process_until_end_of_stream(decoder)) {
		SDL_SetError("Unable to process FLAC file.");
		goto done;
	}

	was_error = 0;

	/* Don't return a buffer that isn't a multiple of samplesize */
	samplesize = ((spec.format & 0xFF) / 8) * spec.channels;
	audio_len &= ~(samplesize - 1);
    spec.size = audio_len;
    spec.p = audio_buf;

done:
	if (was_init && decoder) {
		flac.FLAC__stream_decoder_finish(decoder);
	}

	if (decoder) {
		flac.FLAC__stream_decoder_delete(decoder);
	}

	if (was_error) {
        return was_error;
	}

	return 0;
}





// ------ MP3 ------

int LoadSound_MP3(const uint8_t* p, size_t iLen, SoundData& dat)
{
	// TODO
	ASSERT(0);
	return -1;
}

}