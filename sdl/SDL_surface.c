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

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <string.h>
#if defined(_DEBUG)
#include <stdio.h>
#endif
#include "SDL_video.h"

SDL_PixelFormat *SDL_AllocFormat(int bpp,
		uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
SDL_PixelFormat *SDL_ReallocFormat(SDL_Surface *surface, int bpp,
		uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask);
void SDL_FormatChanged(SDL_Surface *surface);
void SDL_FreeFormat(SDL_PixelFormat *format);

SDL_BlitMap *SDL_AllocBlitMap(void);
void SDL_InvalidateMap(SDL_BlitMap *map);
void SDL_FreeBlitMap(SDL_BlitMap *map);

uint16_t SDL_CalculatePitch(SDL_Surface *surface);

SDL_Surface * SDL_CreateRGBSurface (uint32_t flags,
			int width, int height, int depth,
			uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
	SDL_Surface *screen;
	SDL_Surface *surface;

	if ( width >= 16384 || height >= 65536 ) {
		fprintf(stderr, "Width or height is too large\n");
		return(NULL);
	}

	screen = NULL;
	flags &= ~SDL_HWSURFACE;
	
	surface = (SDL_Surface *)malloc(sizeof(*surface));
	if ( surface == NULL ) {
		fprintf(stderr, "Out of memory\n");
		return(NULL);
	}
	surface->flags = SDL_SWSURFACE;
	surface->format = SDL_AllocFormat(depth, Rmask, Gmask, Bmask, Amask);
	if ( surface->format == NULL ) {
		free(surface);
		return(NULL);
	}
	if ( Amask ) {
		surface->flags |= SDL_SRCALPHA;
	}
	surface->w = width;
	surface->h = height;
	surface->pitch = SDL_CalculatePitch(surface);
	surface->pixels = NULL;
	surface->offset = 0;
	surface->hwdata = NULL;
	surface->locked = 0;
	surface->map = NULL;
	surface->unused1 = 0;
	SDL_SetClipRect(surface, NULL);
	SDL_FormatChanged(surface);

	if ( (flags&SDL_HWSURFACE) == SDL_SWSURFACE ) {
		if ( surface->w && surface->h ) {
			surface->pixels = malloc(surface->h*surface->pitch);
			if ( surface->pixels == NULL ) {
				SDL_FreeSurface(surface);
				fprintf(stderr, "Out of memory\n");
				return(NULL);
			}
			memset(surface->pixels, 0, surface->h*surface->pitch);
		}
	}

	surface->map = SDL_AllocBlitMap();
	if ( surface->map == NULL ) {
		SDL_FreeSurface(surface);
		return(NULL);
	}

	surface->refcount = 1;
	return(surface);
}

SDL_Surface * SDL_CreateRGBSurfaceFrom (void *pixels,
			int width, int height, int depth, int pitch,
			uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
	SDL_Surface *surface;
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 0, 0, depth,
	                               Rmask, Gmask, Bmask, Amask);
	if ( surface != NULL ) {
		surface->flags |= SDL_PREALLOC;
		surface->pixels = pixels;
		surface->w = width;
		surface->h = height;
		surface->pitch = pitch;
		SDL_SetClipRect(surface, NULL);
	}
	return(surface);
}

static inline int SDL_IntersectRect(const SDL_Rect *A, const SDL_Rect *B, SDL_Rect *intersection)
{
	int Amin, Amax, Bmin, Bmax;

	Amin = A->x;
	Amax = Amin + A->w;
	Bmin = B->x;
	Bmax = Bmin + B->w;
	if(Bmin > Amin)
		Amin = Bmin;
	intersection->x = Amin;
	if(Bmax < Amax)
		Amax = Bmax;
	intersection->w = Amax - Amin > 0 ? Amax - Amin : 0;

	Amin = A->y;
	Amax = Amin + A->h;
	Bmin = B->y;
	Bmax = Bmin + B->h;
	if (Bmin > Amin)
		Amin = Bmin;
	intersection->y = Amin;
	if (Bmax < Amax)
		Amax = Bmax;
	intersection->h = Amax - Amin > 0 ? Amax - Amin : 0;
	return ((intersection->w && intersection->h) ? 1 : 0);
}

int SDL_SetClipRect(SDL_Surface *surface, const SDL_Rect *rect)
{
	SDL_Rect full_rect;
	if ( ! surface ) {
		return 0;
	}
	full_rect.x = 0;
	full_rect.y = 0;
	full_rect.w = surface->w;
	full_rect.h = surface->h;
	if ( ! rect ) {
		surface->clip_rect = full_rect;
		return 1;
	}
	return SDL_IntersectRect(rect, &full_rect, &surface->clip_rect);
}

