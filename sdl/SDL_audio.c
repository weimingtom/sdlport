/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include <pthread.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#include "SDL_timer.h"
#include "SDL_audio.h"

/*
MS VC++ 12.0 _MSC_VER = 1800 (Visual C++ 2013)
MS VC++ 11.0 _MSC_VER = 1700 (Visual C++ 2012)
MS VC++ 10.0 _MSC_VER = 1600(Visual C++ 2010)
MS VC++ 9.0 _MSC_VER = 1500(Visual C++ 2008)
MS VC++ 8.0 _MSC_VER = 1400(Visual C++ 2005)
MS VC++ 7.1 _MSC_VER = 1310
MS VC++ 7.0 _MSC_VER = 1300
MS VC++ 6.0 _MSC_VER = 1200
MS VC++ 5.0 _MSC_VER = 1100

#if _MSC_VER >= 1400
#define strcasecmp _stricmp
#else
#define strcasecmp stricmp
#endif
*/
#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define snprintf _snprintf
#endif

#define DEBUG_AUDIO 0
#define DEBUG_BUILD 0
#define SOUND_DEBUG 0

typedef struct SDL_AudioDevice SDL_AudioDevice;

#define _THIS	SDL_AudioDevice *_this
#ifndef _STATUS
#define _STATUS	SDL_status *status
#endif
struct SDL_AudioDevice {
	const char *name;
	const char *desc;
	int  (*OpenAudio)(_THIS, SDL_AudioSpec *spec);
	void (*ThreadInit)(_THIS);
	void (*WaitAudio)(_THIS);
	void (*PlayAudio)(_THIS);
	uint8_t *(*GetAudioBuf)(_THIS);
	void (*WaitDone)(_THIS);
	void (*CloseAudio)(_THIS);
	void (*LockAudio)(_THIS);
	void (*UnlockAudio)(_THIS);
	void (*SetCaption)(_THIS, const char *caption);
	SDL_AudioSpec spec;
	int enabled;
	int paused;
	int opened;
	uint8_t *fake_stream;
	pthread_mutex_t *mixer_lock;
	pthread_t *thread;
	pthread_t threadid;
	uint32_t threadid_valid;
	struct SDL_PrivateAudioData *hidden;
	void (*free)(_THIS);
};
#undef _THIS

typedef struct AudioBootStrap {
	const char *name;
	const char *desc;
	int (*available)(void);
	SDL_AudioDevice *(*create)(int devindex);
} AudioBootStrap;

//AudioBootStrap WAVEOUT_bootstrap;
//SDL_AudioDevice *current_audio;

#define _THIS	SDL_AudioDevice *this

#define NUM_BUFFERS 2

struct SDL_PrivateAudioData {
	HWAVEOUT sound;
	HANDLE audio_sem;
	uint8_t *mixbuf;
	WAVEHDR wavebuf[NUM_BUFFERS];
	int next_buffer;
};

#define sound			(this->hidden->sound)
#define audio_sem 		(this->hidden->audio_sem)
#define mixbuf			(this->hidden->mixbuf)
#define wavebuf			(this->hidden->wavebuf)
#define next_buffer		(this->hidden->next_buffer)



extern uint16_t SDL_FirstAudioFormat(uint16_t format);
extern uint16_t SDL_NextAudioFormat(void);

extern void SDL_CalculateAudioSpec(SDL_AudioSpec *spec);

extern int SDL_RunAudio(void *audiop);

//static AudioBootStrap *bootstrap[] = {
//	&WAVEOUT_bootstrap,
//	NULL
//};
static SDL_AudioDevice *current_audio = NULL;

int SDL_AudioInit(const char *driver_name);
void SDL_AudioQuit(void);








#define SDL_min(x, y)	(((x) < (y)) ? (x) : (y))
#define SDL_max(x, y)	(((x) > (y)) ? (x) : (y))
#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0]))

size_t SDL_strlcpy(char *dst, const char *src, size_t maxlen)
{
    size_t srclen = strlen(src);
    if ( maxlen > 0 ) {
        size_t len = SDL_min(srclen, maxlen-1);
        memcpy(dst, src, len);
        dst[len] = '\0';
    }
    return srclen;
}

