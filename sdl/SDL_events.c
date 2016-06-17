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

#define DEBUG_EVENTS 0
#define DEBUG_PEEP_EVENTS 0
#define USE_LOCK 1
//SDL_SYSWMEVENT

#if USE_LOCK
#include <pthread.h>
#endif

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "SDL_events.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
SDL_EventFilter SDL_EventOK = NULL;
uint8_t SDL_ProcessEvents[SDL_NUMEVENTS];
static uint32_t SDL_eventstate = 0;

#define MAXEVENTS	128
static struct {
#if USE_LOCK
	pthread_mutex_t *lock;
#endif
	int active;
	int head;
	int tail;
	SDL_Event event[MAXEVENTS];
} SDL_EventQ;

#if USE_LOCK
static struct {
	pthread_mutex_t *lock;
	int safe;
} SDL_EventLock;

static pthread_t *SDL_EventThread = NULL;
static pthread_t event_thread;
static uint32_t event_thread_valid = 0;

#endif

static void SDL_Delay(uint32_t ms)
{
	Sleep(ms);
}

void SDL_Lock_EventThread(void)
{
#if USE_LOCK
	if ( SDL_EventThread && (event_thread_valid && !pthread_equal(pthread_self(), event_thread)) ) {
		pthread_mutex_lock(SDL_EventLock.lock);
		while ( ! SDL_EventLock.safe ) {
			SDL_Delay(1);
		}
	}
#endif
}
void SDL_Unlock_EventThread(void)
{
#if USE_LOCK
	if ( SDL_EventThread && (event_thread_valid && !pthread_equal(pthread_self(), event_thread)) ) {
		pthread_mutex_unlock(SDL_EventLock.lock);
	}
#endif
}

static int SDL_GobbleEvents(void *unused)
{
#if USE_LOCK
	event_thread = pthread_self();
	event_thread_valid = 1;
#endif
	while ( SDL_EventQ.active ) {
#if USE_LOCK
		SDL_EventLock.safe = 1;
#endif
		SDL_Delay(1);
#if USE_LOCK
		pthread_mutex_lock(SDL_EventLock.lock);
		SDL_EventLock.safe = 0;
		pthread_mutex_unlock(SDL_EventLock.lock);
#endif
	}
#if USE_LOCK
	event_thread_valid = 0;
#endif
	return(0);
}

static void *gobble_events(void *data)
{
	SDL_GobbleEvents(data);
	return NULL;
}

static int SDL_StartEventThread(uint32_t flags)
{
#if USE_LOCK
	int ret;
	SDL_EventThread = NULL;
	memset(&SDL_EventLock, 0, sizeof(SDL_EventLock));
	SDL_EventQ.lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
	ret = pthread_mutex_init(SDL_EventQ.lock, NULL);
	if ( ret != 0 ) {
		free(SDL_EventQ.lock);
		SDL_EventQ.lock = NULL;
		return(-1);
	}
#endif
	SDL_EventQ.active = 1;
#if USE_LOCK
	if ( (flags&SDL_INIT_EVENTTHREAD) == SDL_INIT_EVENTTHREAD ) {
		//SDL_EventLock.lock = SDL_CreateMutex();
		SDL_EventLock.lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
		ret = pthread_mutex_init(SDL_EventLock.lock, NULL);
		if ( ret != 0 ) {
			free(SDL_EventLock.lock);
			SDL_EventLock.lock = NULL;
			return(-1);
		}
		SDL_EventLock.safe = 0;
		//SDL_EventThread = SDL_CreateThread(SDL_GobbleEvents, NULL);
		//if ( SDL_EventThread == NULL ) {
		//	return(-1);
		//}
		{
			int err;
			SDL_EventThread = (pthread_t *)malloc(sizeof(pthread_t));
			err = pthread_create(SDL_EventThread, NULL, gobble_events, NULL);
			if (err!=0)
			{
				free(SDL_EventThread);
				SDL_EventThread = NULL;
				return(-1);
			}
		}
	} else {
		event_thread_valid = 0;
	}
#endif
	return(0);
}