void SDL_GetClipRect(SDL_Surface *surface, SDL_Rect *rect)
{
	if ( surface && rect ) {
		*rect = surface->clip_rect;
	}
}

int SDL_LockSurface (SDL_Surface *surface)
{
	if ( ! surface->locked ) {
		surface->pixels = (uint8_t *)surface->pixels + surface->offset;
	}
	++surface->locked;
	return(0);
}

void SDL_UnlockSurface (SDL_Surface *surface)
{
	if ( ! surface->locked || (--surface->locked > 0) ) {
		return;
	}
	surface->pixels = (uint8_t *)surface->pixels - surface->offset;
}

void SDL_FreeSurface (SDL_Surface *surface)
{
	if (surface == NULL) {
		return;
	}
	if ( --surface->refcount > 0 ) {
		return;
	}
	while ( surface->locked > 0 ) {
		SDL_UnlockSurface(surface);
	}
	if ( surface->format ) {
		SDL_FreeFormat(surface->format);
		surface->format = NULL;
	}
	if ( surface->map != NULL ) {
		SDL_FreeBlitMap(surface->map);
		surface->map = NULL;
	}
	if ( surface->pixels &&
	     ((surface->flags & SDL_PREALLOC) != SDL_PREALLOC) ) {
		free(surface->pixels);
	}
	free(surface);
}


















/*=======================================*/



SDL_PixelFormat *SDL_AllocFormat(int bpp,
			uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
	SDL_PixelFormat *format;
	uint32_t mask;

	format = malloc(sizeof(*format));
	if ( format == NULL ) {
		fprintf(stderr, "Out of memory\n");
		return(NULL);
	}
	memset(format, 0, sizeof(*format));
	format->alpha = SDL_ALPHA_OPAQUE;

	format->BitsPerPixel = bpp;
	format->BytesPerPixel = (bpp+7)/8;
	if ( Rmask || Bmask || Gmask ) {
		format->palette = NULL;
		format->Rshift = 0;
		format->Rloss = 8;
		if ( Rmask ) {
			for ( mask = Rmask; !(mask&0x01); mask >>= 1 )
				++format->Rshift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Rloss;
		}
		format->Gshift = 0;
		format->Gloss = 8;
		if ( Gmask ) {
			for ( mask = Gmask; !(mask&0x01); mask >>= 1 )
				++format->Gshift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Gloss;
		}
		format->Bshift = 0;
		format->Bloss = 8;
		if ( Bmask ) {
			for ( mask = Bmask; !(mask&0x01); mask >>= 1 )
				++format->Bshift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Bloss;
		}
		format->Ashift = 0;
		format->Aloss = 8;
		if ( Amask ) {
			for ( mask = Amask; !(mask&0x01); mask >>= 1 )
				++format->Ashift;
			for ( ; (mask&0x01); mask >>= 1 )
				--format->Aloss;
		}
		format->Rmask = Rmask;
		format->Gmask = Gmask;
		format->Bmask = Bmask;
		format->Amask = Amask;
	} else if ( bpp > 8 ) {
		if ( bpp > 24 )
			bpp = 24;
		format->Rloss = 8-(bpp/3);
		format->Gloss = 8-(bpp/3)-(bpp%3);
		format->Bloss = 8-(bpp/3);
		format->Rshift = ((bpp/3)+(bpp%3))+(bpp/3);
		format->Gshift = (bpp/3);
		format->Bshift = 0;
		format->Rmask = ((0xFF>>format->Rloss)<<format->Rshift);
		format->Gmask = ((0xFF>>format->Gloss)<<format->Gshift);
		format->Bmask = ((0xFF>>format->Bloss)<<format->Bshift);
	} else {
		format->Rloss = 8;
		format->Gloss = 8;
		format->Bloss = 8;
		format->Aloss = 8;
		format->Rshift = 0;
		format->Gshift = 0;
		format->Bshift = 0;
		format->Ashift = 0;
		format->Rmask = 0;
		format->Gmask = 0;
		format->Bmask = 0;
		format->Amask = 0;
	}
	if ( bpp <= 8 ) {
		int ncolors = 1<<bpp;
#ifdef DEBUG_PALETTE
		fprintf(stderr,"bpp=%d ncolors=%d\n",bpp,ncolors);
#endif
		format->palette = (SDL_Palette *)malloc(sizeof(SDL_Palette));
		if ( format->palette == NULL ) {
			SDL_FreeFormat(format);
			fprintf(stderr, "Out of memory\n");
			return(NULL);
		}
		(format->palette)->ncolors = ncolors;
		(format->palette)->colors = (SDL_Color *)malloc(
				(format->palette)->ncolors*sizeof(SDL_Color));
		if ( (format->palette)->colors == NULL ) {
			SDL_FreeFormat(format);
			fprintf(stderr, "Out of memory\n");
			return(NULL);
		}
		if ( Rmask || Bmask || Gmask ) {
			int i;
			int Rm=0,Gm=0,Bm=0;
			int Rw=0,Gw=0,Bw=0;
#ifdef ENABLE_PALETTE_ALPHA
			int Am=0,Aw=0;
#endif
			if(Rmask)
			{
				Rw=8-format->Rloss;
				for(i=format->Rloss;i>0;i-=Rw)
					Rm|=1<<i;
			}
#ifdef DEBUG_PALETTE
			fprintf(stderr,"Rw=%d Rm=0x%02X\n",Rw,Rm);
#endif
			if(Gmask)
			{
				Gw=8-format->Gloss;
				for(i=format->Gloss;i>0;i-=Gw)
					Gm|=1<<i;
			}
#ifdef DEBUG_PALETTE
			fprintf(stderr,"Gw=%d Gm=0x%02X\n",Gw,Gm);
#endif
			if(Bmask)
			{
				Bw=8-format->Bloss;
				for(i=format->Bloss;i>0;i-=Bw)
					Bm|=1<<i;
			}
#ifdef DEBUG_PALETTE
			fprintf(stderr,"Bw=%d Bm=0x%02X\n",Bw,Bm);
#endif
#ifdef ENABLE_PALETTE_ALPHA
			if(Amask)
			{
				Aw=8-format->Aloss;
				for(i=format->Aloss;i>0;i-=Aw)
					Am|=1<<i;
			}
# ifdef DEBUG_PALETTE
			fprintf(stderr,"Aw=%d Am=0x%02X\n",Aw,Am);
# endif
#endif
			for(i=0; i < ncolors; ++i) {
				int r,g,b;
				r=(i&Rmask)>>format->Rshift;
				r=(r<<format->Rloss)|((r*Rm)>>Rw);
				format->palette->colors[i].r=r;

				g=(i&Gmask)>>format->Gshift;
				g=(g<<format->Gloss)|((g*Gm)>>Gw);
				format->palette->colors[i].g=g;

				b=(i&Bmask)>>format->Bshift;
				b=(b<<format->Bloss)|((b*Bm)>>Bw);
				format->palette->colors[i].b=b;

#ifdef ENABLE_PALETTE_ALPHA
				a=(i&Amask)>>format->Ashift;
				a=(a<<format->Aloss)|((a*Am)>>Aw);
				format->palette->colors[i].unused=a;
#else
				format->palette->colors[i].unused=0;
#endif
			}
		} else if ( ncolors == 2 ) {
			format->palette->colors[0].r = 0xFF;
			format->palette->colors[0].g = 0xFF;
			format->palette->colors[0].b = 0xFF;
			format->palette->colors[1].r = 0x00;
			format->palette->colors[1].g = 0x00;
			format->palette->colors[1].b = 0x00;
		} else {
			memset((format->palette)->colors, 0,
				(format->palette)->ncolors*sizeof(SDL_Color));
		}
	}
	return(format);
}

