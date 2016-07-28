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

/* $Id: music.c 3278 2007-07-15 05:33:35Z slouken $ */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "SDL_endian.h"
#include "SDL_audio.h"
#include "SDL_timer.h"

#include "SDL_mixer.h"

#define SDL_SURROUND

#ifdef SDL_SURROUND
#define MAX_OUTPUT_CHANNELS 6
#else
#define MAX_OUTPUT_CHANNELS 2
#endif

#include "wavestream.h"

int volatile music_active = 1;
static int volatile music_stopped = 0;
static int music_loops = 0;
static char *music_cmd = NULL;
static Mix_Music * volatile music_playing = NULL;
static int music_volume = MIX_MAX_VOLUME;
static int music_swap8;
static int music_swap16;

struct _Mix_Music {
	Mix_MusicType type;
	union {
		WAVStream *wave;
	} data;
	Mix_Fading fading;
	int fade_step;
	int fade_steps;
	int error;
};

static int current_output_channels;
static uint16_t current_output_format;

static int ms_per_step;

static void music_internal_initialize_volume(void);
static void music_internal_volume(int volume);
static int  music_internal_play(Mix_Music *music, double position);
static int  music_internal_position(double position);
static int  music_internal_playing();
static void music_internal_halt(void);

static void (*music_finished_hook)(void) = NULL;

void Mix_HookMusicFinished(void (*music_finished)(void))
{
	SDL_LockAudio();
	music_finished_hook = music_finished;
	SDL_UnlockAudio();
}

static int music_halt_or_loop (void)
{	
	if (!music_internal_playing()) 
	{
		if (music_loops && --music_loops)
		{
			Mix_Fading current_fade = music_playing->fading;
			music_internal_play(music_playing, 0.0);
			music_playing->fading = current_fade;
		} 
		else 
		{
			music_internal_halt();
			if (music_finished_hook)
				music_finished_hook();
			
			return 0;
		}
	}
	
	return 1;
}

void music_mixer(void *udata, uint8_t *stream, int len)
{
	if ( music_playing && music_active ) {
		if ( music_playing->fading != MIX_NO_FADING ) {
			if ( music_playing->fade_step++ < music_playing->fade_steps ) {
				int volume;
				int fade_step = music_playing->fade_step;
				int fade_steps = music_playing->fade_steps;

				if ( music_playing->fading == MIX_FADING_OUT ) {
					volume = (music_volume * (fade_steps-fade_step)) / fade_steps;
				} else { 
					volume = (music_volume * fade_step) / fade_steps;
				}
				music_internal_volume(volume);
			} else {
				if ( music_playing->fading == MIX_FADING_OUT ) {
					music_internal_halt();
					if ( music_finished_hook ) {
						music_finished_hook();
					}
					return;
				}
				music_playing->fading = MIX_NO_FADING;
			}
		}
		
		if (music_halt_or_loop() == 0)
			return;
		
		
		switch (music_playing->type) {
		case MUS_WAV:
			WAVStream_PlaySome(stream, len);
			break;
		default:
			break;
		}
	}
}

int open_music(SDL_AudioSpec *mixer)
{
	int music_error;
	music_error = 0;
	if ( WAVStream_Init(mixer) < 0 ) {
		++music_error;
	}
	music_playing = NULL;
	music_stopped = 0;
	if ( music_error ) {
		return(-1);
	}
	Mix_VolumeMusic(SDL_MIX_MAXVOLUME);

	ms_per_step = (int) (((float)mixer->samples * 1000.0) / mixer->freq);

	return(0);
}

int MIX_string_equals(const char *str1, const char *str2)
{
	while ( *str1 && *str2 ) {
		if ( toupper((unsigned char)*str1) !=
		     toupper((unsigned char)*str2) )
			break;
		++str1;
		++str2;
	}
	return (!*str1 && !*str2);
}

Mix_Music *Mix_LoadMUS(const char *file)
{
	FILE *fp;
	char *ext;
	uint8_t magic[5], moremagic[9];
	Mix_Music *music;

	fp = fopen(file, "rb");
	if ( (fp == NULL) || !fread(magic, 4, 1, fp) ) {
		if ( fp != NULL ) {
			fclose(fp);
		}
		//Mix_SetError("Couldn't read from '%s'", file);
		fprintf(stderr, "Couldn't read from '%s'\n", file);
		return(NULL);
	}
	if (!fread(moremagic, 8, 1, fp)) {
		//Mix_SetError("Couldn't read from '%s'", file);
		fprintf(stderr, "Couldn't read from '%s'\n", file);
		return(NULL);
	}
	magic[4] = '\0';
	moremagic[8] = '\0';
	fclose(fp);

	ext = strrchr(file, '.');
	if ( ext ) ++ext; /* skip the dot in the extension */

	music = (Mix_Music *)malloc(sizeof(Mix_Music));
	if ( music == NULL ) {
		//Mix_SetError("Out of memory");
		fprintf(stderr, "%s\n", "Out of memory");
		return(NULL);
	}
	music->error = 0;

	if ( (ext && MIX_string_equals(ext, "WAV")) ||
	     ((strcmp((char *)magic, "RIFF") == 0) && (strcmp((char *)(moremagic+4), "WAVE") == 0)) ||
	     (strcmp((char *)magic, "FORM") == 0) ) {
		music->type = MUS_WAV;
		music->data.wave = WAVStream_LoadSong(file, (char *)magic);
		if ( music->data.wave == NULL ) {
		  	//Mix_SetError("Unable to load WAV file");
			fprintf(stderr, "%s\n", "Unable to load WAV file");
			music->error = 1;
		}
	} else 
	{
		//Mix_SetError("Unrecognized music format");
		fprintf(stderr, "%s\n", "Unrecognized music format");
		music->error = 1;
	}
	if ( music->error ) {
		free(music);
		music = NULL;
	}
	return(music);
}

