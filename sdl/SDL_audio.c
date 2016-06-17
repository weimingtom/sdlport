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
		     || SDL_strcmp(string, "SYS") == 0
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