size_t SDL_strlcat(char *dst, const char *src, size_t maxlen)
{
    size_t dstlen = strlen(dst);
    size_t srclen = strlen(src);
    if ( dstlen < maxlen ) {
        SDL_strlcpy(dst+dstlen, src, maxlen-dstlen);
    }
    return dstlen+srclen;
}










static int DIB_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void DIB_ThreadInit(_THIS);
static void DIB_WaitAudio(_THIS);
static uint8_t *DIB_GetAudioBuf(_THIS);
static void DIB_PlayAudio(_THIS);
static void DIB_WaitDone(_THIS);
static void DIB_CloseAudio(_THIS);

static int Audio_Available(void)
{
	return(1);
}

static void Audio_DeleteDevice(SDL_AudioDevice *device)
{
	free(device->hidden);
	free(device);
}

static SDL_AudioDevice *Audio_CreateDevice(int devindex)
{
	SDL_AudioDevice *this;
	this = (SDL_AudioDevice *)malloc(sizeof(SDL_AudioDevice));
	if ( this ) {
		memset(this, 0, (sizeof *this));
		this->hidden = (struct SDL_PrivateAudioData *)
				malloc((sizeof *this->hidden));
	}
	if ( (this == NULL) || (this->hidden == NULL) ) {
		fprintf(stderr, "Out of memory\n");
		if ( this ) {
			free(this);
		}
		return(0);
	}
	memset(this->hidden, 0, (sizeof *this->hidden));
	this->OpenAudio = DIB_OpenAudio;
	this->ThreadInit = DIB_ThreadInit;
	this->WaitAudio = DIB_WaitAudio;
	this->PlayAudio = DIB_PlayAudio;
	this->GetAudioBuf = DIB_GetAudioBuf;
	this->WaitDone = DIB_WaitDone;
	this->CloseAudio = DIB_CloseAudio;
	this->free = Audio_DeleteDevice;
	return this;
}

static AudioBootStrap WAVEOUT_bootstrap = {
	"waveout", "Win95/98/NT/2000 WaveOut",
	Audio_Available, Audio_CreateDevice
};

static void CALLBACK FillSound(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
						DWORD dwParam1, DWORD dwParam2)
{
	SDL_AudioDevice *this = (SDL_AudioDevice *)dwInstance;
	if ( uMsg != WOM_DONE )
		return;
	ReleaseSemaphore(audio_sem, 1, NULL);
}

static void SetMMerror(char *function, MMRESULT code)
{
	size_t len;
	char errbuf[MAXERRORLENGTH];
	snprintf(errbuf, SDL_arraysize(errbuf), "%s: ", function);
	len = strlen(errbuf);
	waveOutGetErrorText(code, errbuf+len, (UINT)(MAXERRORLENGTH-len));
	fprintf(stderr, "%s\n", errbuf);
}

static void DIB_ThreadInit(_THIS)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
}

void DIB_WaitAudio(_THIS)
{
	WaitForSingleObject(audio_sem, INFINITE);
}

uint8_t *DIB_GetAudioBuf(_THIS)
{
    uint8_t *retval;
	retval = (uint8_t *)(wavebuf[next_buffer].lpData);
	return retval;
}

void DIB_PlayAudio(_THIS)
{
#if DEBUG_BUILD
        printf("[DIB_PlayAudio] : begin\n");
#endif
	waveOutWrite(sound, &wavebuf[next_buffer], sizeof(wavebuf[0]));
	next_buffer = (next_buffer+1)%NUM_BUFFERS;
#if DEBUG_BUILD
        printf("[DIB_PlayAudio] : end\n");
#endif
}

void DIB_WaitDone(_THIS)
{
	int i, left;
	do {
		left = NUM_BUFFERS;
		for ( i=0; i<NUM_BUFFERS; ++i ) {
			if ( wavebuf[i].dwFlags & WHDR_DONE ) {
				--left;
			}
		}
		if ( left > 0 ) {
			SDL_Delay(100);
		}
	} while ( left > 0 );
}