SDL_PixelFormat *SDL_ReallocFormat(SDL_Surface *surface, int bpp,
			uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
	if ( surface->format ) {
		SDL_FreeFormat(surface->format);
		SDL_FormatChanged(surface);
	}
	surface->format = SDL_AllocFormat(bpp, Rmask, Gmask, Bmask, Amask);
	return surface->format;
}

void SDL_FormatChanged(SDL_Surface *surface)
{
	static int format_version = 0;
	++format_version;
	if ( format_version < 0 ) {
		format_version = 1;
	}
	surface->format_version = format_version;
	SDL_InvalidateMap(surface->map);
}

void SDL_FreeFormat(SDL_PixelFormat *format)
{
	if ( format ) {
		if ( format->palette ) {
			if ( format->palette->colors ) {
				free(format->palette->colors);
			}
			free(format->palette);
		}
		free(format);
	}
}

uint16_t SDL_CalculatePitch(SDL_Surface *surface)
{
	uint16_t pitch;
	pitch = surface->w*surface->format->BytesPerPixel;
	switch (surface->format->BitsPerPixel) {
	case 1:
		pitch = (pitch+7)/8;
		break;
		
	case 4:
		pitch = (pitch+1)/2;
		break;
		
	default:
		break;
	}
	pitch = (pitch + 3) & ~3;
	return(pitch);
}



