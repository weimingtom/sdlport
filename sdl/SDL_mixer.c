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

/* $Id: mixer.c 3359 2007-07-21 06:37:58Z slouken $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "SDL_mutex.h"
#include "SDL_endian.h"
#include "SDL_timer.h"

#include "SDL_mixer.h"

//#define __MIX_INTERNAL_EFFECT__
//#include "effects_internal.h"

#define RIFF		0x46464952
#define WAVE		0x45564157	
#define FORM		0x4d524f46	
#define OGGS		0x5367674f	
#define CREA	    0x61657243		

static int audio_opened = 0;
static SDL_AudioSpec mixer;

typedef struct _Mix_effectinfo
{
	Mix_EffectFunc_t callback;
	Mix_EffectDone_t done_callback;
	void *udata;
	struct _Mix_effectinfo *next;
} effect_info;

static struct _Mix_Channel {
	Mix_Chunk *chunk;
	int playing;
	int paused;
	uint8_t *samples;
	int volume;
	int looping;
	int tag;
	uint32_t expire;
	uint32_t start_time;
	Mix_Fading fading;
	int fade_volume;
	uint32_t fade_length;
	uint32_t ticks_fade;
	effect_info *effects;
} *mix_channel = NULL;

static effect_info *posteffects = NULL;

static int num_channels;
static int reserved_channels = 0;

static void (*mix_postmix)(void *udata, uint8_t *stream, int len) = NULL;
static void *mix_postmix_data = NULL;

static void (*channel_done_callback)(int channel) = NULL;

extern int open_music(SDL_AudioSpec *mixer);
extern void close_music(void);

extern int volatile music_active;
extern void music_mixer(void *udata, uint8_t *stream, int len);
static void (*mix_music)(void *udata, uint8_t *stream, int len) = music_mixer;
static void *music_data = NULL;

static int _Mix_remove_all_effects(int channel, effect_info **e);


static SDL_AudioSpec * SDL_LoadWAV_RW(FILE *src, int freesrc, SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len);
static void SDL_FreeWAV(uint8_t *audio_buf);

static void _Mix_channel_done_playing(int channel)
{
	if (channel_done_callback) {
	    channel_done_callback(channel);
	}

	_Mix_remove_all_effects(channel, &mix_channel[channel].effects);
}


static void *Mix_DoEffects(int chan, void *snd, int len)
{
	int posteffect = (chan == MIX_CHANNEL_POST);
	effect_info *e = ((posteffect) ? posteffects : mix_channel[chan].effects);
	void *buf = snd;

	if (e != NULL) {    
		if (!posteffect) {
			buf = malloc(len);
			if (buf == NULL) {
				return(snd);
			}
		    memcpy(buf, snd, len);
		}

		for (; e != NULL; e = e->next) {
			if (e->callback != NULL) {
				e->callback(chan, buf, len, e->udata);
			}
		}
	}

	return(buf);
}

static void mix_channels(void *udata, uint8_t *stream, int len)
{
	uint8_t *mix_input;
	int i, mixable, volume = SDL_MIX_MAXVOLUME;
	uint32_t sdl_ticks;

//#if SDL_VERSION_ATLEAST(1, 3, 0)
//	memset(stream, mixer.silence, len);
//#endif

	if ( music_active || (mix_music != music_mixer) ) {
		mix_music(music_data, stream, len);
	}

	sdl_ticks = SDL_GetTicks();
	for ( i=0; i<num_channels; ++i ) {
		if( ! mix_channel[i].paused ) {
			if ( mix_channel[i].expire > 0 && mix_channel[i].expire < sdl_ticks ) {
				mix_channel[i].playing = 0;
				mix_channel[i].fading = MIX_NO_FADING;
				mix_channel[i].expire = 0;
				_Mix_channel_done_playing(i);
			} else if ( mix_channel[i].fading != MIX_NO_FADING ) {
				uint32_t ticks = sdl_ticks - mix_channel[i].ticks_fade;
				if( ticks > mix_channel[i].fade_length ) {
					if( mix_channel[i].fading == MIX_FADING_OUT ) {
						mix_channel[i].playing = 0;
						mix_channel[i].expire = 0;
						Mix_Volume(i, mix_channel[i].fade_volume); /* Restore the volume */
						_Mix_channel_done_playing(i);
					}
					mix_channel[i].fading = MIX_NO_FADING;
				} else {
					if( mix_channel[i].fading == MIX_FADING_OUT ) {
						Mix_Volume(i, (mix_channel[i].fade_volume * (mix_channel[i].fade_length-ticks))
								   / mix_channel[i].fade_length );
					} else {
						Mix_Volume(i, (mix_channel[i].fade_volume * ticks) / mix_channel[i].fade_length );
					}
				}
			}
			if ( mix_channel[i].playing > 0 ) {
				int index = 0;
				int remaining = len;
				while (mix_channel[i].playing > 0 && index < len) {
					remaining = len - index;
					volume = (mix_channel[i].volume*mix_channel[i].chunk->volume) / MIX_MAX_VOLUME;
					mixable = mix_channel[i].playing;
					if ( mixable > remaining ) {
						mixable = remaining;
					}

					mix_input = Mix_DoEffects(i, mix_channel[i].samples, mixable);
					SDL_MixAudio(stream+index,mix_input,mixable,volume);
					if (mix_input != mix_channel[i].samples)
						free(mix_input);

					mix_channel[i].samples += mixable;
					mix_channel[i].playing -= mixable;
					index += mixable;

					if (!mix_channel[i].playing && !mix_channel[i].looping) {
						_Mix_channel_done_playing(i);
					}
				}

				while ( mix_channel[i].looping && index < len ) {
					int alen = mix_channel[i].chunk->alen;
					remaining = len - index;
					if (remaining > alen) {
						remaining = alen;
					}

					mix_input = Mix_DoEffects(i, mix_channel[i].chunk->abuf, remaining);
					SDL_MixAudio(stream+index, mix_input, remaining, volume);
					if (mix_input != mix_channel[i].chunk->abuf)
						free(mix_input);

					--mix_channel[i].looping;
					mix_channel[i].samples = mix_channel[i].chunk->abuf + remaining;
					mix_channel[i].playing = mix_channel[i].chunk->alen - remaining;
					index += remaining;
				}
				if ( ! mix_channel[i].playing && mix_channel[i].looping ) {
					--mix_channel[i].looping;
					mix_channel[i].samples = mix_channel[i].chunk->abuf;
					mix_channel[i].playing = mix_channel[i].chunk->alen;
				}
			}
		}
	}

	Mix_DoEffects(MIX_CHANNEL_POST, stream, len);

	if ( mix_postmix ) {
		mix_postmix(mix_postmix_data, stream, len);
	}
}

static void PrintFormat(char *title, SDL_AudioSpec *fmt)
{
	printf("%s: %d bit %s audio (%s) at %u Hz\n", title, (fmt->format&0xFF),
			(fmt->format&0x8000) ? "signed" : "unsigned",
			(fmt->channels > 2) ? "surround" :
			(fmt->channels > 1) ? "stereo" : "mono", fmt->freq);
}