void DIB_CloseAudio(_THIS)
{
	int i;
	if ( audio_sem ) {
		CloseHandle(audio_sem);
	}
	if ( sound ) {
		waveOutClose(sound);
	}
	for ( i=0; i<NUM_BUFFERS; ++i ) {
		if ( wavebuf[i].dwUser != 0xFFFF ) {
			waveOutUnprepareHeader(sound, &wavebuf[i],
						sizeof(wavebuf[i]));
			wavebuf[i].dwUser = 0xFFFF;
		}
	}
	if ( mixbuf != NULL ) {
		free(mixbuf);
		mixbuf = NULL;
	}
}

int DIB_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
	MMRESULT result;
	int i;
	WAVEFORMATEX waveformat;
	sound = NULL;
	audio_sem = NULL;
	for ( i = 0; i < NUM_BUFFERS; ++i )
		wavebuf[i].dwUser = 0xFFFF;
	mixbuf = NULL;
	memset(&waveformat, 0, sizeof(waveformat));
	waveformat.wFormatTag = WAVE_FORMAT_PCM;
	switch ( spec->format & 0xFF ) {
	case 8:
		spec->format = AUDIO_U8;
		waveformat.wBitsPerSample = 8;
		break;
		
	case 16:
		spec->format = AUDIO_S16;
		waveformat.wBitsPerSample = 16;
		break;
		
	default:
		fprintf(stderr, "Unsupported audio format\n");
		return(-1);
	}
	waveformat.nChannels = spec->channels;
	waveformat.nSamplesPerSec = spec->freq;
	waveformat.nBlockAlign =
		waveformat.nChannels * (waveformat.wBitsPerSample/8);
	waveformat.nAvgBytesPerSec = 
		waveformat.nSamplesPerSec * waveformat.nBlockAlign;
	if ( spec->samples < (spec->freq/4) )
		spec->samples = ((spec->freq/4)+3)&~3;
	SDL_CalculateAudioSpec(spec);
	result = waveOutOpen(&sound, WAVE_MAPPER, &waveformat,
			(DWORD_PTR)FillSound, (DWORD_PTR)this, CALLBACK_FUNCTION);
	if ( result != MMSYSERR_NOERROR ) {
		SetMMerror("waveOutOpen()", result);
		return(-1);
	}
#if SOUND_DEBUG
	{
		WAVEOUTCAPS caps;
		result = waveOutGetDevCaps((UINT)sound, &caps, sizeof(caps));
		if ( result != MMSYSERR_NOERROR ) {
			SetMMerror("waveOutGetDevCaps()", result);
			return(-1);
		}
		printf("Audio device: %s\n", caps.szPname);
	}
#endif
	audio_sem = CreateSemaphore(NULL, NUM_BUFFERS-1, NUM_BUFFERS, NULL);
	if ( audio_sem == NULL ) {
		fprintf(stderr, "Couldn't create semaphore\n");
		return(-1);
	}
	mixbuf = (uint8_t *)malloc(NUM_BUFFERS*spec->size);
	if ( mixbuf == NULL ) {
		fprintf(stderr, "Out of memory\n");
		return(-1);
	}
	for ( i = 0; i < NUM_BUFFERS; ++i ) {
		memset(&wavebuf[i], 0, sizeof(wavebuf[i]));
		wavebuf[i].lpData = (LPSTR) &mixbuf[i*spec->size];
		wavebuf[i].dwBufferLength = spec->size;
		wavebuf[i].dwFlags = WHDR_DONE;
		result = waveOutPrepareHeader(sound, &wavebuf[i],
							sizeof(wavebuf[i]));
		if ( result != MMSYSERR_NOERROR ) {
			SetMMerror("waveOutPrepareHeader()", result);
			return(-1);
		}
	}
	next_buffer = 0;
	return(0);
}


















static AudioBootStrap *bootstrap[] = {
	&WAVEOUT_bootstrap,
	NULL
};