uint8_t SDL_FindColor(SDL_Palette *pal, uint8_t r, uint8_t g, uint8_t b)
{
	/* Do colorspace distance matching */
	unsigned int smallest;
	unsigned int distance;
	int rd, gd, bd;
	int i;
	uint8_t pixel=0;
	
	smallest = ~0;
	for ( i=0; i<pal->ncolors; ++i ) {
		rd = pal->colors[i].r - r;
		gd = pal->colors[i].g - g;
		bd = pal->colors[i].b - b;
		distance = (rd*rd)+(gd*gd)+(bd*bd);
		if ( distance < smallest ) {
			pixel = i;
			if ( distance == 0 ) { /* Perfect match! */
				break;
			}
			smallest = distance;
		}
	}
	return(pixel);
}

uint32_t SDL_MapRGB(const SDL_PixelFormat* const format, const uint8_t r, const uint8_t g, const uint8_t b)
{
	if ( format->palette == NULL) {
		return (r >> format->Rloss) << format->Rshift
		       | (g >> format->Gloss) << format->Gshift
		       | (b >> format->Bloss) << format->Bshift
		       | format->Amask;
	} else {
		return SDL_FindColor(format->palette, r, g, b);
	}
}

uint32_t SDL_MapRGBA(const SDL_PixelFormat* const format, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
	if ( format->palette == NULL ) {
	        return (r >> format->Rloss) << format->Rshift
		    | (g >> format->Gloss) << format->Gshift
		    | (b >> format->Bloss) << format->Bshift
		    | ((a >> format->Aloss) << format->Ashift & format->Amask);
	} else {
		return SDL_FindColor(format->palette, r, g, b);
	}
}



void SDL_GetRGBA(uint32_t pixel, const SDL_PixelFormat * const fmt,
		 uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a)
{
	if ( fmt->palette == NULL ) {
	        unsigned v;
		v = (pixel & fmt->Rmask) >> fmt->Rshift;
		*r = (v << fmt->Rloss) + (v >> (8 - (fmt->Rloss << 1)));
		v = (pixel & fmt->Gmask) >> fmt->Gshift;
		*g = (v << fmt->Gloss) + (v >> (8 - (fmt->Gloss << 1)));
		v = (pixel & fmt->Bmask) >> fmt->Bshift;
		*b = (v << fmt->Bloss) + (v >> (8 - (fmt->Bloss << 1)));
		if(fmt->Amask) {
		        v = (pixel & fmt->Amask) >> fmt->Ashift;
			*a = (v << fmt->Aloss) + (v >> (8 - (fmt->Aloss << 1)));
		} else {
		        *a = SDL_ALPHA_OPAQUE;
                }
	} else {
		*r = fmt->palette->colors[pixel].r;
		*g = fmt->palette->colors[pixel].g;
		*b = fmt->palette->colors[pixel].b;
		*a = SDL_ALPHA_OPAQUE;
	}
}

void SDL_GetRGB(uint32_t pixel, const SDL_PixelFormat * const fmt,
                uint8_t *r,uint8_t *g,uint8_t *b)
{
	if ( fmt->palette == NULL ) {
	    unsigned v;
		v = (pixel & fmt->Rmask) >> fmt->Rshift;
		*r = (v << fmt->Rloss) + (v >> (8 - (fmt->Rloss << 1)));
		v = (pixel & fmt->Gmask) >> fmt->Gshift;
		*g = (v << fmt->Gloss) + (v >> (8 - (fmt->Gloss << 1)));
		v = (pixel & fmt->Bmask) >> fmt->Bshift;
		*b = (v << fmt->Bloss) + (v >> (8 - (fmt->Bloss << 1)));
	} else {
		*r = fmt->palette->colors[pixel].r;
		*g = fmt->palette->colors[pixel].g;
		*b = fmt->palette->colors[pixel].b;
	}
}

SDL_BlitMap *SDL_AllocBlitMap(void)
{
	SDL_BlitMap *map;
	map = (SDL_BlitMap *)malloc(sizeof(*map));
	if ( map == NULL ) {
		fprintf(stderr, "Out of memory\n");
		return(NULL);
	}
	memset(map, 0, sizeof(*map));
	map->sw_data = (struct private_swaccel *)malloc(sizeof(*map->sw_data));
	if ( map->sw_data == NULL ) {
		SDL_FreeBlitMap(map);
		fprintf(stderr, "Out of memory\n");
		return(NULL);
	}
	memset(map->sw_data, 0, sizeof(*map->sw_data));
	return(map);
}

void SDL_InvalidateMap(SDL_BlitMap *map)
{
	if ( ! map ) {
		return;
	}
	map->dst = NULL;
	map->format_version = (unsigned int)-1;
	if ( map->table ) {
		free(map->table);
		map->table = NULL;
	}
}