void Mix_FreeMusic(Mix_Music *music)
{
	if ( music ) {
		SDL_LockAudio();
		if ( music == music_playing ) {
			while ( music->fading == MIX_FADING_OUT ) {
				SDL_UnlockAudio();
				SDL_Delay(100);
				SDL_LockAudio();
			}
			if ( music == music_playing ) {
				music_internal_halt();
			}
		}
		SDL_UnlockAudio();
		switch (music->type) {
		case MUS_WAV:
			WAVStream_FreeSong(music->data.wave);
			break;
		default:
			break;
		}
		free(music);
	}
}

Mix_MusicType Mix_GetMusicType(const Mix_Music *music)
{
	Mix_MusicType type = MUS_NONE;

	if ( music ) {
		type = music->type;
	} else {
		SDL_LockAudio();
		if ( music_playing ) {
			type = music_playing->type;
		}
		SDL_UnlockAudio();
	}
	return(type);
}

static int music_internal_play(Mix_Music *music, double position)
{
	int retval = 0;

	if ( music_playing ) {
		music_internal_halt();
	}
	music_playing = music;

	if ( music->type != MUS_MOD ) {
		music_internal_initialize_volume();
	}

	switch (music->type) {
	case MUS_WAV:
		WAVStream_Start(music->data.wave);
		break;
	default:
		//Mix_SetError("Can't play unknown music type");
		fprintf(stderr, "%s\n", "Can't play unknown music type");
		retval = -1;
		break;
	}

	if ( retval == 0 ) {
		if ( position > 0.0 ) {
			if ( music_internal_position(position) < 0 ) {
				//Mix_SetError("Position not implemented for music type");
				fprintf(stderr, "%s\n", "Position not implemented for music type");
				retval = -1;
			}
		} else {
			music_internal_position(0.0);
		}
	}

	if ( retval < 0 ) {
		music_playing = NULL;
	}
	return(retval);
}


int Mix_FadeInMusicPos(Mix_Music *music, int loops, int ms, double position)
{
	int retval;

	if ( music == NULL ) {
		//Mix_SetError("music parameter was NULL");
		fprintf(stderr, "%s\n", "music parameter was NULL");
		return(-1);
	}

	if ( ms ) {
		music->fading = MIX_FADING_IN;
	} else {
		music->fading = MIX_NO_FADING;
	}
	music->fade_step = 0;
	music->fade_steps = ms/ms_per_step;

	SDL_LockAudio();
	while ( music_playing && (music_playing->fading == MIX_FADING_OUT) ) {
		SDL_UnlockAudio();
		SDL_Delay(100);
		SDL_LockAudio();
	}
	music_active = 1;
	music_loops = loops;
	retval = music_internal_play(music, position);
	SDL_UnlockAudio();

	return(retval);
}

int Mix_FadeInMusic(Mix_Music *music, int loops, int ms)
{
	return Mix_FadeInMusicPos(music, loops, ms, 0.0);
}

int Mix_PlayMusic(Mix_Music *music, int loops)
{
	return Mix_FadeInMusicPos(music, loops, 0, 0.0);
}

int music_internal_position(double position)
{
	int retval = 0;

	switch (music_playing->type) {
	default:
		retval = -1;
		break;
	}
	return(retval);
}

int Mix_SetMusicPosition(double position)
{
	int retval;

	SDL_LockAudio();
	if ( music_playing ) {
		retval = music_internal_position(position);
		if ( retval < 0 ) {
			//Mix_SetError("Position not implemented for music type");
			fprintf(stderr, "%s\n", "Position not implemented for music type");
		}
	} else {
		//Mix_SetError("Music isn't playing");
		fprintf(stderr, "%s\n", "Music isn't playing");
		retval = -1;
	}
	SDL_UnlockAudio();

	return(retval);
}

static void music_internal_initialize_volume(void)
{
	if ( music_playing->fading == MIX_FADING_IN ) {
		music_internal_volume(0);
	} else {
		music_internal_volume(music_volume);
	}
}

static void music_internal_volume(int volume)
{
	switch (music_playing->type) {
	case MUS_WAV:
		WAVStream_SetVolume(volume);
		break;
	default:
		break;
	}
}

