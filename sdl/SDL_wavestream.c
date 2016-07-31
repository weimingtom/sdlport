/*
    SDL_mixer:  An audio mixer library based on the SDL library
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* $Id: wavestream.c 2356 2006-05-09 08:02:35Z slouken $ */

/* This file supports streaming WAV files, without volume adjustment */

#include <stdlib.h>
#include <string.h>

#include "SDL_endian.h"
#include "SDL_audio.h"
//#include "SDL_mutex.h"
//#include "SDL_rwops.h"

#include "SDL_mixer.h"
//#include "wavestream.h"

#define RIFF		0x46464952		
#define WAVE		0x45564157		
#define FACT		0x74636166		
#define LIST		0x5453494c		
#define FMT			0x20746D66		
#define DATA		0x61746164		
#define PCM_CODE	1
#define ADPCM_CODE	2
#define WAVE_MONO	1
#define WAVE_STEREO	2

typedef struct WaveFMT {
	uint16_t	encoding;	
	uint16_t	channels;		
	uint32_t	frequency;		
	uint32_t	byterate;		
	uint16_t	blockalign;		
	uint16_t	bitspersample;	
} WaveFMT;

typedef struct Chunk {
	uint32_t magic;
	uint32_t length;
	uint8_t *data;			
} Chunk;

#define FORM		0x4d524f46		
#define AIFF		0x46464941		
#define SSND		0x444e5353		
#define COMM		0x4d4d4f43		

static WAVStream *music = NULL;

static SDL_AudioSpec mixer;
static int wavestream_volume = MIX_MAX_VOLUME;

static FILE *LoadWAVStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop);
static FILE *LoadAIFFStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop);

int WAVStream_Init(SDL_AudioSpec *mixerfmt)
{
	mixer = *mixerfmt;
	return(0);
}

void WAVStream_SetVolume(int volume)
{
	wavestream_volume = volume;
}

WAVStream *WAVStream_LoadSong(const char *file, const char *magic)
{
	WAVStream *wave;
	SDL_AudioSpec wavespec;

	if ( ! mixer.format ) {
		//Mix_SetError("WAV music output not started");
		fprintf(stderr, "%s\n", "WAV music output not started");
		return(NULL);
	}
	wave = (WAVStream *)malloc(sizeof *wave);
	if ( wave ) {
		memset(wave, 0, (sizeof *wave));
		if ( strcmp(magic, "RIFF") == 0 ) {
			wave->wavefp = LoadWAVStream(file, &wavespec,
					&wave->start, &wave->stop);
		} else
		if ( strcmp(magic, "FORM") == 0 ) {
			wave->wavefp = LoadAIFFStream(file, &wavespec,
					&wave->start, &wave->stop);
		} else {
			//Mix_SetError("Unknown WAVE format");
			fprintf(stderr, "%s\n", "Unknown WAVE format");
		}
		if ( wave->wavefp == NULL ) {
			free(wave);
			return(NULL);
		}
		//FIXME:
		//SDL_BuildAudioCVT(&wave->cvt,
		//	wavespec.format, wavespec.channels, wavespec.freq,
		//	mixer.format, mixer.channels, mixer.freq);
	}
	return(wave);
}

/* Start playback of a given WAV stream */
void WAVStream_Start(WAVStream *wave)
{
	clearerr(wave->wavefp);
	fseek(wave->wavefp, wave->start, SEEK_SET);
	music = wave;
}

void WAVStream_PlaySome(uint8_t *stream, int len)
{
	long pos;

	if ( music && ((pos=ftell(music->wavefp)) < music->stop) ) {
		uint8_t *data;
		if ( (music->stop - pos) < len ) {
			len = (music->stop - pos);
		}
		data = (uint8_t *)malloc(sizeof(uint8_t) * len);
		if (data)
		{		
			fread(data, len, 1, music->wavefp);
			SDL_MixAudio(stream, data, len, wavestream_volume);
			free(data);
		}	
	}
}

void WAVStream_Stop(void)
{
	music = NULL;
}