void SDL_FreeBlitMap(SDL_BlitMap *map)
{
	if ( map ) {
		SDL_InvalidateMap(map);
		if ( map->sw_data != NULL ) {
			free(map->sw_data);
		}
		free(map);
	}
}

/*=======================================*/


static int SDL_FillRect1(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color)
{
	/* FIXME: We have to worry about packing order.. *sigh* */
	fprintf(stderr, "%s\n", "1-bpp rect fill not yet implemented");
	return -1;
}
static int SDL_FillRect4(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color)
{
	/* FIXME: We have to worry about packing order.. *sigh* */
	fprintf(stderr, "%s\n", "4-bpp rect fill not yet implemented");
	return -1;
}
int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, uint32_t color)
{
	int x, y;
	uint8_t *row;

	/* This function doesn't work on surfaces < 8 bpp */
	if ( dst->format->BitsPerPixel < 8 ) {
		switch(dst->format->BitsPerPixel) {
		    case 1:
			return SDL_FillRect1(dst, dstrect, color);
			break;

		    case 4:
			return SDL_FillRect4(dst, dstrect, color);
			break;
		    
			default:
			fprintf(stderr, "%s\n", "Fill rect on unsupported surface format");
			return(-1);
			break;
		}
	}

	/* If 'dstrect' == NULL, then fill the whole surface */
	if ( dstrect ) {
		/* Perform clipping */
		if ( !SDL_IntersectRect(dstrect, &dst->clip_rect, dstrect) ) {
			return(0);
		}
	} else {
		dstrect = &dst->clip_rect;
	}

	if ( SDL_LockSurface(dst) != 0 ) {
		return(-1);
	}
	row = (uint8_t *)dst->pixels+dstrect->y*dst->pitch+
			dstrect->x*dst->format->BytesPerPixel;
	if ( dst->format->palette || (color == 0) ) {
		x = dstrect->w*dst->format->BytesPerPixel;
		if ( !color && !((uintptr_t)row&3) && !(x&3) && !(dst->pitch&3) ) {
			int n = x >> 2;
			for ( y=dstrect->h; y; --y ) {
				SDL_memset4(row, 0, n);
				row += dst->pitch;
			}
		} else {
			{
				for (y = dstrect->h; y; y--) {
					memset(row, color, x);
					row += dst->pitch;
				}
			}
		}
	} else {
		switch (dst->format->BytesPerPixel) {
		    case 2:
			for ( y=dstrect->h; y; --y ) {
				uint16_t *pixels = (uint16_t *)row;
				uint16_t c = (uint16_t)color;
				uint32_t cc = (uint32_t)c << 16 | c;
				int n = dstrect->w;
				if((uintptr_t)pixels & 3) {
					*pixels++ = c;
					n--;
				}
				if(n >> 1)
					SDL_memset4(pixels, cc, n >> 1);
				if(n & 1)
					pixels[n - 1] = c;
				row += dst->pitch;
			}
			break;

		    case 3:
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				color <<= 8;
			#endif
			for ( y=dstrect->h; y; --y ) {
				uint8_t *pixels = row;
				for ( x=dstrect->w; x; --x ) {
					memcpy(pixels, &color, 3);
					pixels += 3;
				}
				row += dst->pitch;
			}
			break;

		    case 4:
			for(y = dstrect->h; y; --y) {
				SDL_memset4(row, color, dstrect->w);
				row += dst->pitch;
			}
			break;
		}
	}
	SDL_UnlockSurface(dst);
	return(0);
}

/*=======================================*/





/*=======================================*/

static int SDL_SoftBlit(SDL_Surface *src, SDL_Rect *srcrect,
			SDL_Surface *dst, SDL_Rect *dstrect)
{
	int okay;
	int src_locked;
	int dst_locked;

	/* Everything is okay at the beginning...  */
	okay = 1;

	/* Lock the destination if it's in hardware */
	dst_locked = 0;
	if ( SDL_MUSTLOCK(dst) ) {
		if ( SDL_LockSurface(dst) < 0 ) {
			okay = 0;
		} else {
			dst_locked = 1;
		}
	}
	/* Lock the source if it's in hardware */
	src_locked = 0;
	if ( SDL_MUSTLOCK(src) ) {
		if ( SDL_LockSurface(src) < 0 ) {
			okay = 0;
		} else {
			src_locked = 1;
		}
	}

	/* Set up source and destination buffer pointers, and BLIT! */
	if ( okay  && srcrect->w && srcrect->h ) {
		SDL_BlitInfo info;
		SDL_loblit RunBlit;

		/* Set up the blit information */
		info.s_pixels = (uint8_t *)src->pixels +
				(uint16_t)srcrect->y*src->pitch +
				(uint16_t)srcrect->x*src->format->BytesPerPixel;
		info.s_width = srcrect->w;
		info.s_height = srcrect->h;
		info.s_skip=src->pitch-info.s_width*src->format->BytesPerPixel;
		info.d_pixels = (uint8_t *)dst->pixels +
				(uint16_t)dstrect->y*dst->pitch +
				(uint16_t)dstrect->x*dst->format->BytesPerPixel;
		info.d_width = dstrect->w;
		info.d_height = dstrect->h;
		info.d_skip=dst->pitch-info.d_width*dst->format->BytesPerPixel;
		info.aux_data = src->map->sw_data->aux_data;
		info.src = src->format;
		info.table = src->map->table;
		info.dst = dst->format;
		RunBlit = src->map->sw_data->blit;

		/* Run the actual software blit */
		RunBlit(&info);
	}

	/* We need to unlock the surfaces if they're locked */
	if ( dst_locked ) {
		SDL_UnlockSurface(dst);
	}
	if ( src_locked ) {
		SDL_UnlockSurface(src);
	}
	/* Blit is done! */
	return(okay ? 0 : -1);
}