static void SDL_StopEventThread(void)
{
	SDL_EventQ.active = 0;
#if USE_LOCK
	if ( SDL_EventThread ) {
		//SDL_WaitThread(SDL_EventThread, NULL);
		pthread_join(*SDL_EventThread, NULL);
		free(SDL_EventThread);
		SDL_EventThread = NULL;
		if (SDL_EventLock.lock)
		{
			pthread_mutex_destroy(SDL_EventLock.lock);
			free(SDL_EventLock.lock);
		}
		SDL_EventLock.lock = NULL;
	}
	if (SDL_EventQ.lock)
	{
		pthread_mutex_destroy(SDL_EventQ.lock);
		free(SDL_EventQ.lock);
	}
	SDL_EventQ.lock = NULL;
#endif
}

void SDL_StopEventLoop(void)
{
	SDL_StopEventThread();
	SDL_EventQ.head = 0;
	SDL_EventQ.tail = 0;
}

int SDL_StartEventLoop(uint32_t flags)
{
	int retcode;
#if DEBUG_EVENTS
	printf("SDL_StartEventLoop() begin...\n");
#endif
#if USE_LOCK
	SDL_EventThread = NULL;
	SDL_EventQ.lock = NULL;
#endif
	SDL_StopEventLoop();
	SDL_EventOK = NULL;
	memset(SDL_ProcessEvents,SDL_ENABLE,sizeof(SDL_ProcessEvents));
	SDL_eventstate = ~0;
	retcode = 0;
	if ( retcode < 0 ) {
		return(-1);
	}
	if ( SDL_StartEventThread(flags) < 0 ) {
		SDL_StopEventLoop();
		return(-1);
	}
	return(0);
}

static int SDL_AddEvent(SDL_Event *event)
{
	int tail, added;
#if DEBUG_EVENTS
	printf("SDL_PeepEvents->SDL_AddEvent addevent...\n");
#endif
	tail = (SDL_EventQ.tail+1)%MAXEVENTS;
	if ( tail == SDL_EventQ.head ) {
		added = 0;
	} else {
		SDL_EventQ.event[SDL_EventQ.tail] = *event;
		SDL_EventQ.tail = tail;
		added = 1;
	}
	return(added);
}

static int SDL_CutEvent(int spot)
{
	if ( spot == SDL_EventQ.head ) {
		SDL_EventQ.head = (SDL_EventQ.head+1)%MAXEVENTS;
		return(SDL_EventQ.head);
	} 
	else if ( (spot+1)%MAXEVENTS == SDL_EventQ.tail ) {
		SDL_EventQ.tail = spot;
		return(SDL_EventQ.tail);
	} 
	else {
		int here, next;
		if ( --SDL_EventQ.tail < 0 ) {
			SDL_EventQ.tail = MAXEVENTS-1;
		}
		for ( here=spot; here != SDL_EventQ.tail; here = next ) {
			next = (here+1)%MAXEVENTS;
			SDL_EventQ.event[here] = SDL_EventQ.event[next];
		}
		return(spot);
	}
}

int SDL_PeepEvents(SDL_Event *events, int numevents, SDL_eventaction action, uint32_t mask)
{
	int i, used;
#if DEBUG_PEEP_EVENTS
	printf("SDL_PeepEvents : begin...\n");
#endif
	if ( ! SDL_EventQ.active ) {
		return(-1);
	}
#if DEBUG_PEEP_EVENTS
	printf("SDL_PeepEvents : SDL_EventQ is active...\n");
#endif
	used = 0;
#if USE_LOCK
	if ( pthread_mutex_lock(SDL_EventQ.lock) == 0 ) {
#endif
		if ( action == SDL_ADDEVENT ) {
			for ( i=0; i<numevents; ++i ) {
				used += SDL_AddEvent(&events[i]);
			}
		} else {
			SDL_Event tmpevent;
			int spot;
			if ( events == NULL ) {
				action = SDL_PEEKEVENT;
				numevents = 1;
				events = &tmpevent;
			}
			spot = SDL_EventQ.head;
			while ((used < numevents)&&(spot != SDL_EventQ.tail)) {
				if ( mask & SDL_EVENTMASK(SDL_EventQ.event[spot].type) ) {
					events[used++] = SDL_EventQ.event[spot];
					if ( action == SDL_GETEVENT ) {
						spot = SDL_CutEvent(spot);
					} else {
						spot = (spot+1)%MAXEVENTS;
					}
				} else {
					spot = (spot+1)%MAXEVENTS;
				}
			}
		}
#if USE_LOCK
		pthread_mutex_unlock(SDL_EventQ.lock);
	} else {
		fprintf(stderr, "Couldn't lock event queue");
		used = -1;
	}
#endif
#if DEBUG_PEEP_EVENTS
	printf("SDL_PeepEvents : used == %d...\n", used);
#endif
	return(used);
}