int SDL_RunAudio(void *audiop)
{
	SDL_AudioDevice *audio = (SDL_AudioDevice *)audiop;
	uint8_t *stream;
	int    stream_len;
	void  *udata;
	void (*fill)(void *userdata,uint8_t *stream, int len);
	int    silence;
	
#if DEBUG_BUILD
        printf("[SDL_RunAudio] : Task start.....1\n");
#endif
	if ( audio->ThreadInit ) {
		audio->ThreadInit(audio);
	}
	audio->threadid = pthread_self();
	audio->threadid_valid = 1;
	fill  = audio->spec.callback;
	udata = audio->spec.userdata;
	silence = audio->spec.silence;
	stream_len = audio->spec.size;
#if DEBUG_BUILD
        printf("[SDL_RunAudio] : Task start.....2\n");
#endif
	while ( audio->enabled ) {
#if DEBUG_BUILD
        printf("[SDL_RunAudio] : Task loop.....1\n");
#endif
		stream = audio->GetAudioBuf(audio);
		if ( stream == NULL ) {
			stream = audio->fake_stream;
		}
		memset(stream, silence, stream_len);
#if DEBUG_BUILD
        printf("[SDL_RunAudio] : Task loop.....2, audio->paused == %d \n", audio->paused);
#endif
		if ( ! audio->paused ) {
			pthread_mutex_lock(audio->mixer_lock);
			(*fill)(udata, stream, stream_len);
			pthread_mutex_unlock(audio->mixer_lock);
		}
		if ( stream != audio->fake_stream ) {
			audio->PlayAudio(audio);
		}
		if ( stream == audio->fake_stream ) {
			SDL_Delay((audio->spec.samples*1000)/audio->spec.freq);
		} else {
			audio->WaitAudio(audio);
		}
	}
	if ( audio->WaitDone ) {
#if DEBUG_BUILD
        printf("[SDL_RunAudio] : Task wait done.....\n");
#endif
		audio->WaitDone(audio);
	}
#if DEBUG_BUILD
        printf("[SDL_RunAudio] : Task exiting.\n");
#endif
	return(0);
}

static void SDL_LockAudio_Default(SDL_AudioDevice *audio)
{
	if ( audio->thread && (audio->threadid_valid && !pthread_equal(pthread_self(), audio->threadid)) ) {
		return;
	}
	pthread_mutex_lock(audio->mixer_lock);
}

static void SDL_UnlockAudio_Default(SDL_AudioDevice *audio)
{
	if ( audio->thread && (audio->threadid_valid && !pthread_equal(pthread_self(), audio->threadid)) ) {
		return;
	}
	pthread_mutex_unlock(audio->mixer_lock);
}

static uint16_t SDL_ParseAudioFormat(const char *string)
{
	uint16_t format = 0;
	switch (*string) {
	    case 'U':
		++string;
		format |= 0x0000;
		break;
	    case 'S':
		++string;
		format |= 0x8000;
		break;
	    default:
		return 0;
	}
	switch (atoi(string)) {
	    case 8:
		string += 1;
		format |= 8;
		break;
	    case 16:
		string += 2;
		format |= 16;
		if ( strcmp(string, "LSB") == 0
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		     || strcmp(string, "SYS") == 0
#endif
		    ) {
			format |= 0x0000;
		}
		if ( strcmp(string, "MSB") == 0
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		     || strcmp(string, "SYS") == 0
#endif
		    ) {
			format |= 0x1000;
		}
		break;
	    default:
		return 0;
	}
	return format;
}

int SDL_AudioInit(const char *driver_name)
{
	SDL_AudioDevice *audio;
	int i = 0, idx;
	if ( current_audio != NULL ) {
		SDL_AudioQuit();
	}
	audio = NULL;
	idx = 0;
	if ( audio == NULL ) {
		if ( driver_name != NULL ) {
			for ( i=0; bootstrap[i]; ++i ) {
				if (strcasecmp(bootstrap[i]->name, driver_name) == 0) {
					if ( bootstrap[i]->available() ) {
						audio=bootstrap[i]->create(idx);
						break;
					}
				}
			}
		} else {
			for ( i=0; bootstrap[i]; ++i ) {
				if ( bootstrap[i]->available() ) {
					audio = bootstrap[i]->create(idx);
					if ( audio != NULL ) {
						break;
					}
				}
			}
		}
		if ( audio == NULL ) {
			fprintf(stderr, "No available audio device\n");
		}
	}
	current_audio = audio;
	if ( current_audio ) {
		current_audio->name = bootstrap[i]->name;
		if ( !current_audio->LockAudio && !current_audio->UnlockAudio ) {
			current_audio->LockAudio = SDL_LockAudio_Default;
			current_audio->UnlockAudio = SDL_UnlockAudio_Default;
		}
	}
	return(0);
}