static void SDL_BlitCopy(SDL_BlitInfo *info)
{
	uint8_t *src, *dst;
	int w, h;
	int srcskip, dstskip;

	w = info->d_width*info->dst->BytesPerPixel;
	h = info->d_height;
	src = info->s_pixels;
	dst = info->d_pixels;
	srcskip = w+info->s_skip;
	dstskip = w+info->d_skip;

	while ( h-- ) {
		memcpy(dst, src, w);
		src += srcskip;
		dst += dstskip;
	}
}
void *SDL_revcpy(void *dst, const void *src, size_t len)
{
    char *srcp = (char *)src;
    char *dstp = (char *)dst;
    srcp += len-1;
    dstp += len-1;
    while ( len-- ) {
        *dstp-- = *srcp--;
    }
    return dst;
}
static void SDL_BlitCopyOverlap(SDL_BlitInfo *info)
{
	uint8_t *src, *dst;
	int w, h;
	int srcskip, dstskip;

	w = info->d_width*info->dst->BytesPerPixel;
	h = info->d_height;
	src = info->s_pixels;
	dst = info->d_pixels;
	srcskip = w+info->s_skip;
	dstskip = w+info->d_skip;
	if ( dst < src ) {
		while ( h-- ) {
			memmove(dst, src, w);
			src += srcskip;
			dst += dstskip;
		}
	} else {
		src += ((h-1) * srcskip);
		dst += ((h-1) * dstskip);
		while ( h-- ) {
			SDL_revcpy(dst, src, w);
			src -= srcskip;
			dst -= dstskip;
		}
	}
}




int SDL_CalculateBlit(SDL_Surface *surface)
{
	int blit_index;

	/* Clean everything out to start */
	surface->map->sw_blit = NULL;

	/* Figure out if an accelerated hardware blit is possible */
	surface->flags &= ~SDL_HWACCEL;
	if ( surface->map->identity ) {

	}

	/* Get the blit function index, based on surface mode */
	/* { 0 = nothing, 1 = colorkey, 2 = alpha, 3 = colorkey+alpha } */
	blit_index = 0;
	blit_index |= (!!(surface->flags & SDL_SRCCOLORKEY))      << 0;
	if ( surface->flags & SDL_SRCALPHA
	     && (surface->format->alpha != SDL_ALPHA_OPAQUE
		 || surface->format->Amask) ) {
	        blit_index |= 2;
	}

	/* Check for special "identity" case -- copy blit */
	if ( surface->map->identity && blit_index == 0 ) {
	        surface->map->sw_data->blit = SDL_BlitCopy;

		/* Handle overlapping blits on the same surface */
		if ( surface == surface->map->dst ) {
		        surface->map->sw_data->blit = SDL_BlitCopyOverlap;
		}
	} else {
		if ( surface->format->BitsPerPixel < 8 ) {
			surface->map->sw_data->blit =
			    SDL_CalculateBlit0(surface, blit_index);
		} else {
			switch ( surface->format->BytesPerPixel ) {
			    case 1:
				surface->map->sw_data->blit =
				    SDL_CalculateBlit1(surface, blit_index);
				break;
			    case 2:
			    case 3:
			    case 4:
				surface->map->sw_data->blit =
				    SDL_CalculateBlitN(surface, blit_index);
				break;
			    default:
				surface->map->sw_data->blit = NULL;
				break;
			}
		}
	}
	/* Make sure we have a blit function */
	if ( surface->map->sw_data->blit == NULL ) {
		SDL_InvalidateMap(surface->map);
		fprintf(stderr, "%s\n", "Blit combination not supported");
		return(-1);
	}
	
	if ( surface->map->sw_blit == NULL ) {
		surface->map->sw_blit = SDL_SoftBlit;
	}
	return(0);
}