int Mix_OpenAudio(int frequency, uint16_t format, int nchannels, int chunksize)
{
	int i;
	SDL_AudioSpec desired;

	if ( audio_opened ) {
		if ( format == mixer.format && nchannels == mixer.channels ) {
	    	++audio_opened;
	    	return(0);
		}
		while ( audio_opened ) {
			Mix_CloseAudio();
		}
	}

	desired.freq = frequency;
	desired.format = format;
	desired.channels = nchannels;
	desired.samples = chunksize;
	desired.callback = mix_channels;
	desired.userdata = NULL;

	if ( SDL_OpenAudio(&desired, &mixer) < 0 ) {
		return(-1);
	}
#if 0
	PrintFormat("Audio device", &mixer);
#endif

	/* Initialize the music players */
	if ( open_music(&mixer) < 0 ) {
		SDL_CloseAudio();
		return(-1);
	}

	num_channels = MIX_CHANNELS;
	mix_channel = (struct _Mix_Channel *) malloc(num_channels * sizeof(struct _Mix_Channel));

	for ( i=0; i<num_channels; ++i ) {
		mix_channel[i].chunk = NULL;
		mix_channel[i].playing = 0;
		mix_channel[i].looping = 0;
		mix_channel[i].volume = SDL_MIX_MAXVOLUME;
		mix_channel[i].fade_volume = SDL_MIX_MAXVOLUME;
		mix_channel[i].fading = MIX_NO_FADING;
		mix_channel[i].tag = -1;
		mix_channel[i].expire = 0;
		mix_channel[i].effects = NULL;
		mix_channel[i].paused = 0;
	}
	Mix_VolumeMusic(SDL_MIX_MAXVOLUME);

	_Mix_InitEffects();

	audio_opened = 1;
	SDL_PauseAudio(0);
	return(0);
}

int Mix_AllocateChannels(int numchans)
{
	if ( numchans<0 || numchans==num_channels )
		return(num_channels);

	if ( numchans < num_channels ) {
		int i;
		for(i=numchans; i < num_channels; i++) {
			Mix_UnregisterAllEffects(i);
			Mix_HaltChannel(i);
		}
	}
	SDL_LockAudio();
	mix_channel = (struct _Mix_Channel *) realloc(mix_channel, numchans * sizeof(struct _Mix_Channel));
	if ( numchans > num_channels ) {
		int i;
		for(i=num_channels; i < numchans; i++) {
			mix_channel[i].chunk = NULL;
			mix_channel[i].playing = 0;
			mix_channel[i].looping = 0;
			mix_channel[i].volume = SDL_MIX_MAXVOLUME;
			mix_channel[i].fade_volume = SDL_MIX_MAXVOLUME;
			mix_channel[i].fading = MIX_NO_FADING;
			mix_channel[i].tag = -1;
			mix_channel[i].expire = 0;
			mix_channel[i].effects = NULL;
			mix_channel[i].paused = 0;
		}
	}
	num_channels = numchans;
	SDL_UnlockAudio();
	return(num_channels);
}

int Mix_QuerySpec(int *frequency, uint16_t *format, int *channels)
{
	if ( audio_opened ) {
		if ( frequency ) {
			*frequency = mixer.freq;
		}
		if ( format ) {
			*format = mixer.format;
		}
		if ( channels ) {
			*channels = mixer.channels;
		}
	}
	return(audio_opened);
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



Mix_Chunk *Mix_LoadWAV_RW(FILE *src, int freesrc)
{
	uint32_t magic;
	Mix_Chunk *chunk;
	SDL_AudioSpec wavespec, *loaded;
#if 0
	SDL_AudioCVT wavecvt;
#endif
	int samplesize;

	if ( ! src ) {
		//SDL_SetError("Mix_LoadWAV_RW with NULL src");
		fprintf(stderr, "%s\n", "Mix_LoadWAV_RW with NULL src");
		return(NULL);
	}

	/* Make sure audio has been opened */
	if ( ! audio_opened ) {
		//SDL_SetError("Audio device hasn't been opened");
		fprintf(stderr, "%s\n", "Audio device hasn't been opened");
		if ( freesrc && src ) {
			fclose(src);
		}
		return(NULL);
	}

	chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));
	if ( chunk == NULL ) {
		//SDL_SetError("Out of memory");
		fprintf(stderr, "%s\n", "Out of memory");
		if ( freesrc ) {
			fclose(src);
		}
		return(NULL);
	}

	magic = SDL_ReadLE32(src);
	fseek(src, -(int)sizeof(uint32_t), SEEK_CUR);

	switch (magic) {
		case WAVE:
		case RIFF:
			loaded = SDL_LoadWAV_RW(src, freesrc, &wavespec,
					(uint8_t **)&chunk->abuf, &chunk->alen);
			break;

		default:
			//SDL_SetError("Unrecognized sound file type");
			fprintf(stderr, "%s\n", "Unrecognized sound file type");
			return(0);			
	}
	if ( !loaded ) {
		free(chunk);
		return(NULL);
	}

#if 0
	PrintFormat("Audio device", &mixer);
	PrintFormat("-- Wave file", &wavespec);
#endif

#if 0
	if ( SDL_BuildAudioCVT(&wavecvt,
			wavespec.format, wavespec.channels, wavespec.freq,
			mixer.format, mixer.channels, mixer.freq) < 0 ) {
		SDL_FreeWAV(chunk->abuf);
		free(chunk);
		return(NULL);
	}
#endif
	samplesize = ((wavespec.format & 0xFF)/8)*wavespec.channels;
#if 0
	wavecvt.len = chunk->alen & ~(samplesize-1);
	wavecvt.buf = (uint8_t *)malloc(wavecvt.len*wavecvt.len_mult);
	if ( wavecvt.buf == NULL ) {
		//SDL_SetError("Out of memory");
		fprintf(stderr, "%s\n", "Out of memory");
		SDL_FreeWAV(chunk->abuf);
		free(chunk);
		return(NULL);
	}
	memcpy(wavecvt.buf, chunk->abuf, chunk->alen);
#endif
	SDL_FreeWAV(chunk->abuf);

#if 0
	if ( SDL_ConvertAudio(&wavecvt) < 0 ) {
		free(wavecvt.buf);
		free(chunk);
		return(NULL);
	}
#endif
	chunk->allocated = 1;
#if 0
	chunk->abuf = wavecvt.buf;
	chunk->alen = wavecvt.len_cvt;
#endif
	chunk->volume = MIX_MAX_VOLUME;
	return(chunk);
}

