#include "common.h"

#include <SDL_video.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "TextureLoader.h"
#include "test.h"

#define DEFAULT_FONTNAME "default.ttf"
#define DEFAULT_PTSIZE	18
#define DEFAULT_TEXT	"The quick brown fox jumped over the lazy dog"

void test_bmp(SDL_Surface* screen)
{
	unsigned int g_TextureWidth, g_TextureHeight;
	unsigned char * data = loadBMPRaw("image1.bmp", &g_TextureWidth, &g_TextureHeight, 1, 1);	
	SDL_Rect sr, ds;
	if (data)
	{
		SDL_Surface* surface;
		//
		//int i, j;

		surface = SDL_CreateRGBSurfaceFrom(data,
			g_TextureWidth, g_TextureHeight, 24, 3 * g_TextureWidth,
			MY_Rmask, MY_Gmask, MY_Bmask, MY_Amask
			);
		//SDL_SaveBMP(surface, "image1_out.bmp");
		//FIXME: !!!DON't FREE data !!!
		//free(data);
		
		sr.x = 0;
		sr.y = 0;
		sr.w = g_TextureWidth;
		sr.h = g_TextureHeight;
		
		ds.x = 100;//0;
		ds.y = 100;//0;
		ds.w = g_TextureWidth;//640;
		ds.h = g_TextureHeight;//480;
		
		/*
		for (i = 0; i < surface->h; i++)
		{
			for (j = 0; j < surface->w; j++)
			{
				if (0)
				{
					printf("%02X", ((unsigned char *)data)[i * surface->w * 3 + j * 3]);
					printf("%02X", ((unsigned char *)data)[i * surface->w * 3 + j * 3]);
					printf("%02X\n", ((unsigned char *)data)[i * surface->w * 3 + j * 3]);
				}

				if (0)
				{
					printf("%02X", ((unsigned char *)surface->pixels)[i * surface->w * 3 + j * 3]);
					printf("%02X", ((unsigned char *)surface->pixels)[i * surface->w * 3 + j * 3]);
					printf("%02X\n", ((unsigned char *)surface->pixels)[i * surface->w * 3 + j * 3]);
				}

				((unsigned char *)surface->pixels)[i * surface->w * 3 + j * 3] = 0x00;
				((unsigned char *)surface->pixels)[i * surface->w * 3 + j * 3 + 1] = 0x00;
				((unsigned char *)surface->pixels)[i * surface->w * 3 + j * 3 + 2] = 0xff;
			}
		}
		*/


		SDL_SoftStretch(surface, &sr, screen, &ds);


		/*
		for (i = 0; i < screen->h; i++)
		{
			for (j = 0; j < screen->w; j++)
			{
				((char *)screen->pixels)[i * screen->w * 3 + j * 3] = 0x00;
				((char *)screen->pixels)[i * screen->w * 3 + j * 3 + 1] = 0x00;
				((char *)screen->pixels)[i * screen->w * 3 + j * 3 + 2] = 0xff;
			}
		}
		*/

		//dumpBMPRaw("image1_out.bmp", screen->pixels, screen->w, screen->h);
	}
}

void test_ttf(SDL_Surface* screen)
{
	TTF_Font *font;
	const char *fontname = DEFAULT_FONTNAME;
	int ptsize = DEFAULT_PTSIZE;
	int renderstyle = TTF_STYLE_NORMAL; 
	int i;
	SDL_Color white = { 0xFF, 0xFF, 0xFF, 0 };
	SDL_Color black = { 0x00, 0x00, 0x00, 0 };
	SDL_Color *forecol = &black;
	SDL_Color *backcol = &white;
	
	TTF_Init();
	font = TTF_OpenFont(fontname, ptsize);
	if (font == NULL) {
		fprintf(stderr, "Couldn't load %d pt font from %s\n", 
			ptsize, fontname);
		goto clean_end;
	}
	TTF_SetFontStyle(font, renderstyle);
	//48-57 : 0-9
	//58-64 : :;<=>?@
	//65-90 : A-Z
	//91-96 : [\]^-'
	//97-122: a-z
	for (i = 48; i < 123; i++) {
		SDL_Surface* glyph = NULL;
		char outname[64];
		SDL_Surface *glyph2 = NULL;
		int j;
			
		//, *backcol
		glyph = TTF_RenderGlyph_Blended(font, (uint16_t)i, *forecol);
		if (glyph) {
			int size = glyph->w * glyph->h * 3;
			unsigned char * data = (unsigned char *)malloc(size);
			memset(data, 0, size);
			for (j = 0; j < glyph->w * glyph->h; j++)
			{
				unsigned char * p = (unsigned char *)glyph->pixels;
				*(data + j * 3 + 0) = 255 - *(p + j * 4 + 3);
				*(data + j * 3 + 1) = 255 - *(p + j * 4 + 3);
				*(data + j * 3 + 2) = 255 - *(p + j * 4 + 3);
			}
			glyph2 = SDL_CreateRGBSurfaceFrom(data,
				glyph->w, glyph->h, 24, 3 * glyph->w,
				MY_Rmask, MY_Gmask, MY_Bmask, MY_Amask
				);

			sprintf(outname, "output/glyph-%d.bmp", i);
			//SDL_SaveBMP(glyph, outname);
			dumpBMPRaw(outname, glyph2->pixels, glyph2->w, glyph2->h, 1);
			SDL_FreeSurface(glyph2);
			free(data);
		}
	}
clean_end:
	TTF_Quit();
}

void image_swap_rgb(SDL_Surface *screen)
{
    uint8_t *dst = screen->pixels;
    int x, y;
    int bpp = screen->format->BytesPerPixel;
	for (y = 0; y < screen->h; y++) {
		for (x = 0; x < screen->w; x++) {
	    switch (bpp) {
			case 1:
				//FIXME:
				break;

			case 2:
				//FIXME:
				break;

			case 3:
				//dst[x * 3] = (uint8)(c);
				{
					uint8_t temp = dst[x * 3 + 0];
					dst[x * 3 + 0] = dst[x * 3 + 2];
					dst[x * 3 + 2] = temp;
				}
				break;

			case 4:
				//FIXME:
				break;
			}
		}
		dst += screen->pitch;
    }
}

void test_image(SDL_Surface* screen)
{
	SDL_Surface* surface = IMG_Load("image1.bmp");	
	SDL_Rect sr, ds;
	if (surface)
	{
		image_swap_rgb(surface);
		sr.x = 0;
		sr.y = 0;
		sr.w = surface->w;
		sr.h = surface->h;
		
		ds.x = 100;
		ds.y = 100;
		ds.w = surface->w;
		ds.h = surface->h;
		SDL_SoftStretch(surface, &sr, screen, &ds);
	}
}

