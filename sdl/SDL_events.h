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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
//for NULL
#include <stdlib.h> 
//typedef int8_t		Sint8;
//typedef uint8_t		Uint8;
//typedef int16_t		Sint16;
//typedef uint16_t	Uint16;
//typedef int32_t		Sint32;
//typedef uint32_t	Uint32;

#define SDL_INIT_EVENTTHREAD	0x01000000







typedef enum {
    SDL_NOEVENT = 0,
    SDL_QUIT,
    SDL_USEREVENT = 24,
    SDL_NUMEVENTS = 32
} SDL_EventType;

#define SDL_EVENTMASK(X)	(1<<(X))
typedef enum {
	SDL_QUITMASK		= SDL_EVENTMASK(SDL_QUIT),
} SDL_EventMask ;

#define SDL_ALLEVENTS		0xFFFFFFFF

typedef struct SDL_UserEvent {
	uint8_t type;
	int code;
	void *data1;
	void *data2;
} SDL_UserEvent;

typedef union SDL_Event {
	uint8_t type;
	SDL_UserEvent user;
} SDL_Event;

extern void SDL_PumpEvents(void);

typedef enum {
	SDL_ADDEVENT,
	SDL_PEEKEVENT,
	SDL_GETEVENT
} SDL_eventaction;

extern int SDL_PeepEvents(SDL_Event *events, int numevents, SDL_eventaction action, uint32_t mask);
extern int SDL_PollEvent(SDL_Event *event);
extern int SDL_WaitEvent(SDL_Event *event);
extern int SDL_PushEvent(SDL_Event *event);

typedef int (*SDL_EventFilter)(const SDL_Event *event);
extern void SDL_SetEventFilter(SDL_EventFilter filter);
extern SDL_EventFilter SDL_GetEventFilter(void);

#define SDL_QUERY	-1
#define SDL_IGNORE	 0
#define SDL_DISABLE	 0
#define SDL_ENABLE	 1
extern uint8_t SDL_EventState(uint8_t type, int state);

extern int SDL_StartEventLoop(uint32_t flags);
extern void SDL_StopEventLoop(void);

#ifdef __cplusplus
}
#endif