void WAVStream_FreeSong(WAVStream *wave)
{
	if ( wave ) {
		if ( wave->wavefp ) {
			fclose(wave->wavefp);
		}
		//if ( wave->cvt.buf ) {
		//	free(wave->cvt.buf);
		//}
		free(wave);
	}
}

/* Return non-zero if a stream is currently playing */
int WAVStream_Active(void)
{
	int active;

	active = 0;
	if ( music && (ftell(music->wavefp) < music->stop) ) {
		active = 1;
	}
	return(active);
}
















static uint16_t SDL_Swap16(uint16_t x) {
	return((x<<8)|(x>>8));
}
static uint32_t SDL_Swap32(uint32_t x) {
	return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}
static uint64_t SDL_Swap64(uint64_t x)
{
	uint32_t hi, lo;

	lo = (uint32_t)(x&0xFFFFFFFF);
	x >>= 32;
	hi = (uint32_t)(x&0xFFFFFFFF);
	x = SDL_Swap32(lo);
	x <<= 32;
	x |= SDL_Swap32(hi);
	return(x);
}

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SDL_SwapLE16(X)	(X)
#define SDL_SwapLE32(X)	(X)
#define SDL_SwapLE64(X)	(X)
#define SDL_SwapBE16(X)	SDL_Swap16(X)
#define SDL_SwapBE32(X)	SDL_Swap32(X)
#define SDL_SwapBE64(X)	SDL_Swap64(X)
#else
#define SDL_SwapLE16(X)	SDL_Swap16(X)
#define SDL_SwapLE32(X)	SDL_Swap32(X)
#define SDL_SwapLE64(X)	SDL_Swap64(X)
#define SDL_SwapBE16(X)	(X)
#define SDL_SwapBE32(X)	(X)
#define SDL_SwapBE64(X)	(X)
#endif

static uint32_t SDL_ReadLE32(FILE *src)
{
	uint32_t value;

	fread(&value, (sizeof value), 1, src);
	return SDL_SwapLE32(value);
}

static uint16_t SDL_ReadLE16(FILE *src)
{
	uint16_t value;

	fread(&value, (sizeof value), 1, src);
	return SDL_SwapLE16(value);
}
static uint32_t SDL_ReadBE32 (FILE *src)
{
	uint32_t value;

	fread(&value, (sizeof value), 1, src);
	return SDL_SwapBE32(value);
}
static uint16_t SDL_ReadBE16 (FILE *src)
{
	uint16_t value;

	fread(&value, (sizeof value), 1, src);
	return SDL_SwapBE16(value);
}






static int ReadChunk(FILE *src, Chunk *chunk, int read_data)
{
	chunk->magic	= SDL_ReadLE32(src);
	chunk->length	= SDL_ReadLE32(src);
	if ( read_data ) {
		chunk->data = (uint8_t *)malloc(chunk->length);
		if ( chunk->data == NULL ) {
			//Mix_SetError("Out of memory");
			fprintf(stderr, "%s\n", "Out of memory");
			return(-1);
		}
		if ( fread(chunk->data, chunk->length, 1, src) != 1 ) {
			//Mix_SetError("Couldn't read chunk");
			fprintf(stderr, "%s\n", "Couldn't read chunk");
			free(chunk->data);
			return(-1);
		}
	} else {
		fseek(src, chunk->length, SEEK_CUR);
	}
	return(chunk->length);
}