Mix_Chunk *Mix_QuickLoad_WAV(uint8_t *mem)
{
	Mix_Chunk *chunk;
	uint8_t magic[4];

	if ( ! audio_opened ) {
		fprintf(stderr, "%s\n", "Audio device hasn't been opened");
		return(NULL);
	}

	chunk = (Mix_Chunk *)calloc(1,sizeof(Mix_Chunk));
	if ( chunk == NULL ) {
		//SDL_SetError("Out of memory");
		fprintf(stderr, "%s\n", "Out of memory");
		return(NULL);
	}

	chunk->allocated = 0;
	mem += 12; 
	do {
		memcpy(magic, mem, 4);
		mem += 4;
		chunk->alen = ((mem[3]<<24)|(mem[2]<<16)|(mem[1]<<8)|(mem[0]));
		mem += 4;
		chunk->abuf = mem;
		mem += chunk->alen;
	} while ( memcmp(magic, "data", 4) != 0 );
	chunk->volume = MIX_MAX_VOLUME;

	return(chunk);
}

Mix_Chunk *Mix_QuickLoad_RAW(uint8_t *mem, uint32_t len)
{
	Mix_Chunk *chunk;

	if ( ! audio_opened ) {
		//SDL_SetError("Audio device hasn't been opened");
		fprintf(stderr, "%s\n", "Audio device hasn't been opened");
		return(NULL);
	}

	chunk = (Mix_Chunk *)malloc(sizeof(Mix_Chunk));
	if ( chunk == NULL ) {
		//SDL_SetError("Out of memory");
		fprintf(stderr, "%s\n", "Out of memory");
		return(NULL);
	}

	chunk->allocated = 0;
	chunk->alen = len;
	chunk->abuf = mem;
	chunk->volume = MIX_MAX_VOLUME;

	return(chunk);
}

void Mix_FreeChunk(Mix_Chunk *chunk)
{
	int i;

	/* Caution -- if the chunk is playing, the mixer will crash */
	if ( chunk ) {
		/* Guarantee that this chunk isn't playing */
		SDL_LockAudio();
		if ( mix_channel ) {
			for ( i=0; i<num_channels; ++i ) {
				if ( chunk == mix_channel[i].chunk ) {
					mix_channel[i].playing = 0;
				}
			}
		}
		SDL_UnlockAudio();
		/* Actually free the chunk */
		if ( chunk->allocated ) {
			free(chunk->abuf);
		}
		free(chunk);
	}
}

/* Set a function that is called after all mixing is performed.
   This can be used to provide real-time visual display of the audio stream
   or add a custom mixer filter for the stream data.
*/
void Mix_SetPostMix(void (*mix_func)
                    (void *udata, uint8_t *stream, int len), void *arg)
{
	SDL_LockAudio();
	mix_postmix_data = arg;
	mix_postmix = mix_func;
	SDL_UnlockAudio();
}


void Mix_HookMusic(void (*mix_func)(void *udata, uint8_t *stream, int len),
                                                                void *arg)
{
	SDL_LockAudio();
	if ( mix_func != NULL ) {
		music_data = arg;
		mix_music = mix_func;
	} else {
		music_data = NULL;
		mix_music = music_mixer;
	}
	SDL_UnlockAudio();
}

void *Mix_GetMusicHookData(void)
{
	return(music_data);
}

void Mix_ChannelFinished(void (*channel_finished)(int channel))
{
	SDL_LockAudio();
	channel_done_callback = channel_finished;
	SDL_UnlockAudio();
}

int Mix_ReserveChannels(int num)
{
	if (num > num_channels)
		num = num_channels;
	reserved_channels = num;
	return num;
}

static int checkchunkintegral(Mix_Chunk *chunk)
{
	int frame_width = 1;

	if ((mixer.format & 0xFF) == 16) frame_width = 2;
	frame_width *= mixer.channels;
	while (chunk->alen % frame_width) chunk->alen--;
	return chunk->alen;
}

int Mix_PlayChannelTimed(int which, Mix_Chunk *chunk, int loops, int ticks)
{
	int i;

	if ( chunk == NULL ) {
		//Mix_SetError("Tried to play a NULL chunk");
		fprintf(stderr, "%s\n", "Tried to play a NULL chunk");
		return(-1);
	}
	if ( !checkchunkintegral(chunk)) {
		//Mix_SetError("Tried to play a chunk with a bad frame");
		fprintf(stderr, "%s\n", "Tried to play a chunk with a bad frame");
		return(-1);
	}
	SDL_LockAudio();
	{
		if ( which == -1 ) {
			for ( i=reserved_channels; i<num_channels; ++i ) {
				if ( mix_channel[i].playing <= 0 )
					break;
			}
			if ( i == num_channels ) {
				//Mix_SetError("No free channels available");
				fprintf(stderr, "%s\n", "No free channels available");
				which = -1;
			} else {
				which = i;
			}
		}
		if ( which >= 0 ) {
			uint32_t sdl_ticks = SDL_GetTicks();
			if (Mix_Playing(which))
				_Mix_channel_done_playing(which);
			mix_channel[which].samples = chunk->abuf;
			mix_channel[which].playing = chunk->alen;
			mix_channel[which].looping = loops;
			mix_channel[which].chunk = chunk;
			mix_channel[which].paused = 0;
			mix_channel[which].fading = MIX_NO_FADING;
			mix_channel[which].start_time = sdl_ticks;
			mix_channel[which].expire = (ticks>0) ? (sdl_ticks + ticks) : 0;
		}
	}
	SDL_UnlockAudio();

	return(which);
}


int Mix_ExpireChannel(int which, int ticks)
{
	int status = 0;

	if ( which == -1 ) {
		int i;
		for ( i=0; i < num_channels; ++ i ) {
			status += Mix_ExpireChannel(i, ticks);
		}
	} else if ( which < num_channels ) {
		SDL_LockAudio();
		mix_channel[which].expire = (ticks>0) ? (SDL_GetTicks() + ticks) : 0;
		SDL_UnlockAudio();
		++ status;
	}
	return(status);
}

/* Fade in a sound on a channel, over ms milliseconds */
int Mix_FadeInChannelTimed(int which, Mix_Chunk *chunk, int loops, int ms, int ticks)
{
	int i;

	/* Don't play null pointers :-) */
	if ( chunk == NULL ) {
		return(-1);
	}
	if ( !checkchunkintegral(chunk)) {
		//Mix_SetError("Tried to play a chunk with a bad frame");
		fprintf(stderr, "%s\n", "Tried to play a chunk with a bad frame");
		return(-1);
	}

	/* Lock the mixer while modifying the playing channels */
	SDL_LockAudio();
	{
		/* If which is -1, play on the first free channel */
		if ( which == -1 ) {
			for ( i=reserved_channels; i<num_channels; ++i ) {
				if ( mix_channel[i].playing <= 0 )
					break;
			}
			if ( i == num_channels ) {
				which = -1;
			} else {
				which = i;
			}
		}

		/* Queue up the audio data for this channel */
		if ( which >= 0 ) {
			uint32_t sdl_ticks = SDL_GetTicks();
			if (Mix_Playing(which))
				_Mix_channel_done_playing(which);
			mix_channel[which].samples = chunk->abuf;
			mix_channel[which].playing = chunk->alen;
			mix_channel[which].looping = loops;
			mix_channel[which].chunk = chunk;
			mix_channel[which].paused = 0;
			mix_channel[which].fading = MIX_FADING_IN;
			mix_channel[which].fade_volume = mix_channel[which].volume;
			mix_channel[which].volume = 0;
			mix_channel[which].fade_length = (uint32_t)ms;
			mix_channel[which].start_time = mix_channel[which].ticks_fade = sdl_ticks;
			mix_channel[which].expire = (ticks > 0) ? (sdl_ticks+ticks) : 0;
		}
	}
	SDL_UnlockAudio();

	/* Return the channel on which the sound is being played */
	return(which);
}