void SDL_PumpEvents(void)
{
#if USE_LOCK
	if ( !SDL_EventThread ) {
		
	}
#endif
}

int SDL_PollEvent (SDL_Event *event)
{
	SDL_PumpEvents();
	if ( SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_ALLEVENTS) <= 0 )
		return 0;
	return 1;
}

int SDL_WaitEvent (SDL_Event *event)
{
#if DEBUG_PEEP_EVENTS
	printf("SDL_WaitEvent begin...\n");
#endif
	while ( 1 ) {
#if DEBUG_PEEP_EVENTS
	printf("SDL_WaitEvent SDL_PumpEvents begin...\n");
#endif
		SDL_PumpEvents();
#if DEBUG_PEEP_EVENTS
	printf("SDL_WaitEvent SDL_PumpEvents end...\n");
#endif
		switch (SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_ALLEVENTS)) {
		case -1: 
#if DEBUG_PEEP_EVENTS
	printf("SDL_WaitEvent return 0...\n");
#endif
			return 0;
			
		case 1: 
#if DEBUG_PEEP_EVENTS
	printf("SDL_WaitEvent return 1...\n");
#endif
			return 1;
			
		case 0: 
#if DEBUG_PEEP_EVENTS
	printf("SDL_WaitEvent delay...\n");
#endif
			SDL_Delay(10);
		}
	}
#if DEBUG_PEEP_EVENTS
	printf("SDL_WaitEvent bottom...\n");
#endif
}

int SDL_PushEvent(SDL_Event *event)
{
#if DEBUG_EVENTS
	printf("SDL_PushEvent...\n");
#endif
	if ( SDL_PeepEvents(event, 1, SDL_ADDEVENT, 0) <= 0 )
		return -1;
	return 0;
}

void SDL_SetEventFilter (SDL_EventFilter filter)
{
	SDL_Event bitbucket;
	SDL_EventOK = filter;
	while ( SDL_PollEvent(&bitbucket) > 0 )
		;
}

SDL_EventFilter SDL_GetEventFilter(void)
{
	return(SDL_EventOK);
}

uint8_t SDL_EventState (uint8_t type, int state)
{
	SDL_Event bitbucket;
	uint8_t current_state;
	if ( type == 0xFF ) {
		current_state = SDL_IGNORE;
		for ( type=0; type<SDL_NUMEVENTS; ++type ) {
			if ( SDL_ProcessEvents[type] != SDL_IGNORE ) {
				current_state = SDL_ENABLE;
			}
			SDL_ProcessEvents[type] = state;
			if ( state == SDL_ENABLE ) {
				SDL_eventstate |= (0x00000001 << (type));
			} else {
				SDL_eventstate &= ~(0x00000001 << (type));
			}
		}
		while ( SDL_PollEvent(&bitbucket) > 0 )
			;
		return(current_state);
	}
	current_state = SDL_ProcessEvents[type];
	switch (state) {
		case SDL_IGNORE:
		case SDL_ENABLE:
			/* Set state and discard pending events */
			SDL_ProcessEvents[type] = state;
			if ( state == SDL_ENABLE ) {
				SDL_eventstate |= (0x00000001 << (type));
			} else {
				SDL_eventstate &= ~(0x00000001 << (type));
			}
			while ( SDL_PollEvent(&bitbucket) > 0 )
				;
			break;
		
		default:
			break;
	}
	return(current_state);
}