char *SDL_AudioDriverName(char *namebuf, int maxlen)
{
	if ( current_audio != NULL ) {
		SDL_strlcpy(namebuf, current_audio->name, maxlen);
		return(namebuf);
	}
	return(NULL);
}


static void *run_audio(void *data)
{
	SDL_RunAudio(data);
	return NULL;
}
int SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained)
{
	SDL_AudioDevice *audio;
	//const char *env;
	int ret;
	if ( ! current_audio ) {
		if ( (SDL_AudioInit(NULL) < 0) ||
		     (current_audio == NULL) ) {
			return(-1);
		}
	}
	audio = current_audio;
	if (audio->opened) {
		fprintf(stderr, "Audio device is already opened\n");
		return(-1);
	}
	if ( desired->freq == 0 ) {
		desired->freq = 22050;
	}
	if ( desired->format == 0 ) {
		desired->format = AUDIO_S16;
	}
	if ( desired->channels == 0 ) {
		desired->channels = 2;
	}
	switch ( desired->channels ) {
	case 1:
	case 2:
	case 4:
	case 6:
		break;
		
	default:
		fprintf(stderr, "1 (mono) and 2 (stereo) channels supported\n");
		return(-1);
	}
	if ( desired->samples == 0 ) {
		int samples = (desired->freq / 1000) * 46;
		int power2 = 1;
		while ( power2 < samples ) {
			power2 *= 2;
		}
		desired->samples = power2;
	}
	if ( desired->callback == NULL ) {
		fprintf(stderr, "SDL_OpenAudio() passed a NULL callback\n");
		return(-1);
	}

#if SDL_THREADS_DISABLED

#else
	audio->mixer_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	ret = pthread_mutex_init(audio->mixer_lock, NULL);
	if ( ret != 0 ) {
		free(audio->mixer_lock);
		fprintf(stderr, "Couldn't create mixer lock\n");
		SDL_CloseAudio();
		return(-1);
	}
#endif
	SDL_CalculateAudioSpec(desired);
	memcpy(&audio->spec, desired, sizeof(audio->spec));
	audio->enabled = 1;
	audio->paused  = 1;
	audio->opened = audio->OpenAudio(audio, &audio->spec)+1;
	if ( ! audio->opened ) {
		SDL_CloseAudio();
		return(-1);
	}
	if ( audio->spec.samples != desired->samples ) {
		desired->samples = audio->spec.samples;
		SDL_CalculateAudioSpec(desired);
	}
	audio->fake_stream = malloc(audio->spec.size);
	if ( audio->fake_stream == NULL ) {
		SDL_CloseAudio();
		fprintf(stderr, "Out of memory\n");
		return(-1);
	}
	if ( obtained != NULL ) {
		memcpy(obtained, &audio->spec, sizeof(audio->spec));
	} else if ( desired->freq != audio->spec.freq ||
                    desired->format != audio->spec.format ||
	            desired->channels != audio->spec.channels ) {
#if DEBUG_BUILD
	printf("SDL_OpenAudio : audio is not desired...\n");
#endif
			SDL_CloseAudio();
			return(-1);
	}
	switch (audio->opened) {
	case 1:
		//audio->thread = SDL_CreateThread(SDL_RunAudio, audio);
		//if ( audio->thread == NULL ) {
		//	SDL_CloseAudio();
		//	fprintf(stderr, "Couldn't create audio thread\n");
		//	return(-1);
		//}
		{
			int err;
			audio->thread = (pthread_t *)malloc(sizeof(pthread_t));
			err = pthread_create(audio->thread, NULL, run_audio, audio);
			if (err!=0)
			{
				free(audio->thread);
				audio->thread = NULL;
				SDL_CloseAudio();
				fprintf(stderr, "Couldn't create audio thread\n");
				return(-1);
			}
		}
		break;
		
	default:
		break;
	}
	return(0);
}