/* Set volume of a particular channel */
int Mix_Volume(int which, int volume)
{
	int i;
	int prev_volume;

	if ( which == -1 ) {
		prev_volume = 0;
		for ( i=0; i<num_channels; ++i ) {
			prev_volume += Mix_Volume(i, volume);
		}
		prev_volume /= num_channels;
	} else {
		prev_volume = mix_channel[which].volume;
		if ( volume >= 0 ) {
			if ( volume > SDL_MIX_MAXVOLUME ) {
				volume = SDL_MIX_MAXVOLUME;
			}
			mix_channel[which].volume = volume;
		}
	}
	return(prev_volume);
}
/* Set volume of a particular chunk */
int Mix_VolumeChunk(Mix_Chunk *chunk, int volume)
{
	int prev_volume;

	prev_volume = chunk->volume;
	if ( volume >= 0 ) {
		if ( volume > MIX_MAX_VOLUME ) {
			volume = MIX_MAX_VOLUME;
		}
		chunk->volume = volume;
	}
	return(prev_volume);
}

/* Halt playing of a particular channel */
int Mix_HaltChannel(int which)
{
	int i;

	if ( which == -1 ) {
		for ( i=0; i<num_channels; ++i ) {
			Mix_HaltChannel(i);
		}
	} else {
		SDL_LockAudio();
		if (mix_channel[which].playing) {
			_Mix_channel_done_playing(which);
		mix_channel[which].playing = 0;
		}
		mix_channel[which].expire = 0;
		if(mix_channel[which].fading != MIX_NO_FADING) /* Restore volume */
			mix_channel[which].volume = mix_channel[which].fade_volume;
		mix_channel[which].fading = MIX_NO_FADING;
		SDL_UnlockAudio();
	}
	return(0);
}

/* Halt playing of a particular group of channels */
int Mix_HaltGroup(int tag)
{
	int i;

	for ( i=0; i<num_channels; ++i ) {
		if( mix_channel[i].tag == tag ) {
			Mix_HaltChannel(i);
		}
	}
	return(0);
}

/* Fade out a channel and then stop it automatically */
int Mix_FadeOutChannel(int which, int ms)
{
	int status;

	status = 0;
	if ( audio_opened ) {
		if ( which == -1 ) {
			int i;

			for ( i=0; i<num_channels; ++i ) {
				status += Mix_FadeOutChannel(i, ms);
			}
		} else {
			SDL_LockAudio();
			if ( mix_channel[which].playing && 
			    (mix_channel[which].volume > 0) &&
			    (mix_channel[which].fading != MIX_FADING_OUT) ) {

				mix_channel[which].fading = MIX_FADING_OUT;
				mix_channel[which].fade_volume = mix_channel[which].volume;
				mix_channel[which].fade_length = ms;
				mix_channel[which].ticks_fade = SDL_GetTicks();
				++status;
			}
			SDL_UnlockAudio();
		}
	}
	return(status);
}

/* Halt playing of a particular group of channels */
int Mix_FadeOutGroup(int tag, int ms)
{
	int i;
	int status = 0;
	for ( i=0; i<num_channels; ++i ) {
		if( mix_channel[i].tag == tag ) {
			status += Mix_FadeOutChannel(i,ms);
		}
	}
	return(status);
}

Mix_Fading Mix_FadingChannel(int which)
{
	return mix_channel[which].fading;
}

/* Check the status of a specific channel.
   If the specified mix_channel is -1, check all mix channels.
*/
int Mix_Playing(int which)
{
	int status;

	status = 0;
	if ( which == -1 ) {
		int i;

		for ( i=0; i<num_channels; ++i ) {
			if ((mix_channel[i].playing > 0) ||
				(mix_channel[i].looping > 0))
			{
				++status;
			}
		}
	} else {
		if ((mix_channel[which].playing > 0) ||
			(mix_channel[which].looping > 0))
		{
			++status;
		}
	}
	return(status);
}

/* rcg06072001 Get the chunk associated with a channel. */
Mix_Chunk *Mix_GetChunk(int channel)
{
	Mix_Chunk *retval = NULL;

	if ((channel >= 0) && (channel < num_channels)) {
		retval = mix_channel[channel].chunk;
	}

	return(retval);
}

/* Close the mixer, halting all playing audio */
void Mix_CloseAudio(void)
{
	int i;

	if ( audio_opened ) {
		if ( audio_opened == 1 ) {
			for (i = 0; i < num_channels; i++) {
				Mix_UnregisterAllEffects(i);
			}
			Mix_UnregisterAllEffects(MIX_CHANNEL_POST);
			close_music();
			Mix_HaltChannel(-1);
			_Mix_DeinitEffects();
			SDL_CloseAudio();
			free(mix_channel);
			mix_channel = NULL;
		}
		--audio_opened;
	}
}

/* Pause a particular channel (or all) */
void Mix_Pause(int which)
{
	uint32_t sdl_ticks = SDL_GetTicks();
	if ( which == -1 ) {
		int i;

		for ( i=0; i<num_channels; ++i ) {
			if ( mix_channel[i].playing > 0 ) {
				mix_channel[i].paused = sdl_ticks;
			}
		}
	} else {
		if ( mix_channel[which].playing > 0 ) {
			mix_channel[which].paused = sdl_ticks;
		}
	}
}

/* Resume a paused channel */
void Mix_Resume(int which)
{
	uint32_t sdl_ticks = SDL_GetTicks();

	SDL_LockAudio();
	if ( which == -1 ) {
		int i;

		for ( i=0; i<num_channels; ++i ) {
			if ( mix_channel[i].playing > 0 ) {
				if(mix_channel[i].expire > 0)
					mix_channel[i].expire += sdl_ticks - mix_channel[i].paused;
				mix_channel[i].paused = 0;
			}
		}
	} else {
		if ( mix_channel[which].playing > 0 ) {
			if(mix_channel[which].expire > 0)
				mix_channel[which].expire += sdl_ticks - mix_channel[which].paused;
			mix_channel[which].paused = 0;
		}
	}
	SDL_UnlockAudio();
}

