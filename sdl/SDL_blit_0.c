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

#include <memory.h>
#if defined(_DEBUG)
#include <stdio.h>
#endif
#include "SDL_video.h"

static void BlitBto1(SDL_BlitInfo *info)
{
	int c;
	int width, height;
	uint8_t *src, *map, *dst;
	int srcskip, dstskip;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = info->d_pixels;
	dstskip = info->d_skip;
	map = info->table;
	srcskip += width-(width+7)/8;

	if ( map ) {
		while ( height-- ) {
		        uint8_t byte = 0, bit;
	    		for ( c=0; c<width; ++c ) {
				if ( (c&7) == 0 ) {
					byte = *src++;
				}
				bit = (byte&0x80)>>7;
				if ( 1 ) {
				  *dst = map[bit];
				}
				dst++;
				byte <<= 1;
			}
			src += srcskip;
			dst += dstskip;
		}
	} else {
		while ( height-- ) {
		        uint8_t byte = 0, bit;
	    		for ( c=0; c<width; ++c ) {
				if ( (c&7) == 0 ) {
					byte = *src++;
				}
				bit = (byte&0x80)>>7;
				if ( 1 ) {
				  *dst = bit;
				}
				dst++;
				byte <<= 1;
			}
			src += srcskip;
			dst += dstskip;
		}
	}
}
static void BlitBto2(SDL_BlitInfo *info)
{
	int c;
	int width, height;
	uint8_t *src;
	uint16_t *map, *dst;
	int srcskip, dstskip;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = (uint16_t *)info->d_pixels;
	dstskip = info->d_skip/2;
	map = (uint16_t *)info->table;
	srcskip += width-(width+7)/8;

	while ( height-- ) {
	        uint8_t byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( 1 ) {
				*dst = map[bit];
			}
			byte <<= 1;
			dst++;
		}
		src += srcskip;
		dst += dstskip;
	}
}
static void BlitBto3(SDL_BlitInfo *info)
{
	int c, o;
	int width, height;
	uint8_t *src, *map, *dst;
	int srcskip, dstskip;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = info->d_pixels;
	dstskip = info->d_skip;
	map = info->table;
	srcskip += width-(width+7)/8;

	while ( height-- ) {
	        uint8_t byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( 1 ) {
				o = bit * 4;
				dst[0] = map[o++];
				dst[1] = map[o++];
				dst[2] = map[o++];
			}
			byte <<= 1;
			dst += 3;
		}
		src += srcskip;
		dst += dstskip;
	}
}
static void BlitBto4(SDL_BlitInfo *info)
{
	int width, height;
	uint8_t *src;
	uint32_t *map, *dst;
	int srcskip, dstskip;
	int c;

	/* Set up some basic variables */
	width = info->d_width;
	height = info->d_height;
	src = info->s_pixels;
	srcskip = info->s_skip;
	dst = (uint32_t *)info->d_pixels;
	dstskip = info->d_skip/4;
	map = (uint32_t *)info->table;
	srcskip += width-(width+7)/8;

	while ( height-- ) {
	        uint8_t byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( 1 ) {
				*dst = map[bit];
			}
			byte <<= 1;
			dst++;
		}
		src += srcskip;
		dst += dstskip;
	}
}

static void BlitBto1Key(SDL_BlitInfo *info)
{
        int width = info->d_width;
	int height = info->d_height;
	uint8_t *src = info->s_pixels;
	uint8_t *dst = info->d_pixels;
	int srcskip = info->s_skip;
	int dstskip = info->d_skip;
	uint32_t ckey = info->src->colorkey;
	uint8_t *palmap = info->table;
	int c;

	/* Set up some basic variables */
	srcskip += width-(width+7)/8;

	if ( palmap ) {
		while ( height-- ) {
		        uint8_t  byte = 0, bit;
	    		for ( c=0; c<width; ++c ) {
				if ( (c&7) == 0 ) {
					byte = *src++;
				}
				bit = (byte&0x80)>>7;
				if ( bit != ckey ) {
				  *dst = palmap[bit];
				}
				dst++;
				byte <<= 1;
			}
			src += srcskip;
			dst += dstskip;
		}
	} else {
		while ( height-- ) {
		        uint8_t  byte = 0, bit;
	    		for ( c=0; c<width; ++c ) {
				if ( (c&7) == 0 ) {
					byte = *src++;
				}
				bit = (byte&0x80)>>7;
				if ( bit != ckey ) {
				  *dst = bit;
				}
				dst++;
				byte <<= 1;
			}
			src += srcskip;
			dst += dstskip;
		}
	}
}

static void BlitBto2Key(SDL_BlitInfo *info)
{
        int width = info->d_width;
	int height = info->d_height;
	uint8_t *src = info->s_pixels;
	uint16_t *dstp = (uint16_t *)info->d_pixels;
	int srcskip = info->s_skip;
	int dstskip = info->d_skip;
	uint32_t ckey = info->src->colorkey;
	uint8_t *palmap = info->table;
	int c;

	/* Set up some basic variables */
	srcskip += width-(width+7)/8;
	dstskip /= 2;

	while ( height-- ) {
	        uint8_t byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( bit != ckey ) {
				*dstp=((uint16_t *)palmap)[bit];
			}
			byte <<= 1;
			dstp++;
		}
		src += srcskip;
		dstp += dstskip;
	}
}

