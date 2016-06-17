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

#include "SDL_video.h"

#ifdef __cplusplus
extern "C" {
#endif

//modified from sdlpixel.hpp
extern void SDL_ext_getPixel(SDL_Surface* surface, int x, int y, 
	uint8_t* colorR, uint8_t* colorG, uint8_t* colorB, uint8_t* colorA);
extern void SDL_ext_putPixel(SDL_Surface* surface, int x, int y, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB);

//modified from sdlpixel.hpp
extern uint32_t SDL_ext_Alpha32(uint32_t src, uint32_t dst, uint8_t a);
extern uint16_t SDL_ext_Alpha16(uint16_t src, uint16_t dst, uint8_t a, 
	SDL_PixelFormat *f);
extern void SDL_ext_putPixelAlpha(SDL_Surface* surface, int x, int y, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA);

//modified from sdlgraphics.hpp
extern void SDL_ext_drawPoint(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x, int y);
extern void SDL_ext_fillRectangle(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int areaX, int areaY, int areaWidth, int areaHeight);
extern void SDL_ext_drawHLine(SDL_Surface* mTarget,
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x1, int y, int x2);
extern void SDL_ext_drawVLine(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x, int y1, int y2);

//modified from sdlgraphics.hpp
extern void SDL_ext_drawLine(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x1, int y1, int x2, int y2);
extern void SDL_ext_drawRectangle(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int areaX, int areaY, int areaWidth, int areaHeight);
extern void SDL_ext_drawImage(SDL_Surface* mTarget, 
	SDL_Surface* srcSurface, 
	int srcX, int srcY, int dstX, int dstY,
    int width, int height);

#ifdef __cplusplus
}
#endif