int Mix_Paused(int which)
{
	if ( which > num_channels )
		return(0);
	if ( which < 0 ) {
		int status = 0;
		int i;
		for( i=0; i < num_channels; ++i ) {
			if ( mix_channel[i].paused ) {
				++ status;
			}
		}
		return(status);
	} else {
		return(mix_channel[which].paused != 0);
	}
}

/* Change the group of a channel */
int Mix_GroupChannel(int which, int tag)
{
	if ( which < 0 || which > num_channels )
		return(0);

	SDL_LockAudio();
	mix_channel[which].tag = tag;
	SDL_UnlockAudio();
	return(1);
}

/* Assign several consecutive channels to a group */
int Mix_GroupChannels(int from, int to, int tag)
{
	int status = 0;
	for( ; from <= to; ++ from ) {
		status += Mix_GroupChannel(from, tag);
	}
	return(status);
}

/* Finds the first available channel in a group of channels */
int Mix_GroupAvailable(int tag)
{
	int i;
	for( i=0; i < num_channels; i ++ ) {
		if ( ((tag == -1) || (tag == mix_channel[i].tag)) &&
		                    (mix_channel[i].playing <= 0) )
			return i;
	}
	return(-1);
}

int Mix_GroupCount(int tag)
{
	int count = 0;
	int i;
	for( i=0; i < num_channels; i ++ ) {
		if ( mix_channel[i].tag==tag || tag==-1 )
			++ count;
	}
	return(count);
}

/* Finds the "oldest" sample playing in a group of channels */
int Mix_GroupOldest(int tag)
{
	int chan = -1;
	uint32_t mintime = SDL_GetTicks();
	int i;
	for( i=0; i < num_channels; i ++ ) {
		if ( (mix_channel[i].tag==tag || tag==-1) && mix_channel[i].playing > 0
			 && mix_channel[i].start_time <= mintime ) {
			mintime = mix_channel[i].start_time;
			chan = i;
		}
	}
	return(chan);
}

/* Finds the "most recent" (i.e. last) sample playing in a group of channels */
int Mix_GroupNewer(int tag)
{
	int chan = -1;
	uint32_t maxtime = 0;
	int i;
	for( i=0; i < num_channels; i ++ ) {
		if ( (mix_channel[i].tag==tag || tag==-1) && mix_channel[i].playing > 0
			 && mix_channel[i].start_time >= maxtime ) {
			maxtime = mix_channel[i].start_time;
			chan = i;
		}
	}
	return(chan);
}



/*
 * rcg06122001 The special effects exportable API.
 *  Please see effect_*.c for internally-implemented effects, such
 *  as Mix_SetPanning().
 */

/* MAKE SURE you hold the audio lock (SDL_LockAudio()) before calling this! */
static int _Mix_register_effect(effect_info **e, Mix_EffectFunc_t f,
				Mix_EffectDone_t d, void *arg)
{
	effect_info *new_e = malloc(sizeof (effect_info));

	if (!e) {
		//Mix_SetError("Internal error");
		fprintf(stderr, "%s\n", "Internal error");
		return(0);
	}

	if (f == NULL) {
		//Mix_SetError("NULL effect callback");
		fprintf(stderr, "%s\n", "NULL effect callback");
		return(0);
	}

	if (new_e == NULL) {
		//Mix_SetError("Out of memory");
		fprintf(stderr, "%s\n", "Out of memory");
		return(0);
	}

	new_e->callback = f;
	new_e->done_callback = d;
	new_e->udata = arg;
	new_e->next = NULL;

	/* add new effect to end of linked list... */
	if (*e == NULL) {
		*e = new_e;
	} else {
		effect_info *cur = *e;
		while (1) {
			if (cur->next == NULL) {
				cur->next = new_e;
				break;
			}
			cur = cur->next;
		}
	}

	return(1);
}


/* MAKE SURE you hold the audio lock (SDL_LockAudio()) before calling this! */
static int _Mix_remove_effect(int channel, effect_info **e, Mix_EffectFunc_t f)
{
	effect_info *cur;
	effect_info *prev = NULL;
	effect_info *next = NULL;

	if (!e) {
		//Mix_SetError("Internal error");
		fprintf(stderr, "%s\n", "Internal error");
		return(0);
	}

	for (cur = *e; cur != NULL; cur = cur->next) {
		if (cur->callback == f) {
			next = cur->next;
			if (cur->done_callback != NULL) {
				cur->done_callback(channel, cur->udata);
			}
			free(cur);

			if (prev == NULL) {   /* removing first item of list? */
				*e = next;
			} else {
				prev->next = next;
			}
			return(1);
		}
		prev = cur;
	}

	//Mix_SetError("No such effect registered");
	fprintf(stderr, "%s\n", "No such effect registered");
	return(0);
}


/* MAKE SURE you hold the audio lock (SDL_LockAudio()) before calling this! */
static int _Mix_remove_all_effects(int channel, effect_info **e)
{
	effect_info *cur;
	effect_info *next;

	if (!e) {
		//Mix_SetError("Internal error");
		fprintf(stderr, "%s\n", "Internal error");
		return(0);
	}

	for (cur = *e; cur != NULL; cur = next) {
		next = cur->next;
		if (cur->done_callback != NULL) {
			cur->done_callback(channel, cur->udata);
		}
		free(cur);
	}
	*e = NULL;

	return(1);
}


int Mix_RegisterEffect(int channel, Mix_EffectFunc_t f,
			Mix_EffectDone_t d, void *arg)
{
	effect_info **e = NULL;
	int retval;

	if (channel == MIX_CHANNEL_POST) {
		e = &posteffects;
	} else {
		if ((channel < 0) || (channel >= num_channels)) {
			//Mix_SetError("Invalid channel number");
			fprintf(stderr, "%s\n", "Invalid channel number");
			return(0);
		}
		e = &mix_channel[channel].effects;
	}

	SDL_LockAudio();
	retval = _Mix_register_effect(e, f, d, arg);
	SDL_UnlockAudio();
	return(retval);
}


int Mix_UnregisterEffect(int channel, Mix_EffectFunc_t f)
{
	effect_info **e = NULL;
	int retval;

	if (channel == MIX_CHANNEL_POST) {
		e = &posteffects;
	} else {
		if ((channel < 0) || (channel >= num_channels)) {
			//Mix_SetError("Invalid channel number");
			fprintf(stderr, "%s\n", "Invalid channel number");
			return(0);
		}
		e = &mix_channel[channel].effects;
	}

	SDL_LockAudio();
	retval = _Mix_remove_effect(channel, e, f);
	SDL_UnlockAudio();
	return(retval);
}


int Mix_UnregisterAllEffects(int channel)
{
	effect_info **e = NULL;
	int retval;

	if (channel == MIX_CHANNEL_POST) {
		e = &posteffects;
	} else {
		if ((channel < 0) || (channel >= num_channels)) {
			//Mix_SetError("Invalid channel number");
			fprintf(stderr, "%s\n", "Invalid channel number");
			return(0);
		}
		e = &mix_channel[channel].effects;
	}

	SDL_LockAudio();
	retval = _Mix_remove_all_effects(channel, e);
	SDL_UnlockAudio();
	return(retval);
}






