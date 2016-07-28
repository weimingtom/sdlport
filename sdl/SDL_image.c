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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "SDL_endian.h"
#include "SDL_image.h"

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

#ifndef BI_RGB
#define BI_RGB		0
#define BI_RLE8		1
#define BI_RLE4		2
#define BI_BITFIELDS	3
#endif


static struct {
	char *type;
	int (*is)(FILE *src);
	SDL_Surface *(*load)(FILE *src);
} supported[] = {
	{ "BMP", IMG_isBMP, IMG_LoadBMP_RW },
};


SDL_Surface *IMG_Load(const char *file)
{
    FILE *src = fopen(file, "rb");
    char *ext = strrchr(file, '.');
    if (ext) {
        ext++;
    }
    if (!src) {
        return NULL;
    }
    return IMG_LoadTyped_RW(src, 1, ext);
}

SDL_Surface *IMG_Load_RW(FILE *src, int freesrc)
{
    return IMG_LoadTyped_RW(src, freesrc, NULL);
}

static int IMG_string_equals(const char *str1, const char *str2)
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

SDL_Surface *IMG_LoadTyped_RW(FILE *src, int freesrc, char *type)
{
	int i;
	SDL_Surface *image;

	if (src == NULL) {
		fprintf(stderr, "%s\n", "Passed a NULL data source");
		return(NULL);
	}

	if (fseek(src, 0, SEEK_CUR) < 0) {
		fprintf(stderr, "%s\n", "Can't seek in this data source");
		if (freesrc)
			fclose(src);
		return(NULL);
	}

	image = NULL;
	for ( i=0; i < ARRAYSIZE(supported); ++i ) {
		if(supported[i].is) {
			if(!supported[i].is(src))
				continue;
		} else {
			if(!type
			   || !IMG_string_equals(type, supported[i].type))
				continue;
		}
#ifdef DEBUG_IMGLIB
		fprintf(stderr, "IMGLIB: Loading image as %s\n",
			supported[i].type);
#endif
		image = supported[i].load(src);
		if (freesrc)
			fclose(src);
		return image;
	}

	if (freesrc) {
		fclose(src);
	}
	fprintf(stderr, "%s\n", "Unsupported image format");
	return NULL;
}

int IMG_InvertAlpha(int on)
{
    return 1;
}







int IMG_isBMP(FILE *src)
{
	int start;
	int is_BMP;
	char magic[2];

	if (!src)
		return 0;
	start = ftell(src);
	is_BMP = 0;
	if (fread(magic, sizeof(magic), 1, src)) {
		if (strncmp(magic, "BM", 2) == 0) {
			is_BMP = 1;
		}
	}
	fseek(src, start, SEEK_SET);
	return(is_BMP);
}

static int readRlePixels(SDL_Surface * surface, FILE * src, int isRle8)
{
	int pitch = surface->pitch;
	int height = surface->h;
	uint8_t * bits = (uint8_t *)surface->pixels + ((height-1) * pitch);
	int ofs = 0;
	uint8_t ch;
	uint8_t needsPad;

	for (;;) {
		if (!fread(&ch, 1, 1, src)) 
			return 1;
		if (ch) {
			uint8_t pixel;
			if (!fread(&pixel, 1, 1, src)) 
				return 1;
			if (isRle8) {
				do {
					bits[ofs++] = pixel;
				} while (--ch);
			} else {
				uint8_t pixel0 = pixel >> 4;
				uint8_t pixel1 = pixel & 0x0F;
				for (;;) {
					bits[ofs++] = pixel0;     
					if (!--ch) 
						break;
					bits[ofs++] = pixel1;    
					if (!--ch) 
						break;
				}
			}
		} else {
			if (!fread(&ch, 1, 1, src)) 
				return 1;
			switch (ch) {
			case 0:      
				ofs = 0;
				bits -= pitch;      
				break;
			
			case 1:            
				return 0;       
			
			case 2:       
				if (!fread(&ch, 1, 1, src)) 
					return 1;
				ofs += ch;
				if (!fread(&ch, 1, 1, src)) 
					return 1;
				bits -= (ch * pitch);
				break;

			default:
				if (isRle8) {
					needsPad = ( ch & 1 );
					do {
						if (!fread(bits + ofs++, 1, 1, src)) 
							return 1;
					} while (--ch);
				} else {
					needsPad = ( ((ch+1)>>1) & 1 ); 
					for (;;) {
						uint8_t pixel;
						if (!fread(&pixel, 1, 1, src)) 
							return 1;
						bits[ofs++] = pixel >> 4;
						if (!--ch) 
							break;
						bits[ofs++] = pixel & 0x0F;
						if (!--ch) 
							break;
					}
				}
				if (needsPad && !fread(&ch, 1, 1, src)) 
					return 1;
				break;
			}
		}
	}
}










