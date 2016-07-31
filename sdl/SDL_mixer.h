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

/* $Id: SDL_mixer.h 3278 2007-07-15 05:33:35Z slouken $ */

#pragma once

//#include "SDL_types.h"
//#include "SDL_rwops.h"
//#include "SDL_audio.h"
//#include "SDL_endian.h"
//#include "SDL_version.h"
//#include "begin_code.h"
#include <stdio.h>
#include "SDL_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_MIXER_MAJOR_VERSION	1
#define SDL_MIXER_MINOR_VERSION	2
#define SDL_MIXER_PATCHLEVEL	8

#define SDL_MIXER_VERSION(X)						\
{									\
	(X)->major = SDL_MIXER_MAJOR_VERSION;				\
	(X)->minor = SDL_MIXER_MINOR_VERSION;				\
	(X)->patch = SDL_MIXER_PATCHLEVEL;				\
}

#define MIX_MAJOR_VERSION	SDL_MIXER_MAJOR_VERSION
#define MIX_MINOR_VERSION	SDL_MIXER_MINOR_VERSION
#define MIX_PATCHLEVEL		SDL_MIXER_PATCHLEVEL
#define MIX_VERSION(X)		SDL_MIXER_VERSION(X)

//extern const SDL_version * Mix_Linked_Version(void);

#ifndef MIX_CHANNELS
#define MIX_CHANNELS	8
#endif

#define MIX_DEFAULT_FREQUENCY	22050
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define MIX_DEFAULT_FORMAT	AUDIO_S16LSB
#else
#define MIX_DEFAULT_FORMAT	AUDIO_S16MSB
#endif
#define MIX_DEFAULT_CHANNELS	2
#define MIX_MAX_VOLUME		128

typedef struct Mix_Chunk {
	int allocated;
	uint8_t *abuf;
	uint32_t alen;
	uint8_t volume;		
} Mix_Chunk;

typedef enum {
	MIX_NO_FADING,
	MIX_FADING_OUT,
	MIX_FADING_IN
} Mix_Fading;

typedef enum {
	MUS_NONE,
	MUS_CMD,
	MUS_WAV,
	MUS_MOD,
	MUS_MID,
	MUS_OGG,
	MUS_MP3,
	MUS_MP3_MAD
} Mix_MusicType;

typedef struct _Mix_Music Mix_Music;

extern int Mix_OpenAudio(int frequency, uint16_t format, int channels,
							int chunksize);

extern int Mix_AllocateChannels(int numchans);

extern int Mix_QuerySpec(int *frequency, uint16_t *format, int *channels);

extern Mix_Chunk * Mix_LoadWAV_RW(FILE *src, int freesrc);
#define Mix_LoadWAV(file)	Mix_LoadWAV_RW(fopen(file, "rb"), 1)
extern Mix_Music * Mix_LoadMUS(const char *file);

extern Mix_Music * Mix_LoadMUS_RW(FILE *rw);

extern Mix_Chunk * Mix_QuickLoad_WAV(uint8_t *mem);

extern Mix_Chunk * Mix_QuickLoad_RAW(uint8_t *mem, uint32_t len);

extern void Mix_FreeChunk(Mix_Chunk *chunk);
extern void Mix_FreeMusic(Mix_Music *music);

extern Mix_MusicType Mix_GetMusicType(const Mix_Music *music);

extern void Mix_SetPostMix(void (*mix_func)
                             (void *udata, uint8_t *stream, int len), void *arg);

extern void Mix_HookMusic(void (*mix_func)
                          (void *udata, uint8_t *stream, int len), void *arg);

extern void Mix_HookMusicFinished(void (*music_finished)(void));

extern void * Mix_GetMusicHookData(void);

extern void Mix_ChannelFinished(void (*channel_finished)(int channel));

#define MIX_CHANNEL_POST  -2

typedef void (*Mix_EffectFunc_t)(int chan, void *stream, int len, void *udata);
typedef void (*Mix_EffectDone_t)(int chan, void *udata);

extern int Mix_RegisterEffect(int chan, Mix_EffectFunc_t f,
					Mix_EffectDone_t d, void *arg);