static FILE *LoadWAVStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop)
{
	int was_error;
	FILE *wavefp;
	FILE *src;
	Chunk chunk;
	int lenread;

	/* WAV magic header */
	uint32_t RIFFchunk;
	uint32_t wavelen;
	uint32_t WAVEmagic;

	/* FMT chunk */
	WaveFMT *format = NULL;

	/* Make sure we are passed a valid data source */
	was_error = 0;
	wavefp = fopen(file, "rb");
	src = NULL;
	if ( wavefp ) {
		src = wavefp; //SDL_RWFromFP(wavefp, 0);
	}
	if ( src == NULL ) {
		was_error = 1;
		goto done;
	}

	/* Check the magic header */
	RIFFchunk	= SDL_ReadLE32(src);
	wavelen		= SDL_ReadLE32(src);
	WAVEmagic	= SDL_ReadLE32(src);
	if ( (RIFFchunk != RIFF) || (WAVEmagic != WAVE) ) {
		//Mix_SetError("Unrecognized file type (not WAVE)");
		fprintf(stderr, "%s\n", "Unrecognized file type (not WAVE)");
		was_error = 1;
		goto done;
	}

	/* Read the audio data format chunk */
	chunk.data = NULL;
	do {
		/* FIXME! Add this logic to SDL_LoadWAV_RW() */
		if ( chunk.data ) {
			free(chunk.data);
		}
		lenread = ReadChunk(src, &chunk, 1);
		if ( lenread < 0 ) {
			was_error = 1;
			goto done;
		}
	} while ( (chunk.magic == FACT) || (chunk.magic == LIST) );

	/* Decode the audio data format */
	format = (WaveFMT *)chunk.data;
	if ( chunk.magic != FMT ) {
		free(chunk.data);
		//Mix_SetError("Complex WAVE files not supported");
		fprintf(stderr, "%s\n", "Complex WAVE files not supported");
		was_error = 1;
		goto done;
	}
	switch (SDL_SwapLE16(format->encoding)) {
		case PCM_CODE:
			/* We can understand this */
			break;
		default:
			//Mix_SetError("Unknown WAVE data format");
			fprintf(stderr, "%s\n", "Unknown WAVE data format");
			was_error = 1;
			goto done;
	}
	memset(spec, 0, (sizeof *spec));
	spec->freq = SDL_SwapLE32(format->frequency);
	switch (SDL_SwapLE16(format->bitspersample)) {
		case 8:
			spec->format = AUDIO_U8;
			break;
		case 16:
			spec->format = AUDIO_S16;
			break;
		default:
			//Mix_SetError("Unknown PCM data format");
			fprintf(stderr, "%s\n", "Unknown PCM data format");
			was_error = 1;
			goto done;
	}
	spec->channels = (uint8_t) SDL_SwapLE16(format->channels);
	spec->samples = 4096;		/* Good default buffer size */

	/* Set the file offset to the DATA chunk data */
	chunk.data = NULL;
	do {
		*start = ftell(src) + 2*sizeof(uint32_t);
		lenread = ReadChunk(src, &chunk, 0);
		if ( lenread < 0 ) {
			was_error = 1;
			goto done;
		}
	} while ( chunk.magic != DATA );
	*stop = ftell(src);

done:
	if ( format != NULL ) {
		free(format);
	}
	//if ( src ) {
	//	SDL_RWclose(src);
	//}
	if ( was_error ) {
		if ( wavefp ) {
			fclose(wavefp);
			wavefp = NULL;
		}
	}
	return(wavefp);
}

/* I couldn't get SANE_to_double() to work, so I stole this from libsndfile.
 * I don't pretend to fully understand it.
 */

static uint32_t SANE_to_uint32_t (uint8_t *sanebuf)
{
	/* Negative number? */
	if (sanebuf[0] & 0x80)
		return 0;

	/* Less than 1? */
	if (sanebuf[0] <= 0x3F)
		return 1;

	/* Way too big? */
	if (sanebuf[0] > 0x40)
		return 0x4000000;

	/* Still too big? */
	if (sanebuf[0] == 0x40 && sanebuf[1] > 0x1C)
		return 800000000;

	return ((sanebuf[2] << 23) | (sanebuf[3] << 15) | (sanebuf[4] << 7)
		| (sanebuf[5] >> 1)) >> (29 - sanebuf[1]);
}

