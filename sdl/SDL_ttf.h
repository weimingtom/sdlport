/*
    SDL_ttf:  A companion library to SDL for working with TrueType (tm) fonts
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

/* $Id: SDL_ttf.h 2387 2006-05-11 09:03:37Z slouken $ */

/* This library is a wrapper around the excellent FreeType 2.0 library,
   available at:
	http://www.freetype.org/
*/

#pragma once

#include "SDL_endian.h"
#include "SDL_video.h"
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_TTF_MAJOR_VERSION	2
#define SDL_TTF_MINOR_VERSION	0
#define SDL_TTF_PATCHLEVEL	8

#define SDL_TTF_VERSION(X)						\
{									\
	(X)->major = SDL_TTF_MAJOR_VERSION;				\
	(X)->minor = SDL_TTF_MINOR_VERSION;				\
	(X)->patch = SDL_TTF_PATCHLEVEL;				\
}

#define TTF_MAJOR_VERSION	SDL_TTF_MAJOR_VERSION
#define TTF_MINOR_VERSION	SDL_TTF_MINOR_VERSION
#define TTF_PATCHLEVEL		SDL_TTF_PATCHLEVEL
#define TTF_VERSION(X)		SDL_TTF_VERSION(X)

#define UNICODE_BOM_NATIVE	0xFEFF
#define UNICODE_BOM_SWAPPED	0xFFFE

extern void TTF_ByteSwappedUNICODE(int swapped);

typedef struct _TTF_Font TTF_Font;

extern int TTF_Init(void);

extern TTF_Font * TTF_OpenFont(const char *file, int ptsize);
extern TTF_Font * TTF_OpenFontIndex(const char *file, int ptsize, long index);

#define TTF_STYLE_NORMAL	0x00
#define TTF_STYLE_BOLD		0x01
#define TTF_STYLE_ITALIC	0x02
#define TTF_STYLE_UNDERLINE	0x04

extern int TTF_GetFontStyle(TTF_Font *font);
extern void TTF_SetFontStyle(TTF_Font *font, int style);

extern int TTF_FontHeight(TTF_Font *font);

extern int TTF_FontAscent(TTF_Font *font);

extern int TTF_FontDescent(TTF_Font *font);

extern int TTF_FontLineSkip(TTF_Font *font);

extern long TTF_FontFaces(TTF_Font *font);

extern int TTF_FontFaceIsFixedWidth(TTF_Font *font);
extern char * TTF_FontFaceFamilyName(TTF_Font *font);
extern char * TTF_FontFaceStyleName(TTF_Font *font);

extern int TTF_GlyphMetrics(TTF_Font *font, uint16_t ch,
	int *minx, int *maxx,
    int *miny, int *maxy, int *advance);

extern int TTF_SizeText(TTF_Font *font, const char *text, int *w, int *h);
extern int TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h);
extern int TTF_SizeUNICODE(TTF_Font *font, const uint16_t *text, int *w, int *h);

extern SDL_Surface * TTF_RenderText_Solid(TTF_Font *font,
	const char *text, SDL_Color fg);
extern SDL_Surface * TTF_RenderUTF8_Solid(TTF_Font *font,
	const char *text, SDL_Color fg);
extern SDL_Surface * TTF_RenderUNICODE_Solid(TTF_Font *font,
	const uint16_t *text, SDL_Color fg);

extern SDL_Surface * TTF_RenderGlyph_Solid(TTF_Font *font,
	uint16_t ch, SDL_Color fg);

extern SDL_Surface * TTF_RenderText_Shaded(TTF_Font *font,
	const char *text, SDL_Color fg, SDL_Color bg);
extern SDL_Surface * TTF_RenderUTF8_Shaded(TTF_Font *font,
	const char *text, SDL_Color fg, SDL_Color bg);
extern SDL_Surface * TTF_RenderUNICODE_Shaded(TTF_Font *font,
	const uint16_t *text, SDL_Color fg, SDL_Color bg);

extern SDL_Surface * TTF_RenderGlyph_Shaded(TTF_Font *font,
	uint16_t ch, SDL_Color fg, SDL_Color bg);

extern SDL_Surface * TTF_RenderText_Blended(TTF_Font *font,
	const char *text, SDL_Color fg);
extern SDL_Surface * TTF_RenderUTF8_Blended(TTF_Font *font,
	const char *text, SDL_Color fg);
extern SDL_Surface * TTF_RenderUNICODE_Blended(TTF_Font *font,
	const uint16_t *text, SDL_Color fg);

extern SDL_Surface * TTF_RenderGlyph_Blended(TTF_Font *font,
	uint16_t ch, SDL_Color fg);

#define TTF_RenderText(font, text, fg, bg)	\
	TTF_RenderText_Shaded(font, text, fg, bg)
#define TTF_RenderUTF8(font, text, fg, bg)	\
	TTF_RenderUTF8_Shaded(font, text, fg, bg)
#define TTF_RenderUNICODE(font, text, fg, bg)	\
	TTF_RenderUNICODE_Shaded(font, text, fg, bg)

extern void TTF_CloseFont(TTF_Font *font);

extern void TTF_Quit(void);

extern int TTF_WasInit(void);

#ifdef __cplusplus
}
#endif