extern int Mix_UnregisterEffect(int channel, Mix_EffectFunc_t f);
extern int Mix_UnregisterAllEffects(int channel);
#define MIX_EFFECTSMAXSPEED  "MIX_EFFECTSMAXSPEED"
extern int Mix_SetPanning(int channel, uint8_t left, uint8_t right);
extern int Mix_SetPosition(int channel, int16_t angle, uint8_t distance);
extern int Mix_SetDistance(int channel, uint8_t distance);

#if 0
extern no_parse_int Mix_SetReverb(int channel, uint8_t echo);
#endif
extern int Mix_SetReverseStereo(int channel, int flip);







extern int Mix_ReserveChannels(int num);

extern int Mix_GroupChannel(int which, int tag);
extern int Mix_GroupChannels(int from, int to, int tag);
extern int Mix_GroupAvailable(int tag);
extern int Mix_GroupCount(int tag);
extern int Mix_GroupOldest(int tag);
extern int Mix_GroupNewer(int tag);

#define Mix_PlayChannel(channel,chunk,loops) Mix_PlayChannelTimed(channel,chunk,loops,-1)
extern int Mix_PlayChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ticks);
extern int Mix_PlayMusic(Mix_Music *music, int loops);

extern int Mix_FadeInMusic(Mix_Music *music, int loops, int ms);
extern int Mix_FadeInMusicPos(Mix_Music *music, int loops, int ms, double position);
#define Mix_FadeInChannel(channel,chunk,loops,ms) Mix_FadeInChannelTimed(channel,chunk,loops,ms,-1)
extern int Mix_FadeInChannelTimed(int channel, Mix_Chunk *chunk, int loops, int ms, int ticks);

extern int Mix_Volume(int channel, int volume);
extern int Mix_VolumeChunk(Mix_Chunk *chunk, int volume);
extern int Mix_VolumeMusic(int volume);

extern int Mix_HaltChannel(int channel);
extern int Mix_HaltGroup(int tag);
extern int Mix_HaltMusic(void);

extern int Mix_ExpireChannel(int channel, int ticks);

extern int Mix_FadeOutChannel(int which, int ms);
extern int Mix_FadeOutGroup(int tag, int ms);
extern int Mix_FadeOutMusic(int ms);

extern Mix_Fading Mix_FadingMusic(void);
extern Mix_Fading Mix_FadingChannel(int which);

extern void Mix_Pause(int channel);
extern void Mix_Resume(int channel);
extern int Mix_Paused(int channel);

extern void Mix_PauseMusic(void);
extern void Mix_ResumeMusic(void);
extern void Mix_RewindMusic(void);
extern int Mix_PausedMusic(void);

extern int Mix_SetMusicPosition(double position);

extern int Mix_Playing(int channel);
extern int Mix_PlayingMusic(void);

extern int Mix_SetMusicCMD(const char *command);

extern int Mix_SetSynchroValue(int value);
extern int Mix_GetSynchroValue(void);

extern Mix_Chunk * Mix_GetChunk(int channel);

extern void Mix_CloseAudio(void);

//#define Mix_SetError	SDL_SetError
//#define Mix_GetError	SDL_GetError







//===============================
// effects_internal.h


extern int _Mix_effects_max_speed;
extern void *_Eff_volume_table;
void *_Eff_build_volume_table_u8(void);
void *_Eff_build_volume_table_s8(void);

void _Mix_InitEffects(void);
void _Mix_DeinitEffects(void);
void _Eff_PositionDeinit(void);













//===============================


//wavestream.h

/* This file supports streaming WAV files, without volume adjustment */

//#include <stdio.h>

typedef struct {
	FILE *wavefp;
	long  start;
	long  stop;
	//SDL_AudioCVT cvt;
} WAVStream;

extern int WAVStream_Init(SDL_AudioSpec *mixer);

extern void WAVStream_SetVolume(int volume);

extern WAVStream *WAVStream_LoadSong(const char *file, const char *magic);

extern void WAVStream_Start(WAVStream *wave);

extern void WAVStream_PlaySome(uint8_t *stream, int len);

extern void WAVStream_Stop(void);

extern void WAVStream_FreeSong(WAVStream *wave);

extern int WAVStream_Active(void);









#ifdef __cplusplus
}
#endif