/*=======================================*/



void SDL_DitherColors(SDL_Color *colors, int bpp)
{
	int i;
	if(bpp != 8)
		return;		/* only 8bpp supported right now */

	for(i = 0; i < 256; i++) {
		int r, g, b;
		/* map each bit field to the full [0, 255] interval,
		   so 0 is mapped to (0, 0, 0) and 255 to (255, 255, 255) */
		r = i & 0xe0;
		r |= r >> 3 | r >> 6;
		colors[i].r = r;
		g = (i << 3) & 0xe0;
		g |= g >> 3 | g >> 6;
		colors[i].g = g;
		b = i & 0x3;
		b |= b << 2;
		b |= b << 4;
		colors[i].b = b;
	}
}
static uint8_t *Map1to1(SDL_Palette *src, SDL_Palette *dst, int *identical)
{
	uint8_t *map;
	int i;

	if ( identical ) {
		if ( src->ncolors <= dst->ncolors ) {
			/* If an identical palette, no need to map */
			if ( memcmp(src->colors, dst->colors, src->ncolors*
						sizeof(SDL_Color)) == 0 ) {
				*identical = 1;
				return(NULL);
			}
		}
		*identical = 0;
	}
	map = (uint8_t *)malloc(src->ncolors);
	if ( map == NULL ) {
		fprintf(stderr, "%s\n", "Out Of Memory");
		return(NULL);
	}
	for ( i=0; i<src->ncolors; ++i ) {
		map[i] = SDL_FindColor(dst,
			src->colors[i].r, src->colors[i].g, src->colors[i].b);
	}
	return(map);
}
static uint8_t *Map1toN(SDL_PixelFormat *src, SDL_PixelFormat *dst)
{
	uint8_t *map;
	int i;
	int  bpp;
	unsigned alpha;
	SDL_Palette *pal = src->palette;

	bpp = ((dst->BytesPerPixel == 3) ? 4 : dst->BytesPerPixel);
	map = (uint8_t *)malloc(pal->ncolors*bpp);
	if ( map == NULL ) {
		fprintf(stderr, "%s\n", "Out Of Memory");
		return(NULL);
	}

	alpha = dst->Amask ? src->alpha : 0;
	/* We memory copy to the pixel map so the endianness is preserved */
	for ( i=0; i<pal->ncolors; ++i ) {
		ASSEMBLE_RGBA(&map[i*bpp], dst->BytesPerPixel, dst,
			      pal->colors[i].r, pal->colors[i].g,
			      pal->colors[i].b, alpha);
	}
	return(map);
}
static uint8_t *MapNto1(SDL_PixelFormat *src, SDL_PixelFormat *dst, int *identical)
{
	/* Generate a 256 color dither palette */
	SDL_Palette dithered;
	SDL_Color colors[256];
	SDL_Palette *pal = dst->palette;
	
	/* SDL_DitherColors does not initialize the 'unused' component of colors,
	   but Map1to1 compares it against pal, so we should initialize it. */  
	memset(colors, 0, sizeof(colors));

	dithered.ncolors = 256;
	SDL_DitherColors(colors, 8);
	dithered.colors = colors;
	return(Map1to1(&dithered, pal, identical));
}
int SDL_MapSurface (SDL_Surface *src, SDL_Surface *dst)
{
	SDL_PixelFormat *srcfmt;
	SDL_PixelFormat *dstfmt;
	SDL_BlitMap *map;

	/* Clear out any previous mapping */
	map = src->map;
	SDL_InvalidateMap(map);

	/* Figure out what kind of mapping we're doing */
	map->identity = 0;
	srcfmt = src->format;
	dstfmt = dst->format;
	switch (srcfmt->BytesPerPixel) {
	    case 1:
		switch (dstfmt->BytesPerPixel) {
		    case 1:
			/* Palette --> Palette */
			/* If both SDL_HWSURFACE, assume have same palette */
			if ( ((src->flags & SDL_HWSURFACE) == SDL_HWSURFACE) &&
			     ((dst->flags & SDL_HWSURFACE) == SDL_HWSURFACE) ) {
				map->identity = 1;
			} else {
				map->table = Map1to1(srcfmt->palette,
					dstfmt->palette, &map->identity);
			}
			if ( ! map->identity ) {
				if ( map->table == NULL ) {
					return(-1);
				}
			}
			if (srcfmt->BitsPerPixel!=dstfmt->BitsPerPixel)
				map->identity = 0;
			break;

		    default:
			/* Palette --> BitField */
			map->table = Map1toN(srcfmt, dstfmt);
			if ( map->table == NULL ) {
				return(-1);
			}
			break;
		}
		break;
	default:
		switch (dstfmt->BytesPerPixel) {
		    case 1:
			/* BitField --> Palette */
			map->table = MapNto1(srcfmt, dstfmt, &map->identity);
			if ( ! map->identity ) {
				if ( map->table == NULL ) {
					return(-1);
				}
			}
			map->identity = 0;	/* Don't optimize to copy */
			break;
		    default:
			/* BitField --> BitField */
			if ( FORMAT_EQUAL(srcfmt, dstfmt) )
				map->identity = 1;
			break;
		}
		break;
	}

	map->dst = dst;
	map->format_version = dst->format_version;

	/* Choose your blitters wisely */
	return(SDL_CalculateBlit(src));
}
int SDL_LowerBlit (SDL_Surface *src, SDL_Rect *srcrect,
				SDL_Surface *dst, SDL_Rect *dstrect)
{
	SDL_blit do_blit;

	/* Check to make sure the blit mapping is valid */
	if ( (src->map->dst != dst) ||
             (src->map->dst->format_version != src->map->format_version) ) {
		if ( SDL_MapSurface(src, dst) < 0 ) {
			return(-1);
		}
	}

	/* Figure out which blitter to use */
	do_blit = src->map->sw_blit;
	return(do_blit(src, srcrect, dst, dstrect));
}


