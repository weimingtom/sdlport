#include "common.h"

#include <SDL_video.h>
#include <malloc.h>
#include <stdio.h>
#include "TextureLoader.h"
#include "test.h"


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