static uint16_t SDL_Swap16(uint16_t x) {
	return((x<<8)|(x>>8));
}
static uint32_t SDL_Swap32(uint32_t x) {
	return((x<<24)|((x<<8)&0x00FF0000)|((x>>8)&0x0000FF00)|(x>>24));
}
static uint64_t SDL_Swap64(uint64_t x)
{
	uint32_t hi, lo;

	lo = (uint32_t)(x&0xFFFFFFFF);
	x >>= 32;
	hi = (uint32_t)(x&0xFFFFFFFF);
	x = SDL_Swap32(lo);
	x <<= 32;
	x |= SDL_Swap32(hi);
	return(x);
}

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define SDL_SwapLE16(X)	(X)
#define SDL_SwapLE32(X)	(X)
#define SDL_SwapLE64(X)	(X)
#define SDL_SwapBE16(X)	SDL_Swap16(X)
#define SDL_SwapBE32(X)	SDL_Swap32(X)
#define SDL_SwapBE64(X)	SDL_Swap64(X)
#else
#define SDL_SwapLE16(X)	SDL_Swap16(X)
#define SDL_SwapLE32(X)	SDL_Swap32(X)
#define SDL_SwapLE64(X)	SDL_Swap64(X)
#define SDL_SwapBE16(X)	(X)
#define SDL_SwapBE32(X)	(X)
#define SDL_SwapBE64(X)	(X)
#endif

static uint32_t SDL_ReadLE32(FILE *src)
{
	uint32_t value;

	fread(&value, (sizeof value), 1, src);
	return SDL_SwapLE32(value);
}

static uint16_t SDL_ReadLE16(FILE *src)
{
	uint16_t value;

	fread(&value, (sizeof value), 1, src);
	return SDL_SwapLE16(value);
}