int Mix_VolumeMusic(int volume)
{
	int prev_volume;

	prev_volume = music_volume;
	if ( volume < 0 ) {
		return prev_volume;
	}
	if ( volume > SDL_MIX_MAXVOLUME ) {
		volume = SDL_MIX_MAXVOLUME;
	}
	music_volume = volume;
	SDL_LockAudio();
	if ( music_playing ) {
		music_internal_volume(music_volume);
	}
	SDL_UnlockAudio();
	return(prev_volume);
}

static void music_internal_halt(void)
{
	switch (music_playing->type) {
	case MUS_WAV:
		WAVStream_Stop();
		break;
	default:
		return;
	}
	music_playing->fading = MIX_NO_FADING;
	music_playing = NULL;
}

int Mix_HaltMusic(void)
{
	SDL_LockAudio();
	if ( music_playing ) {
		music_internal_halt();
	}
	SDL_UnlockAudio();

	return(0);
}

int Mix_FadeOutMusic(int ms)
{
	int retval = 0;

	if (ms <= 0) { 
		Mix_HaltMusic();
		return 1;
	}

	SDL_LockAudio();
	if ( music_playing) {
                int fade_steps = (ms + ms_per_step - 1)/ms_per_step;
                if ( music_playing->fading == MIX_NO_FADING ) {
	        	music_playing->fade_step = 0;
                } else {
                        int step;
                        int old_fade_steps = music_playing->fade_steps;
                        if ( music_playing->fading == MIX_FADING_OUT ) {
                                step = music_playing->fade_step;
                        } else {
                                step = old_fade_steps
                                        - music_playing->fade_step + 1;
                        }
                        music_playing->fade_step = (step * fade_steps)
                                / old_fade_steps;
                }
		music_playing->fading = MIX_FADING_OUT;
		music_playing->fade_steps = fade_steps;
		retval = 1;
	}
	SDL_UnlockAudio();

	return(retval);
}

Mix_Fading Mix_FadingMusic(void)
{
	Mix_Fading fading = MIX_NO_FADING;

	SDL_LockAudio();
	if ( music_playing ) {
		fading = music_playing->fading;
	}
	SDL_UnlockAudio();

	return(fading);
}

void Mix_PauseMusic(void)
{
	music_active = 0;
}

void Mix_ResumeMusic(void)
{
	music_active = 1;
}

void Mix_RewindMusic(void)
{
	Mix_SetMusicPosition(0.0);
}

int Mix_PausedMusic(void)
{
	return (music_active == 0);
}

static int music_internal_playing()
{
	int playing = 1;
	switch (music_playing->type) {
	    case MUS_WAV:
			if ( ! WAVStream_Active() ) {
				playing = 0;
			}
			break;
	    default:
			playing = 0;
			break;
	}
	return(playing);
}

int Mix_PlayingMusic(void)
{
	int playing = 0;

	SDL_LockAudio();
	if ( music_playing ) {
		playing = music_internal_playing();
	}
	SDL_UnlockAudio();

	return(playing);
}

int Mix_SetMusicCMD(const char *command)
{
	Mix_HaltMusic();
	if ( music_cmd ) {
		free(music_cmd);
		music_cmd = NULL;
	}
	if ( command ) {
		music_cmd = (char *)malloc(strlen(command)+1);
		if ( music_cmd == NULL ) {
			return(-1);
		}
		strcpy(music_cmd, command);
	}
	return(0);
}

int Mix_SetSynchroValue(int i)
{
	if ( music_playing && ! music_stopped ) {
		switch (music_playing->type) {
		default:
			return(-1);
			break;
		}
		return(-1);
	}
	return(-1);
}

int Mix_GetSynchroValue(void)
{
	if ( music_playing && ! music_stopped ) {
		switch (music_playing->type) {
		default:
			return(-1);
			break;
		}
		return(-1);
	}
	return(-1);
}

void close_music(void)
{
	Mix_HaltMusic();
}

Mix_Music *Mix_LoadMUS_RW(FILE *rw) {
	uint8_t magic[5];
	Mix_Music *music;
	int start;

	if (!rw) {
		//Mix_SetError("RWops pointer is NULL");
		fprintf(stderr, "%s\n", "RWops pointer is NULL");
		return NULL;
	}

	start = ftell(rw);
	if (fread(magic,1,4,rw)!=4) {
		//Mix_SetError("Couldn't read from RWops");
		fprintf(stderr, "%s\n", "Couldn't read from RWops");
		return NULL;
	}
	fseek(rw, start, SEEK_SET);
	magic[4]='\0';

	music=(Mix_Music *)malloc(sizeof(Mix_Music));
	if (music==NULL ) {
		//Mix_SetError("Out of memory");
		fprintf(stderr, "%s\n", "Out of memory");
		return(NULL);
	}
	music->error = 0;
	{
		//Mix_SetError("Unrecognized music format");
		fprintf(stderr, "%s\n", "Unrecognized music format");
		music->error=1;
	}
	if (music->error) {
		free(music);
		music=NULL;
	}
	return(music);
}