SDL_audiostatus SDL_GetAudioStatus(void)
{
	SDL_AudioDevice *audio = current_audio;
	SDL_audiostatus status;
	status = SDL_AUDIO_STOPPED;
	if ( audio && audio->enabled ) {
		if ( audio->paused ) {
			status = SDL_AUDIO_PAUSED;
		} else {
			status = SDL_AUDIO_PLAYING;
		}
	}
	return(status);
}

void SDL_PauseAudio (int pause_on)
{
	SDL_AudioDevice *audio = current_audio;
	if ( audio ) {
		audio->paused = pause_on;
	}
}

void SDL_LockAudio (void)
{
	SDL_AudioDevice *audio = current_audio;
	if ( audio && audio->LockAudio ) {
		audio->LockAudio(audio);
	}
}

void SDL_UnlockAudio (void)
{
	SDL_AudioDevice *audio = current_audio;
	if ( audio && audio->UnlockAudio ) {
		audio->UnlockAudio(audio);
	}
}

void SDL_CloseAudio (void)
{
	SDL_AudioQuit();
}

void SDL_AudioQuit(void)
{
	SDL_AudioDevice *audio = current_audio;
	if ( audio ) {
		audio->enabled = 0;
#if DEBUG_AUDIO
	printf("SDL_AudioQuit() 1...\n");
#endif
		if ( audio->thread != NULL ) {
			//SDL_WaitThread(audio->thread, NULL);
			pthread_join(*(audio->thread), NULL);
			free(audio->thread);
			audio->thread = NULL;
		}
#if DEBUG_AUDIO
	printf("SDL_AudioQuit() 2...\n");
#endif
		if ( audio->mixer_lock != NULL ) {
			pthread_mutex_destroy(audio->mixer_lock);
			free(audio->mixer_lock);
			audio->mixer_lock = NULL;
		}
#if DEBUG_AUDIO
	printf("SDL_AudioQuit() 3...\n");
#endif
		if ( audio->fake_stream != NULL ) {
			free(audio->fake_stream);
		}
#if DEBUG_AUDIO
	printf("SDL_AudioQuit() 4...\n");
#endif
#if DEBUG_AUDIO
	printf("SDL_AudioQuit() 5...\n");
#endif
		if ( audio->opened ) {
			audio->CloseAudio(audio);
			audio->opened = 0;
		}
#if DEBUG_AUDIO
	printf("SDL_AudioQuit() 6...\n");
#endif
		audio->free(audio);
		current_audio = NULL;
#if DEBUG_AUDIO
	printf("SDL_AudioQuit() 7...\n");
#endif
	}
}

#define NUM_FORMATS	6
static int format_idx;
static int format_idx_sub;
static uint16_t format_list[NUM_FORMATS][NUM_FORMATS] = {
 { AUDIO_U8, AUDIO_S8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB },
 { AUDIO_S8, AUDIO_U8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB },
 { AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8 },
 { AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8 },
 { AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U8, AUDIO_S8 },
 { AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U8, AUDIO_S8 },
};

uint16_t SDL_FirstAudioFormat(uint16_t format)
{
	for ( format_idx=0; format_idx < NUM_FORMATS; ++format_idx ) {
		if ( format_list[format_idx][0] == format ) {
			break;
		}
	}
	format_idx_sub = 0;
	return(SDL_NextAudioFormat());
}

uint16_t SDL_NextAudioFormat(void)
{
	if ( (format_idx == NUM_FORMATS) || (format_idx_sub == NUM_FORMATS) ) {
		return(0);
	}
	return(format_list[format_idx][format_idx_sub++]);
}

void SDL_CalculateAudioSpec(SDL_AudioSpec *spec)
{
	switch (spec->format) {
	case AUDIO_U8:
		spec->silence = 0x80;
		break;
		
	default:
		spec->silence = 0x00;
		break;
	}
	spec->size = (spec->format&0xFF)/8;
	spec->size *= spec->channels;
	spec->size *= spec->samples;
}