int SDL_UpperBlit (SDL_Surface *src, SDL_Rect *srcrect,
		   SDL_Surface *dst, SDL_Rect *dstrect)
{
    SDL_Rect fulldst;
	int srcx, srcy, w, h;

	if ( ! src || ! dst ) {
		fprintf(stderr, "%s\n", "SDL_UpperBlit: passed a NULL surface");
		return(-1);
	}
	if ( src->locked || dst->locked ) {
		fprintf(stderr, "%s\n", "Surfaces must not be locked during blit");
		return(-1);
	}

	/* If the destination rectangle is NULL, use the entire dest surface */
	if ( dstrect == NULL ) {
	        fulldst.x = fulldst.y = 0;
		dstrect = &fulldst;
	}

	/* clip the source rectangle to the source surface */
	if(srcrect) {
	        int maxw, maxh;
	
		srcx = srcrect->x;
		w = srcrect->w;
		if(srcx < 0) {
		        w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->w - srcx;
		if(maxw < w)
			w = maxw;

		srcy = srcrect->y;
		h = srcrect->h;
		if(srcy < 0) {
		        h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->h - srcy;
		if(maxh < h)
			h = maxh;
	    
	} else {
	        srcx = srcy = 0;
		w = src->w;
		h = src->h;
	}

	/* clip the destination rectangle against the clip rectangle */
	{
	    SDL_Rect *clip = &dst->clip_rect;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if(dx > 0) {
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if(dx > 0)
			w -= dx;

		dy = clip->y - dstrect->y;
		if(dy > 0) {
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if(dy > 0)
			h -= dy;
	}

	if(w > 0 && h > 0) {
	    SDL_Rect sr;
	    sr.x = srcx;
		sr.y = srcy;
		sr.w = dstrect->w = w;
		sr.h = dstrect->h = h;
		return SDL_LowerBlit(src, &sr, dst, dstrect);
	}
	dstrect->w = dstrect->h = 0;
	return 0;
}














/*==============================================*/


int SDL_SetColorKey (SDL_Surface *surface, uint32_t flag, uint32_t key)
{
	/* Sanity check the flag as it gets passed in */
	if ( flag & SDL_SRCCOLORKEY ) {
		if ( flag & (SDL_RLEACCEL|SDL_RLEACCELOK) ) {
			flag = (SDL_SRCCOLORKEY | SDL_RLEACCELOK);
		} else {
			flag = SDL_SRCCOLORKEY;
		}
	} else {
		flag = 0;
	}

	/* Optimize away operations that don't change anything */
	if ( (flag == (surface->flags & (SDL_SRCCOLORKEY|SDL_RLEACCELOK))) &&
	     (key == surface->format->colorkey) ) {
		return(0);
	}

	if ( flag ) {
		surface->flags |= SDL_SRCCOLORKEY;
		surface->format->colorkey = key;

		if ( flag & SDL_RLEACCELOK ) {
			surface->flags |= SDL_RLEACCELOK;
		} else {
			surface->flags &= ~SDL_RLEACCELOK;
		}
	} else {
		surface->flags &= ~(SDL_SRCCOLORKEY|SDL_RLEACCELOK);
		surface->format->colorkey = 0;
	}
	SDL_InvalidateMap(surface->map);
	return(0);
}


