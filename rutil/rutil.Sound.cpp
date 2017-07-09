#include "rutil.Sound.h"
#include "rutil.h"

// ogg
#include <vorbis/vorbisfile.h>
// mp3
//#include <lame.h>
// flac
#include <FLAC/stream_decoder.h>
// midi
// TODO

namespace rutil
{

SoundData::SoundData()
{
	bit = 32;
	rate = 44100;
	size = 0;
	channels = 2;
	p = 0;
}

SoundData::~SoundData()
{
	if (p) free(p);
}

// ------ LoadSound utilities ------

uint32_t ReadLE32(const unsigned char* p)
{
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

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

int LoadSound_WAV(const unsigned char* p, long long iLen, SoundData& dat)
{
	int was_error;
	Chunk chunk;
	int lenread;
	int IEEE_float_encoded, MS_ADPCM_encoded, IMA_ADPCM_encoded;
	int samplesize;
	memset(&chunk, 0, sizeof(chunk));

	/* FMT chunk */
	WaveFMT *format = NULL;

	uint32_t WAVEmagic = 0;
	uint32_t RIFFchunk = ReadLE32(p);
	uint32_t wavelen = ReadLE32(p + 4);
	uint32_t headerDiff = 0;
	if (wavelen == WAVE) {
		WAVEmagic = wavelen;
		wavelen = RIFFchunk;
		RIFFchunk = RIFF;
	}
	else {
		WAVEmagic = ReadLE32(p + 8);
	}
	if ((RIFFchunk != RIFF) || (WAVEmagic != WAVE)) {
		SDL_SetError("Unrecognized file type (not WAVE)");
		was_error = 1;
		goto done;
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
			goto done;
		}
		/* 2 Uint32's for chunk header+len, plus the lenread */
		headerDiff += lenread + 2 * sizeof(Uint32);
	} while ((chunk.magic == FACT) || (chunk.magic == LIST) || (chunk.magic == BEXT) || (chunk.magic == JUNK));

	/* Decode the audio data format */
	format = (WaveFMT *)chunk.data;
	if (chunk.magic != FMT) {
		SDL_SetError("Complex WAVE files not supported");
		was_error = 1;
		goto done;
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
			goto done;
		}
		MS_ADPCM_encoded = 1;
		break;
	case IMA_ADPCM_CODE:
		/* Try to understand this */
		if (InitIMA_ADPCM(format) < 0) {
			was_error = 1;
			goto done;
		}
		IMA_ADPCM_encoded = 1;
		break;
	case MP3_CODE:
		SDL_SetError("MPEG Layer 3 data not supported");
		was_error = 1;
		goto done;
	default:
		SDL_SetError("Unknown WAVE data format: 0x%.4x",
			SDL_SwapLE16(format->encoding));
		was_error = 1;
		goto done;
	}
	SDL_zerop(spec);
	spec->freq = SDL_SwapLE32(format->frequency);

	if (IEEE_float_encoded) {
		if ((SDL_SwapLE16(format->bitspersample)) != 32) {
			was_error = 1;
		}
		else {
			spec->format = AUDIO_F32;
		}
	}
	else {
		switch (SDL_SwapLE16(format->bitspersample)) {
		case 4:
			if (MS_ADPCM_encoded || IMA_ADPCM_encoded) {
				spec->format = AUDIO_S16;
			}
			else {
				was_error = 1;
			}
			break;
		case 8:
			spec->format = AUDIO_U8;
			break;
		case 16:
			spec->format = AUDIO_S16;
			break;
		case 32:
			spec->format = AUDIO_S32;
			break;
		default:
			was_error = 1;
			break;
		}
	}
	


	if (was_error) {
		SDL_SetError("Unknown %d-bit PCM data format",
			SDL_SwapLE16(format->bitspersample));
		goto done;
	}
	spec->channels = (Uint8)SDL_SwapLE16(format->channels);
	spec->samples = 4096;       /* Good default buffer size */

								/* Read the audio data chunk */
	*audio_buf = NULL;
	do {
		SDL_free(*audio_buf);
		*audio_buf = NULL;
		lenread = ReadChunk(src, &chunk);
		if (lenread < 0) {
			was_error = 1;
			goto done;
		}
		*audio_len = lenread;
		*audio_buf = chunk.data;
		if (chunk.magic != DATA)
			headerDiff += lenread + 2 * sizeof(Uint32);
	} while (chunk.magic != DATA);
	headerDiff += 2 * sizeof(Uint32);   /* for the data chunk and len */

	if (MS_ADPCM_encoded) {
		if (MS_ADPCM_decode(audio_buf, audio_len) < 0) {
			was_error = 1;
			goto done;
		}
	}
	if (IMA_ADPCM_encoded) {
		if (IMA_ADPCM_decode(audio_buf, audio_len) < 0) {
			was_error = 1;
			goto done;
		}
	}

	/* Don't return a buffer that isn't a multiple of samplesize */
	samplesize = ((SDL_AUDIO_BITSIZE(spec->format)) / 8) * spec->channels;
	*audio_len &= ~(samplesize - 1);

done:
	SDL_free(format);
	if (src) {
		if (freesrc) {
			SDL_RWclose(src);
		}
		else {
			/* seek to the end of the file (given by the RIFF chunk) */
			SDL_RWseek(src, wavelen - chunk.length - headerDiff, RW_SEEK_CUR);
		}
	}
	if (was_error) {
		spec = NULL;
	}
	return (spec);
}



// ------ OGG ------