void SDL_Audio_SetCaption(const char *caption)
{
	if ((current_audio) && (current_audio->SetCaption)) {
		current_audio->SetCaption(current_audio, caption);
	}
}


//============================
//from SDL_mixer.c
//1.2.15


static const uint8_t mix8[] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
  0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
  0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A,
  0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
  0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
  0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B,
  0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
  0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71,
  0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C,
  0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92,
  0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
  0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
  0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3,
  0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE,
  0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
  0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4,
  0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
  0xEB, 0xEC, 0xED, 0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5,
  0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE,
  0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE
};

#define ADJUST_VOLUME(s, v)	(s = (s*v)/SDL_MIX_MAXVOLUME)
#define ADJUST_VOLUME_U8(s, v)	(s = (((s-128)*v)/SDL_MIX_MAXVOLUME)+128)

void SDL_MixAudio (uint8_t *dst, const uint8_t *src, uint32_t len, int volume)
{
	uint16_t format;

	if ( volume == 0 ) {
		return;
	}
	if ( current_audio ) {
		format = current_audio->spec.format;
	} else {
  		format = AUDIO_S16;
	}
	switch (format) {

		case AUDIO_U8: {
			uint8_t src_sample;

			while ( len-- ) {
				src_sample = *src;
				ADJUST_VOLUME_U8(src_sample, volume);
				*dst = mix8[*dst+src_sample];
				++dst;
				++src;
			}
		}
		break;

		case AUDIO_S8: {
			{
			int8_t *dst8, *src8;
			int8_t src_sample;
			int dst_sample;
			const int max_audioval = ((1<<(8-1))-1);
			const int min_audioval = -(1<<(8-1));

			src8 = (int8_t *)src;
			dst8 = (int8_t *)dst;
			while ( len-- ) {
				src_sample = *src8;
				ADJUST_VOLUME(src_sample, volume);
				dst_sample = *dst8 + src_sample;
				if ( dst_sample > max_audioval ) {
					*dst8 = max_audioval;
				} else
				if ( dst_sample < min_audioval ) {
					*dst8 = min_audioval;
				} else {
					*dst8 = dst_sample;
				}
				++dst8;
				++src8;
			}
			}
		}
		break;

		case AUDIO_S16LSB: {
			{
			int16_t src1, src2;
			int dst_sample;
			const int max_audioval = ((1<<(16-1))-1);
			const int min_audioval = -(1<<(16-1));

			len /= 2;
			while ( len-- ) {
				src1 = ((src[1])<<8|src[0]);
				ADJUST_VOLUME(src1, volume);
				src2 = ((dst[1])<<8|dst[0]);
				src += 2;
				dst_sample = src1+src2;
				if ( dst_sample > max_audioval ) {
					dst_sample = max_audioval;
				} else
				if ( dst_sample < min_audioval ) {
					dst_sample = min_audioval;
				}
				dst[0] = dst_sample&0xFF;
				dst_sample >>= 8;
				dst[1] = dst_sample&0xFF;
				dst += 2;
			}
			}
		}
		break;

		case AUDIO_S16MSB: {
			int16_t src1, src2;
			int dst_sample;
			const int max_audioval = ((1<<(16-1))-1);
			const int min_audioval = -(1<<(16-1));

			len /= 2;
			while ( len-- ) {
				src1 = ((src[0])<<8|src[1]);
				ADJUST_VOLUME(src1, volume);
				src2 = ((dst[0])<<8|dst[1]);
				src += 2;
				dst_sample = src1+src2;
				if ( dst_sample > max_audioval ) {
					dst_sample = max_audioval;
				} else
				if ( dst_sample < min_audioval ) {
					dst_sample = min_audioval;
				}
				dst[1] = dst_sample&0xFF;
				dst_sample >>= 8;
				dst[0] = dst_sample&0xFF;
				dst += 2;
			}
		}
		break;

		default:
			//SDL_SetError("SDL_MixAudio(): unknown audio format");
			fprintf(stderr, "%s\n", "SDL_MixAudio(): unknown audio format");
			return;
	}
}