//========================
//from SDL_wav.h


#define RIFF		0x46464952
#define WAVE		0x45564157
#define FACT		0x74636166
#define LIST		0x5453494c
#define FMT		0x20746D66
#define DATA		0x61746164
#define PCM_CODE	0x0001
#define MS_ADPCM_CODE	0x0002
#define IMA_ADPCM_CODE	0x0011
#define MP3_CODE	0x0055
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


//========================
//from SDL_wav.c

static int ReadChunk(FILE *src, Chunk *chunk);



//--------------------------
//can be ignored ???

struct MS_ADPCM_decodestate {
	uint8_t hPredictor;
	uint16_t iDelta;
	int16_t iSamp1;
	int16_t iSamp2;
};
static struct MS_ADPCM_decoder {
	WaveFMT wavefmt;
	uint16_t wSamplesPerBlock;
	uint16_t wNumCoef;
	int16_t aCoeff[7][2];
	struct MS_ADPCM_decodestate state[2];
} MS_ADPCM_state;

static int InitMS_ADPCM(WaveFMT *format)
{
	uint8_t *rogue_feel;
	int i;

	MS_ADPCM_state.wavefmt.encoding = SDL_SwapLE16(format->encoding);
	MS_ADPCM_state.wavefmt.channels = SDL_SwapLE16(format->channels);
	MS_ADPCM_state.wavefmt.frequency = SDL_SwapLE32(format->frequency);
	MS_ADPCM_state.wavefmt.byterate = SDL_SwapLE32(format->byterate);
	MS_ADPCM_state.wavefmt.blockalign = SDL_SwapLE16(format->blockalign);
	MS_ADPCM_state.wavefmt.bitspersample =
					 SDL_SwapLE16(format->bitspersample);
	rogue_feel = (uint8_t *)format+sizeof(*format);
	if ( sizeof(*format) == 16 ) {
		rogue_feel += sizeof(uint16_t);
	}
	MS_ADPCM_state.wSamplesPerBlock = ((rogue_feel[1]<<8)|rogue_feel[0]);
	rogue_feel += sizeof(uint16_t);
	MS_ADPCM_state.wNumCoef = ((rogue_feel[1]<<8)|rogue_feel[0]);
	rogue_feel += sizeof(uint16_t);
	if ( MS_ADPCM_state.wNumCoef != 7 ) {
		fprintf(stderr, "%s\n", "Unknown set of MS_ADPCM coefficients");
		return(-1);
	}
	for ( i=0; i<MS_ADPCM_state.wNumCoef; ++i ) {
		MS_ADPCM_state.aCoeff[i][0] = ((rogue_feel[1]<<8)|rogue_feel[0]);
		rogue_feel += sizeof(uint16_t);
		MS_ADPCM_state.aCoeff[i][1] = ((rogue_feel[1]<<8)|rogue_feel[0]);
		rogue_feel += sizeof(uint16_t);
	}
	return(0);
}

static int32_t MS_ADPCM_nibble(struct MS_ADPCM_decodestate *state,
					uint8_t nybble, int16_t *coeff)
{
	const int32_t max_audioval = ((1<<(16-1))-1);
	const int32_t min_audioval = -(1<<(16-1));
	const int32_t adaptive[] = {
		230, 230, 230, 230, 307, 409, 512, 614,
		768, 614, 512, 409, 307, 230, 230, 230
	};
	int32_t new_sample, delta;

	new_sample = ((state->iSamp1 * coeff[0]) +
		      (state->iSamp2 * coeff[1]))/256;
	if ( nybble & 0x08 ) {
		new_sample += state->iDelta * (nybble-0x10);
	} else {
		new_sample += state->iDelta * nybble;
	}
	if ( new_sample < min_audioval ) {
		new_sample = min_audioval;
	} else
	if ( new_sample > max_audioval ) {
		new_sample = max_audioval;
	}
	delta = ((int32_t)state->iDelta * adaptive[nybble])/256;
	if ( delta < 16 ) {
		delta = 16;
	}
	state->iDelta = (uint16_t)delta;
	state->iSamp2 = state->iSamp1;
	state->iSamp1 = (int16_t)new_sample;
	return(new_sample);
}

static int MS_ADPCM_decode(uint8_t **audio_buf, uint32_t *audio_len)
{
	struct MS_ADPCM_decodestate *state[2];
	uint8_t *freeable, *encoded, *decoded;
	int32_t encoded_len, samplesleft;
	int8_t nybble, stereo;
	int16_t *coeff[2];
	int32_t new_sample;

	/* Allocate the proper sized output buffer */
	encoded_len = *audio_len;
	encoded = *audio_buf;
	freeable = *audio_buf;
	*audio_len = (encoded_len/MS_ADPCM_state.wavefmt.blockalign) * 
				MS_ADPCM_state.wSamplesPerBlock*
				MS_ADPCM_state.wavefmt.channels*sizeof(int16_t);
	*audio_buf = (uint8_t *)malloc(*audio_len);
	if ( *audio_buf == NULL ) {
		fprintf(stderr, "%s\n", "SDL_ENOMEM");
		return(-1);
	}
	decoded = *audio_buf;

	/* Get ready... Go! */
	stereo = (MS_ADPCM_state.wavefmt.channels == 2);
	state[0] = &MS_ADPCM_state.state[0];
	state[1] = &MS_ADPCM_state.state[stereo];
	while ( encoded_len >= MS_ADPCM_state.wavefmt.blockalign ) {
		/* Grab the initial information for this block */
		state[0]->hPredictor = *encoded++;
		if ( stereo ) {
			state[1]->hPredictor = *encoded++;
		}
		state[0]->iDelta = ((encoded[1]<<8)|encoded[0]);
		encoded += sizeof(int16_t);
		if ( stereo ) {
			state[1]->iDelta = ((encoded[1]<<8)|encoded[0]);
			encoded += sizeof(int16_t);
		}
		state[0]->iSamp1 = ((encoded[1]<<8)|encoded[0]);
		encoded += sizeof(int16_t);
		if ( stereo ) {
			state[1]->iSamp1 = ((encoded[1]<<8)|encoded[0]);
			encoded += sizeof(int16_t);
		}
		state[0]->iSamp2 = ((encoded[1]<<8)|encoded[0]);
		encoded += sizeof(int16_t);
		if ( stereo ) {
			state[1]->iSamp2 = ((encoded[1]<<8)|encoded[0]);
			encoded += sizeof(int16_t);
		}
		coeff[0] = MS_ADPCM_state.aCoeff[state[0]->hPredictor];
		coeff[1] = MS_ADPCM_state.aCoeff[state[1]->hPredictor];

		/* Store the two initial samples we start with */
		decoded[0] = state[0]->iSamp2&0xFF;
		decoded[1] = state[0]->iSamp2>>8;
		decoded += 2;
		if ( stereo ) {
			decoded[0] = state[1]->iSamp2&0xFF;
			decoded[1] = state[1]->iSamp2>>8;
			decoded += 2;
		}
		decoded[0] = state[0]->iSamp1&0xFF;
		decoded[1] = state[0]->iSamp1>>8;
		decoded += 2;
		if ( stereo ) {
			decoded[0] = state[1]->iSamp1&0xFF;
			decoded[1] = state[1]->iSamp1>>8;
			decoded += 2;
		}

		/* Decode and store the other samples in this block */
		samplesleft = (MS_ADPCM_state.wSamplesPerBlock-2)*
					MS_ADPCM_state.wavefmt.channels;
		while ( samplesleft > 0 ) {
			nybble = (*encoded)>>4;
			new_sample = MS_ADPCM_nibble(state[0],nybble,coeff[0]);
			decoded[0] = new_sample&0xFF;
			new_sample >>= 8;
			decoded[1] = new_sample&0xFF;
			decoded += 2;

			nybble = (*encoded)&0x0F;
			new_sample = MS_ADPCM_nibble(state[1],nybble,coeff[1]);
			decoded[0] = new_sample&0xFF;
			new_sample >>= 8;
			decoded[1] = new_sample&0xFF;
			decoded += 2;

			++encoded;
			samplesleft -= 2;
		}
		encoded_len -= MS_ADPCM_state.wavefmt.blockalign;
	}
	free(freeable);
	return(0);
}