static SDL_Surface *LoadBMP_RW(FILE *src, int freesrc)
{
	int was_error;
	long fp_offset;
	int bmpPitch;
	int i, pad;
	SDL_Surface *surface;
	uint32_t Rmask;
	uint32_t Gmask;
	uint32_t Bmask;
	uint32_t Amask;
	SDL_Palette *palette;
	uint8_t *bits;
	int ExpandBMP;

	char   magic[2];
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;

	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;

	surface = NULL;
	was_error = 0;
	if (src == NULL) {
		was_error = 1;
		goto done;
	}

	fp_offset = ftell(src);
	//SDL_ClearError();
	if (fread(magic, 1, 2, src) != 2) {
		//SDL_Error(SDL_EFREAD);
		fprintf(stderr, "%s\n", "SDL_EFREAD");
		was_error = 1;
		goto done;
	}
	if (strncmp(magic, "BM", 2) != 0) {
		fprintf(stderr, "%s\n", "File is not a Windows BMP file");
		was_error = 1;
		goto done;
	}
	bfSize		= SDL_ReadLE32(src);
	bfReserved1	= SDL_ReadLE16(src);
	bfReserved2	= SDL_ReadLE16(src);
	bfOffBits	= SDL_ReadLE32(src);

	biSize		= SDL_ReadLE32(src);
	if ( biSize == 12 ) {
		biWidth		= (uint32_t)SDL_ReadLE16(src);
		biHeight	= (uint32_t)SDL_ReadLE16(src);
		biPlanes	= SDL_ReadLE16(src);
		biBitCount	= SDL_ReadLE16(src);
		biCompression	= BI_RGB;
		biSizeImage	= 0;
		biXPelsPerMeter	= 0;
		biYPelsPerMeter	= 0;
		biClrUsed	= 0;
		biClrImportant	= 0;
	} else {
		biWidth		= SDL_ReadLE32(src);
		biHeight	= SDL_ReadLE32(src);
		biPlanes	= SDL_ReadLE16(src);
		biBitCount	= SDL_ReadLE16(src);
		biCompression	= SDL_ReadLE32(src);
		biSizeImage	= SDL_ReadLE32(src);
		biXPelsPerMeter	= SDL_ReadLE32(src);
		biYPelsPerMeter	= SDL_ReadLE32(src);
		biClrUsed	= SDL_ReadLE32(src);
		biClrImportant	= SDL_ReadLE32(src);
	}

	//if ( strcmp(SDL_GetError(), "") != 0 ) {
	//	was_error = 1;
	//	goto done;
	//}

	switch (biBitCount) {
		case 1:
		case 4:
			ExpandBMP = biBitCount;
			biBitCount = 8;
			break;
		default:
			ExpandBMP = 0;
			break;
	}

	Rmask = Gmask = Bmask = Amask = 0;
	switch (biCompression) {
		case BI_RGB:
			if ( bfOffBits == (14+biSize) ) {
				switch (biBitCount) {
					case 15:
					case 16:
						Rmask = 0x7C00;
						Gmask = 0x03E0;
						Bmask = 0x001F;
						break;
					case 24:
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
					        Rmask = 0x000000FF;
					        Gmask = 0x0000FF00;
					        Bmask = 0x00FF0000;
#else
						Rmask = 0x00FF0000;
						Gmask = 0x0000FF00;
						Bmask = 0x000000FF;
#endif
						break;
					case 32:
						Amask = 0xFF000000;
						Rmask = 0x00FF0000;
						Gmask = 0x0000FF00;
						Bmask = 0x000000FF;
						break;
					default:
						break;
				}
				break;
			}
			// Fall through -- read the RGB masks

		default:
			switch (biBitCount) {
				case 15:
				case 16:
				case 32:
					Rmask = SDL_ReadLE32(src);
					Gmask = SDL_ReadLE32(src);
					Bmask = SDL_ReadLE32(src);
					Amask = SDL_ReadLE32(src);
					break;
				default:
					break;
			}
			break;
	}

	surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
			biWidth, biHeight, biBitCount, Rmask, Gmask, Bmask, Amask);
	if ( surface == NULL ) {
		was_error = 1;
		goto done;
	}

	palette = (surface->format)->palette;
	if (palette) {
		if (fseek(src, fp_offset+14+biSize, SEEK_SET) < 0) {
			//SDL_Error(SDL_EFSEEK);
			fprintf(stderr, "%s\n", "SDL_EFSEEK");
			was_error = 1;
			goto done;
		}

		biClrUsed = 1 << biBitCount;
		if ( biSize == 12 ) {
			for ( i = 0; i < (int)biClrUsed; ++i ) {
				fread(&palette->colors[i].b, 1, 1, src);
				fread(&palette->colors[i].g, 1, 1, src);
				fread(&palette->colors[i].r, 1, 1, src);
				palette->colors[i].unused = 0;
			}	
		} else {
			for ( i = 0; i < (int)biClrUsed; ++i ) {
				fread(&palette->colors[i].b, 1, 1, src);
				fread(&palette->colors[i].g, 1, 1, src);
				fread(&palette->colors[i].r, 1, 1, src);
				fread(&palette->colors[i].unused, 1, 1, src);
			}	
		}
		palette->ncolors = biClrUsed;
	}

	if (fseek(src, fp_offset+bfOffBits, SEEK_SET) < 0) {
		fprintf(stderr, "%s\n", "SDL_EFSEEK");
		was_error = 1;
		goto done;
	}
	if ((biCompression == BI_RLE4) || (biCompression == BI_RLE8)) {
		was_error = readRlePixels(surface, src, biCompression == BI_RLE8);
		if (was_error) 
			fprintf(stderr, "%s\n", "Error reading from BMP");
		goto done;
	}
	bits = (uint8_t *)surface->pixels+(surface->h*surface->pitch);
	switch (ExpandBMP) {
	case 1:
		bmpPitch = (biWidth + 7) >> 3;
		pad  = (((bmpPitch)%4) ? (4-((bmpPitch)%4)) : 0);
		break;

	case 4:
		bmpPitch = (biWidth + 1) >> 1;
		pad  = (((bmpPitch)%4) ? (4-((bmpPitch)%4)) : 0);
		break;
	
	default:
		pad  = ((surface->pitch%4) ?
				(4-(surface->pitch%4)) : 0);
		break;
	}
	while ( bits > (uint8_t *)surface->pixels ) {
		bits -= surface->pitch;
		switch (ExpandBMP) {
			case 1:
			case 4: {
					uint8_t pixel = 0;
					int shift = (8-ExpandBMP);
					for (i=0; i<surface->w; ++i ) {
						if ( i%(8/ExpandBMP) == 0 ) {
							if (!fread(&pixel, 1, 1, src)) {
								fprintf(stderr, "%s\n", "Error reading from BMP");
								was_error = 1;
								goto done;
							}
						}
						*(bits+i) = (pixel>>shift);
						pixel <<= ExpandBMP;
					} 
				}
				break;

			default:
				if (fread(bits, 1, surface->pitch, src) != surface->pitch ) {
					//SDL_Error(SDL_EFREAD);
					fprintf(stderr, "%s\n", "SDL_EFREAD");
					was_error = 1;
					goto done;
				}
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				switch(biBitCount) {
					case 15:
					case 16: {
							uint16_t *pix = (uint16_t *)bits;
						for(i = 0; i < surface->w; i++)
								pix[i] = SDL_Swap16(pix[i]);
						break;
					}

					case 32: {
							uint32_t *pix = (uint32_t *)bits;
						for(i = 0; i < surface->w; i++)
								pix[i] = SDL_Swap32(pix[i]);
						break;
					}
				}
#endif
			break;
		}
		if (pad) {
			uint8_t padbyte;
			for (i = 0; i < pad; ++i) {
				fread(&padbyte, 1, 1, src);
			}
		}
	}
done:
	if (was_error) {
		if (src) {
			fseek(src, fp_offset, SEEK_SET);
		}
		if (surface) {
			SDL_FreeSurface(surface);
		}
		surface = NULL;
	}
	if (freesrc && src) {
		fclose(src);
	}
	return surface;
}

SDL_Surface *IMG_LoadBMP_RW(FILE *src)
{
	return LoadBMP_RW(src, 0);
}