int LoadSound_OGG(const unsigned char* p, long long iLen, SoundData &sd)
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

	if ((!src) || (!audio_buf) || (!audio_len))   /* sanity checks. */
		goto done;

	if (!Mix_Init(MIX_INIT_OGG))
		goto done;

	callbacks.read_func = sdl_read_func;
	callbacks.seek_func = sdl_seek_func;
	callbacks.tell_func = sdl_tell_func;
	callbacks.close_func = freesrc ?
		sdl_close_func_freesrc : sdl_close_func_nofreesrc;

	if (vorbis.ov_open_callbacks(src, &vf, NULL, 0, callbacks) != 0)
	{
		SDL_SetError("OGG bitstream is not valid Vorbis stream!");
		goto done;
	}

	must_close = 0;

	info = vorbis.ov_info(&vf, -1);

	*audio_buf = NULL;
	*audio_len = 0;
	memset(spec, '\0', sizeof(SDL_AudioSpec));

	spec->format = AUDIO_S16;
	spec->channels = info->channels;
	spec->freq = info->rate;
	spec->samples = 4096; /* buffer size */

	samples = (long)vorbis.ov_pcm_total(&vf, -1);

	*audio_len = spec->size = samples * spec->channels * 2;
	*audio_buf = (Uint8 *)SDL_malloc(*audio_len);
	if (*audio_buf == NULL)
		goto done;

	buf = *audio_buf;
	to_read = *audio_len;
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
	samplesize = ((spec->format & 0xFF) / 8)*spec->channels;
	*audio_len &= ~(samplesize - 1);

done:
	if (freesrc && src && must_close) {
		SDL_RWclose(src);
	}

	if (was_error) {
		spec = NULL;
	}

	return(spec);
}





// ------ FLAC ------

typedef struct {
	SDL_RWops* sdl_src;
	SDL_AudioSpec* sdl_spec;
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

		*bytes = SDL_RWread(data->sdl_src, buffer, sizeof(FLAC__byte),
			*bytes);

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

	if (SDL_RWseek(data->sdl_src, absolute_byte_offset, RW_SEEK_SET) < 0) {
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

	Sint64 pos = SDL_RWtell(data->sdl_src);

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

	Sint64 pos = SDL_RWtell(data->sdl_src);
	Sint64 length = SDL_RWseek(data->sdl_src, 0, RW_SEEK_END);

	if (SDL_RWseek(data->sdl_src, pos, RW_SEEK_SET) != pos || length < 0) {
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

	Sint64 pos = SDL_RWtell(data->sdl_src);
	Sint64 end = SDL_RWseek(data->sdl_src, 0, RW_SEEK_END);

	// was the original position equal to the end (a.k.a. the seek didn't move)?
	if (pos == end) {
		// must be EOF
		return true;
	}
	else {
		// not EOF, return to the original position
		SDL_RWseek(data->sdl_src, pos, RW_SEEK_SET);
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

	if (data->sdl_spec->channels != 2 || data->flac_bps != 16) {
		SDL_SetError("Current FLAC support is only for 16 bit Stereo files.");
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
	}

	// check if it is the first audio frame so we can initialize the output
	// buffer
	if (frame->header.number.sample_number == 0) {
		*(data->sdl_audio_len) = data->sdl_spec->size;
		data->sdl_audio_read = 0;
		*(data->sdl_audio_buf) = SDL_malloc(*(data->sdl_audio_len));

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
		memset(data->sdl_spec, '\0', sizeof(SDL_AudioSpec));

		data->sdl_spec->format = AUDIO_S16;
		data->sdl_spec->freq = (int)(metadata->data.stream_info.sample_rate);
		data->sdl_spec->channels = (Uint8)(metadata->data.stream_info.channels);
		data->sdl_spec->samples = 8192; /* buffer size */

		total_samples = metadata->data.stream_info.total_samples;
		bps = metadata->data.stream_info.bits_per_sample;

		data->sdl_spec->size = (Uint32)(total_samples * data->sdl_spec->channels * (bps / 8));
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
int LoadSound_FLAC(const unsigned char* p, long long iLen, SoundData& dat)
{
	FLAC__StreamDecoder *decoder = 0;
	FLAC__StreamDecoderInitStatus init_status;
	int was_error = 1;
	int was_init = 0;
	Uint32 samplesize;

	// create the client data passing information
	FLAC_SDL_Data* client_data;
	client_data = (FLAC_SDL_Data *)SDL_malloc(sizeof(FLAC_SDL_Data));

	if ((!src) || (!audio_buf) || (!audio_len))   /* sanity checks. */
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

	client_data->sdl_src = src;
	client_data->sdl_spec = spec;
	client_data->sdl_audio_buf = audio_buf;
	client_data->sdl_audio_len = audio_len;

	if (!flac.FLAC__stream_decoder_process_until_end_of_stream(decoder)) {
		SDL_SetError("Unable to process FLAC file.");
		goto done;
	}

	was_error = 0;

	/* Don't return a buffer that isn't a multiple of samplesize */
	samplesize = ((spec->format & 0xFF) / 8) * spec->channels;
	*audio_len &= ~(samplesize - 1);

done:
	if (was_init && decoder) {
		flac.FLAC__stream_decoder_finish(decoder);
	}

	if (decoder) {
		flac.FLAC__stream_decoder_delete(decoder);
	}

	if (freesrc && src) {
		SDL_RWclose(src);
	}

	if (was_error) {
		spec = NULL;
	}

	return spec;
}





// ------ MP3 ------

int LoadSound_MP3(const unsigned char* p, long long iLen, SoundData& dat)
{
	// TODO
	ASSERT(0);
	return -1;
}

}