struct IMA_ADPCM_decodestate {
	int32_t sample;
	int8_t index;
};
static struct IMA_ADPCM_decoder {
	WaveFMT wavefmt;
	uint16_t wSamplesPerBlock;
	/* * * */
	struct IMA_ADPCM_decodestate state[2];
} IMA_ADPCM_state;

static int InitIMA_ADPCM(WaveFMT *format)
{
	uint8_t *rogue_feel;

	/* Set the rogue pointer to the IMA_ADPCM specific data */
	IMA_ADPCM_state.wavefmt.encoding = SDL_SwapLE16(format->encoding);
	IMA_ADPCM_state.wavefmt.channels = SDL_SwapLE16(format->channels);
	IMA_ADPCM_state.wavefmt.frequency = SDL_SwapLE32(format->frequency);
	IMA_ADPCM_state.wavefmt.byterate = SDL_SwapLE32(format->byterate);
	IMA_ADPCM_state.wavefmt.blockalign = SDL_SwapLE16(format->blockalign);
	IMA_ADPCM_state.wavefmt.bitspersample =
					 SDL_SwapLE16(format->bitspersample);
	rogue_feel = (uint8_t *)format+sizeof(*format);
	if ( sizeof(*format) == 16 ) {
		rogue_feel += sizeof(uint16_t);
	}
	IMA_ADPCM_state.wSamplesPerBlock = ((rogue_feel[1]<<8)|rogue_feel[0]);
	return(0);
}

static int32_t IMA_ADPCM_nibble(struct IMA_ADPCM_decodestate *state,uint8_t nybble)
{
	const int32_t max_audioval = ((1<<(16-1))-1);
	const int32_t min_audioval = -(1<<(16-1));
	const int index_table[16] = {
		-1, -1, -1, -1,
		 2,  4,  6,  8,
		-1, -1, -1, -1,
		 2,  4,  6,  8
	};
	const int32_t step_table[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
		34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
		143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408,
		449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282,
		1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327,
		3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630,
		9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
		22385, 24623, 27086, 29794, 32767
	};
	int32_t delta, step;

	/* Compute difference and new sample value */
	step = step_table[state->index];
	delta = step >> 3;
	if ( nybble & 0x04 ) delta += step;
	if ( nybble & 0x02 ) delta += (step >> 1);
	if ( nybble & 0x01 ) delta += (step >> 2);
	if ( nybble & 0x08 ) delta = -delta;
	state->sample += delta;

	/* Update index value */
	state->index += index_table[nybble];
	if ( state->index > 88 ) {
		state->index = 88;
	} else
	if ( state->index < 0 ) {
		state->index = 0;
	}

	/* Clamp output sample */
	if ( state->sample > max_audioval ) {
		state->sample = max_audioval;
	} else
	if ( state->sample < min_audioval ) {
		state->sample = min_audioval;
	}
	return(state->sample);
}

/* Fill the decode buffer with a channel block of data (8 samples) */
static void Fill_IMA_ADPCM_block(uint8_t *decoded, uint8_t *encoded,
	int channel, int numchannels, struct IMA_ADPCM_decodestate *state)
{
	int i;
	int8_t nybble;
	int32_t new_sample;

	decoded += (channel * 2);
	for ( i=0; i<4; ++i ) {
		nybble = (*encoded)&0x0F;
		new_sample = IMA_ADPCM_nibble(state, nybble);
		decoded[0] = new_sample&0xFF;
		new_sample >>= 8;
		decoded[1] = new_sample&0xFF;
		decoded += 2 * numchannels;

		nybble = (*encoded)>>4;
		new_sample = IMA_ADPCM_nibble(state, nybble);
		decoded[0] = new_sample&0xFF;
		new_sample >>= 8;
		decoded[1] = new_sample&0xFF;
		decoded += 2 * numchannels;

		++encoded;
	}
}

#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))

