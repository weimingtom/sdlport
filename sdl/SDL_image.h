/*
    SDL_image:  An example image loading library for use with SDL
    Copyright (C) 1997-2006 Sam Lantinga

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

//#include "SDL.h"
//#include "SDL_version.h"
//#include "begin_code.h"
#include <stdio.h>
#include "SDL_video.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_IMAGE_MAJOR_VERSION	1
#define SDL_IMAGE_MINOR_VERSION	2
#define SDL_IMAGE_PATCHLEVEL	6

#define SDL_IMAGE_VERSION(X)						\
{									\
	(X)->major = SDL_IMAGE_MAJOR_VERSION;				\
	(X)->minor = SDL_IMAGE_MINOR_VERSION;				\
	(X)->patch = SDL_IMAGE_PATCHLEVEL;				\
}

//extern const SDL_version * IMG_Linked_Version(void);

extern SDL_Surface * IMG_LoadTyped_RW(FILE *src, int freesrc, char *type);
extern SDL_Surface * IMG_Load(const char *file);
extern SDL_Surface * IMG_Load_RW(FILE *src, int freesrc);

extern int IMG_InvertAlpha(int on);

extern int IMG_isBMP(FILE *src);

extern SDL_Surface * IMG_LoadBMP_RW(FILE *src);

//#define IMG_SetError	SDL_SetError
//#define IMG_GetError	SDL_GetError

#ifdef __cplusplus
}
#endif