static FILE *LoadAIFFStream (const char *file, SDL_AudioSpec *spec,
					long *start, long *stop)
{
	int was_error;
	int found_SSND;
	int found_COMM;
	FILE *wavefp;
	FILE *src;

	uint32_t chunk_type;
	uint32_t chunk_length;
	long next_chunk;

	/* AIFF magic header */
	uint32_t FORMchunk;
	uint32_t AIFFmagic;
	/* SSND chunk        */
	uint32_t offset;
	uint32_t blocksize;
	/* COMM format chunk */
	uint16_t channels = 0;
	uint32_t numsamples = 0;
	uint16_t samplesize = 0;
	uint8_t sane_freq[10];
	uint32_t frequency = 0;


	/* Make sure we are passed a valid data source */
	was_error = 0;
	wavefp = fopen(file, "rb");
	src = NULL;
	if ( wavefp ) {
		src = wavefp;//SDL_RWFromFP(wavefp, 0);
	}
	if ( src == NULL ) {
		was_error = 1;
		goto done;
	}

	/* Check the magic header */
	FORMchunk	= SDL_ReadLE32(src);
	chunk_length	= SDL_ReadBE32(src);
	AIFFmagic	= SDL_ReadLE32(src);
	if ( (FORMchunk != FORM) || (AIFFmagic != AIFF) ) {
		//Mix_SetError("Unrecognized file type (not AIFF)");
		fprintf(stderr, "%s\n", "Unrecognized file type (not AIFF)");
		was_error = 1;
		goto done;
	}

	/* From what I understand of the specification, chunks may appear in
         * any order, and we should just ignore unknown ones.
	 *
	 * TODO: Better sanity-checking. E.g. what happens if the AIFF file
	 *       contains compressed sound data?
         */

	found_SSND = 0;
	found_COMM = 0;

	do {
	    chunk_type		= SDL_ReadLE32(src);
	    chunk_length	= SDL_ReadBE32(src);
	    next_chunk		= ftell(src) + chunk_length;

	    /* Paranoia to avoid infinite loops */
	    if (chunk_length == 0)
		break;

            switch (chunk_type) {
		case SSND:
		    found_SSND		= 1;
		    offset		= SDL_ReadBE32(src);
		    blocksize		= SDL_ReadBE32(src);
		    *start		= ftell(src) + offset;
		    break;

		case COMM:
		    found_COMM		= 1;

		    /* Read the audio data format chunk */
		    channels		= SDL_ReadBE16(src);
		    numsamples		= SDL_ReadBE32(src);
		    samplesize		= SDL_ReadBE16(src);
		    fread(sane_freq, sizeof(sane_freq), 1, src);
		    frequency		= SANE_to_uint32_t(sane_freq);
		    break;

		default:
		    break;
	    }
	} while ((!found_SSND || !found_COMM)
		 && fseek(src, next_chunk, SEEK_SET) != -1);

	if (!found_SSND) {
	    //Mix_SetError("Bad AIFF file (no SSND chunk)");
	    fprintf(stderr, "%s\n", "Bad AIFF file (no SSND chunk)");
		was_error = 1;
	    goto done;
	}
		    
	if (!found_COMM) {
	    //Mix_SetError("Bad AIFF file (no COMM chunk)");
	    fprintf(stderr, "%s\n", "Bad AIFF file (no COMM chunk)");
		was_error = 1;
	    goto done;
	}

	*stop = *start + channels * numsamples * (samplesize / 8);

	/* Decode the audio data format */
	memset(spec, 0, (sizeof *spec));
	spec->freq = frequency;
	switch (samplesize) {
		case 8:
			spec->format = AUDIO_S8;
			break;
		case 16:
			spec->format = AUDIO_S16MSB;
			break;
		default:
			//Mix_SetError("Unknown samplesize in data format");
			fprintf(stderr, "%s\n", "Unknown samplesize in data format");
			was_error = 1;
			goto done;
	}
	spec->channels = (uint8_t) channels;
	spec->samples = 4096;		/* Good default buffer size */

done:
	//if ( src ) {
	//	SDL_RWclose(src);
	//}
	if ( was_error ) {
		if ( wavefp ) {
			fclose(wavefp);
			wavefp = NULL;
		}
	}
	return(wavefp);
}