static int IMA_ADPCM_decode(uint8_t **audio_buf, uint32_t *audio_len)
{
	struct IMA_ADPCM_decodestate *state;
	uint8_t *freeable, *encoded, *decoded;
	int32_t encoded_len, samplesleft;
	unsigned int c, channels;

	/* Check to make sure we have enough variables in the state array */
	channels = IMA_ADPCM_state.wavefmt.channels;
	if ( channels > SDL_arraysize(IMA_ADPCM_state.state) ) {
		fprintf(stderr, "IMA ADPCM decoder can only handle %d channels\n",
					SDL_arraysize(IMA_ADPCM_state.state));
		return(-1);
	}
	state = IMA_ADPCM_state.state;

	/* Allocate the proper sized output buffer */
	encoded_len = *audio_len;
	encoded = *audio_buf;
	freeable = *audio_buf;
	*audio_len = (encoded_len/IMA_ADPCM_state.wavefmt.blockalign) * 
				IMA_ADPCM_state.wSamplesPerBlock*
				IMA_ADPCM_state.wavefmt.channels*sizeof(int16_t);
	*audio_buf = (uint8_t *)malloc(*audio_len);
	if ( *audio_buf == NULL ) {
		fprintf(stderr, "%s\n", "SDL_ENOMEM");
		return(-1);
	}
	decoded = *audio_buf;

	/* Get ready... Go! */
	while ( encoded_len >= IMA_ADPCM_state.wavefmt.blockalign ) {
		/* Grab the initial information for this block */
		for ( c=0; c<channels; ++c ) {
			/* Fill the state information for this block */
			state[c].sample = ((encoded[1]<<8)|encoded[0]);
			encoded += 2;
			if ( state[c].sample & 0x8000 ) {
				state[c].sample -= 0x10000;
			}
			state[c].index = *encoded++;
			/* Reserved byte in buffer header, should be 0 */
			if ( *encoded++ != 0 ) {
				/* Uh oh, corrupt data?  Buggy code? */;
			}

			/* Store the initial sample we start with */
			decoded[0] = (uint8_t)(state[c].sample&0xFF);
			decoded[1] = (uint8_t)(state[c].sample>>8);
			decoded += 2;
		}

		/* Decode and store the other samples in this block */
		samplesleft = (IMA_ADPCM_state.wSamplesPerBlock-1)*channels;
		while ( samplesleft > 0 ) {
			for ( c=0; c<channels; ++c ) {
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
	return(0);
}

//can be ignored ???
//--------------------------

SDL_AudioSpec * SDL_LoadWAV_RW (FILE *src, int freesrc,
		SDL_AudioSpec *spec, uint8_t **audio_buf, uint32_t *audio_len)
{
	int was_error;
	Chunk chunk;
	int lenread;
	int MS_ADPCM_encoded, IMA_ADPCM_encoded;
	int samplesize;

	/* WAV magic header */
	uint32_t RIFFchunk;
	uint32_t wavelen = 0;
	uint32_t WAVEmagic;
	uint32_t headerDiff = 0;

	/* FMT chunk */
	WaveFMT *format = NULL;

	/* Make sure we are passed a valid data source */
	was_error = 0;
	if ( src == NULL ) {
		was_error = 1;
		goto done;
	}
		
	/* Check the magic header */
	RIFFchunk	= SDL_ReadLE32(src);
	wavelen		= SDL_ReadLE32(src);
	if ( wavelen == WAVE ) { /* The RIFFchunk has already been read */
		WAVEmagic = wavelen;
		wavelen   = RIFFchunk;
		RIFFchunk = RIFF;
	} else {
		WAVEmagic = SDL_ReadLE32(src);
	}
	if ( (RIFFchunk != RIFF) || (WAVEmagic != WAVE) ) {
		fprintf(stderr, "%s\n", "Unrecognized file type (not WAVE)");
		was_error = 1;
		goto done;
	}
	headerDiff += sizeof(uint32_t); /* for WAVE */

	chunk.data = NULL;
	do {
		if ( chunk.data != NULL ) {
			free(chunk.data);
			chunk.data = NULL;
		}
		lenread = ReadChunk(src, &chunk);
		if ( lenread < 0 ) {
			was_error = 1;
			goto done;
		}
		headerDiff += lenread + 2 * sizeof(uint32_t);
	} while ( (chunk.magic == FACT) || (chunk.magic == LIST) );

	format = (WaveFMT *)chunk.data;
	if ( chunk.magic != FMT ) {
		fprintf(stderr, "%s\n", "Complex WAVE files not supported");
		was_error = 1;
		goto done;
	}
	MS_ADPCM_encoded = IMA_ADPCM_encoded = 0;
	switch (SDL_SwapLE16(format->encoding)) {
		case PCM_CODE:
			break;
		case MS_ADPCM_CODE:
			if ( InitMS_ADPCM(format) < 0 ) {
				was_error = 1;
				goto done;
			}
			MS_ADPCM_encoded = 1;
			break;
		case IMA_ADPCM_CODE:
			if ( InitIMA_ADPCM(format) < 0 ) {
				was_error = 1;
				goto done;
			}
			IMA_ADPCM_encoded = 1;
			break;
		case MP3_CODE:
			fprintf(stderr, "%s\n", "MPEG Layer 3 data not supported : %d\n",
					SDL_SwapLE16(format->encoding));
			was_error = 1;
			goto done;
		default:
			fprintf(stderr, "Unknown WAVE data format: 0x%.4x\n",
					SDL_SwapLE16(format->encoding));
			was_error = 1;
			goto done;
	}
	memset(spec, 0, (sizeof *spec));
	spec->freq = SDL_SwapLE32(format->frequency);
	switch (SDL_SwapLE16(format->bitspersample)) {
		case 4:
			if ( MS_ADPCM_encoded || IMA_ADPCM_encoded ) {
				spec->format = AUDIO_S16;
			} else {
				was_error = 1;
			}
			break;
		case 8:
			spec->format = AUDIO_U8;
			break;
		case 16:
			spec->format = AUDIO_S16;
			break;
		default:
			was_error = 1;
			break;
	}
	if ( was_error ) {
		fprintf(stderr, "Unknown %d-bit PCM data format\n",
			SDL_SwapLE16(format->bitspersample));
		goto done;
	}
	spec->channels = (uint8_t)SDL_SwapLE16(format->channels);
	spec->samples = 4096;		
	
	*audio_buf = NULL;
	do {
		if ( *audio_buf != NULL ) {
			free(*audio_buf);
			*audio_buf = NULL;
		}
		lenread = ReadChunk(src, &chunk);
		if ( lenread < 0 ) {
			was_error = 1;
			goto done;
		}
		*audio_len = lenread;
		*audio_buf = chunk.data;
		if(chunk.magic != DATA) headerDiff += lenread + 2 * sizeof(uint32_t);
	} while ( chunk.magic != DATA );
	headerDiff += 2 * sizeof(uint32_t); /* for the data chunk and len */

	if ( MS_ADPCM_encoded ) {
		if ( MS_ADPCM_decode(audio_buf, audio_len) < 0 ) {
			was_error = 1;
			goto done;
		}
	}
	if ( IMA_ADPCM_encoded ) {
		if ( IMA_ADPCM_decode(audio_buf, audio_len) < 0 ) {
			was_error = 1;
			goto done;
		}
	}

	samplesize = ((spec->format & 0xFF)/8)*spec->channels;
	*audio_len &= ~(samplesize-1);

done:
	if ( format != NULL ) {
		free(format);
	}
	if ( src ) {
		if ( freesrc ) {
			fclose(src);
		} else {
			fseek(src, wavelen - chunk.length - headerDiff, SEEK_CUR);
		}
	}
	if ( was_error ) {
		spec = NULL;
	}
	return(spec);
}

void SDL_FreeWAV(uint8_t *audio_buf)
{
	if ( audio_buf != NULL ) {
		free(audio_buf);
	}
}

static int ReadChunk(FILE *src, Chunk *chunk)
{
	chunk->magic	= SDL_ReadLE32(src);
	chunk->length	= SDL_ReadLE32(src);
	chunk->data = (uint8_t *)malloc(chunk->length);
	if ( chunk->data == NULL ) {
		fprintf(stderr, "%s\n", "SDL_ENOMEM");
		return(-1);
	}
	if ( fread(chunk->data, chunk->length, 1, src) != 1 ) {
		fprintf(stderr, "%s\n", "SDL_EFREAD");
		free(chunk->data);
		chunk->data = NULL;
		return(-1);
	}
	return(chunk->length);
}