static void BlitBto3Key(SDL_BlitInfo *info)
{
        int width = info->d_width;
	int height = info->d_height;
	uint8_t *src = info->s_pixels;
	uint8_t *dst = info->d_pixels;
	int srcskip = info->s_skip;
	int dstskip = info->d_skip;
	uint32_t ckey = info->src->colorkey;
	uint8_t *palmap = info->table;
	int c;

	/* Set up some basic variables */
	srcskip += width-(width+7)/8;

	while ( height-- ) {
	        uint8_t  byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( bit != ckey ) {
				memcpy(dst, &palmap[bit*4], 3);
			}
			byte <<= 1;
			dst += 3;
		}
		src += srcskip;
		dst += dstskip;
	}
}

static void BlitBto4Key(SDL_BlitInfo *info)
{
        int width = info->d_width;
	int height = info->d_height;
	uint8_t *src = info->s_pixels;
	uint32_t *dstp = (uint32_t *)info->d_pixels;
	int srcskip = info->s_skip;
	int dstskip = info->d_skip;
	uint32_t ckey = info->src->colorkey;
	uint8_t *palmap = info->table;
	int c;

	/* Set up some basic variables */
	srcskip += width-(width+7)/8;
	dstskip /= 4;

	while ( height-- ) {
	        uint8_t byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( bit != ckey ) {
				*dstp=((uint32_t *)palmap)[bit];
			}
			byte <<= 1;
			dstp++;
		}
		src += srcskip;
		dstp += dstskip;
	}
}

static void BlitBtoNAlpha(SDL_BlitInfo *info)
{
        int width = info->d_width;
	int height = info->d_height;
	uint8_t *src = info->s_pixels;
	uint8_t *dst = info->d_pixels;
	int srcskip = info->s_skip;
	int dstskip = info->d_skip;
	const SDL_Color *srcpal	= info->src->palette->colors;
	SDL_PixelFormat *dstfmt = info->dst;
	int  dstbpp;
	int c;
	const int A = info->src->alpha;

	/* Set up some basic variables */
	dstbpp = dstfmt->BytesPerPixel;
	srcskip += width-(width+7)/8;

	while ( height-- ) {
	        uint8_t byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( 1 ) {
			        uint32_t pixel;
			        unsigned sR, sG, sB;
				unsigned dR, dG, dB;
				sR = srcpal[bit].r;
				sG = srcpal[bit].g;
				sB = srcpal[bit].b;
				DISEMBLE_RGB(dst, dstbpp, dstfmt,
							pixel, dR, dG, dB);
				ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
			  	ASSEMBLE_RGB(dst, dstbpp, dstfmt, dR, dG, dB);
			}
			byte <<= 1;
			dst += dstbpp;
		}
		src += srcskip;
		dst += dstskip;
	}
}

static void BlitBtoNAlphaKey(SDL_BlitInfo *info)
{
        int width = info->d_width;
	int height = info->d_height;
	uint8_t *src = info->s_pixels;
	uint8_t *dst = info->d_pixels;
	int srcskip = info->s_skip;
	int dstskip = info->d_skip;
	SDL_PixelFormat *srcfmt = info->src;
	SDL_PixelFormat *dstfmt = info->dst;
	const SDL_Color *srcpal	= srcfmt->palette->colors;
	int dstbpp;
	int c;
	const int A = srcfmt->alpha;
	uint32_t ckey = srcfmt->colorkey;

	/* Set up some basic variables */
	dstbpp = dstfmt->BytesPerPixel;
	srcskip += width-(width+7)/8;

	while ( height-- ) {
	        uint8_t  byte = 0, bit;
	    	for ( c=0; c<width; ++c ) {
			if ( (c&7) == 0 ) {
				byte = *src++;
			}
			bit = (byte&0x80)>>7;
			if ( bit != ckey ) {
			        int sR, sG, sB;
				int dR, dG, dB;
				uint32_t pixel;
				sR = srcpal[bit].r;
				sG = srcpal[bit].g;
				sB = srcpal[bit].b;
				DISEMBLE_RGB(dst, dstbpp, dstfmt,
							pixel, dR, dG, dB);
				ALPHA_BLEND(sR, sG, sB, A, dR, dG, dB);
			  	ASSEMBLE_RGB(dst, dstbpp, dstfmt, dR, dG, dB);
			}
			byte <<= 1;
			dst += dstbpp;
		}
		src += srcskip;
		dst += dstskip;
	}
}

static SDL_loblit bitmap_blit[] = {
	NULL, BlitBto1, BlitBto2, BlitBto3, BlitBto4
};

static SDL_loblit colorkey_blit[] = {
    NULL, BlitBto1Key, BlitBto2Key, BlitBto3Key, BlitBto4Key
};

SDL_loblit SDL_CalculateBlit0(SDL_Surface *surface, int blit_index)
{
	int which;

	if ( surface->format->BitsPerPixel != 1 ) {
		/* We don't support sub 8-bit packed pixel modes */
		return NULL;
	}
	if ( surface->map->dst->format->BitsPerPixel < 8 ) {
		which = 0;
	} else {
		which = surface->map->dst->format->BytesPerPixel;
	}
	switch(blit_index) {
	case 0:			/* copy */
	    return bitmap_blit[which];

	case 1:			/* colorkey */
	    return colorkey_blit[which];

	case 2:			/* alpha */
	    return which >= 2 ? &BlitBtoNAlpha : NULL;

	case 4:			/* alpha + colorkey */
	    return which >= 2 ? &BlitBtoNAlphaKey : NULL;
	}
	return NULL;
}

