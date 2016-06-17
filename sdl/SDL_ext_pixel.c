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

#if defined(_DEBUG)
#include <stdio.h>
#endif

#include "SDL_endian.h"
#include "SDL_ext_pixel.h"
/*#include "SDL_audio.h"*/

#ifndef ABS
#define ABS(x) ((x)<0?-(x):(x))
#endif

void SDL_ext_getPixel(SDL_Surface* surface, int x, int y, 
	uint8_t* colorR, uint8_t* colorG, uint8_t* colorB, uint8_t* colorA)
{
    int bpp;
	uint8_t *p;
    unsigned int color = 0;

	bpp = surface->format->BytesPerPixel;

    SDL_LockSurface(surface);

    p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp)
    {
    case 1:
		color = *p;
        break;

    case 2:
        color = *(uint16_t *)p;
        break;

    case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			color = p[0] << 16 | p[1] << 8 | p[2];
		} 
		else 
		{
			color = p[0] | p[1] << 8 | p[2] << 16;
		}
		break;

	case 4:
		color = *(uint32_t *)p;
		break;
    }

    SDL_GetRGBA(color, surface->format, colorR, colorG, colorB, colorA);
    SDL_UnlockSurface(surface);
}

void SDL_ext_putPixel(SDL_Surface* surface, int x, int y, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB)
{
    int bpp;
	uint8_t *p;
	uint32_t pixel;

	if (x < surface->clip_rect.x || 
		x > surface->clip_rect.x + surface->clip_rect.w ||
		y < surface->clip_rect.y ||
		y > surface->clip_rect.y + surface->clip_rect.h ||
		surface->clip_rect.w == 0 ||
		surface->clip_rect.h == 0)
	{
		return;
	}

	bpp = surface->format->BytesPerPixel;

    SDL_LockSurface(surface);

    p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    pixel = SDL_MapRGB(surface->format, colorR, colorG, colorB);

    switch(bpp)
    {
      case 1:
          *p = pixel;
          break;

      case 2:
          *(uint16_t *)p = pixel;
          break;

      case 3:
          if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
          {
              p[0] = (pixel >> 16) & 0xff;
              p[1] = (pixel >> 8) & 0xff;
              p[2] = pixel & 0xff;
          }
          else
          {
              p[0] = pixel & 0xff;
              p[1] = (pixel >> 8) & 0xff;
              p[2] = (pixel >> 16) & 0xff;
          }
          break;

      case 4:
          *(uint32_t *)p = pixel;
          break;
    }

    SDL_UnlockSurface(surface);
}

uint32_t SDL_ext_Alpha32(uint32_t src, uint32_t dst, uint8_t a)
{
	uint32_t r, g, b;
    b = ((src & 0xff) * a + (dst & 0xff) * (255 - a)) >> 8;
    g = ((src & 0xff00) * a + (dst & 0xff00) * (255 - a)) >> 8;
    r = ((src & 0xff0000) * a + (dst & 0xff0000) * (255 - a)) >> 8;

    return (b & 0xff) | (g & 0xff00) | (r & 0xff0000);
}

uint16_t SDL_ext_Alpha16(uint16_t src, uint16_t dst, uint8_t a, 
	SDL_PixelFormat *f)
{
	uint32_t r, g, b;
    b = ((src & f->Rmask) * a + (dst & f->Rmask) * (255 - a)) >> 8;
    g = ((src & f->Gmask) * a + (dst & f->Gmask) * (255 - a)) >> 8;
    r = ((src & f->Bmask) * a + (dst & f->Bmask) * (255 - a)) >> 8;

    return (uint16_t)((b & f->Rmask) | (g & f->Gmask) | (r & f->Bmask));
}

void SDL_ext_putPixelAlpha(SDL_Surface* surface, int x, int y, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA)
{
    int bpp;
	uint8_t *p;
	uint32_t pixel;

	if (x < surface->clip_rect.x || 
		x > surface->clip_rect.x + surface->clip_rect.w ||
		y < surface->clip_rect.y ||
		y > surface->clip_rect.y + surface->clip_rect.h ||
		surface->clip_rect.w == 0 ||
		surface->clip_rect.h == 0)
	{
		return;
	}

	bpp = surface->format->BytesPerPixel;

    SDL_LockSurface(surface);

    p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    pixel = SDL_MapRGB(surface->format, colorR, colorG, colorB);

    switch(bpp)
    {
      case 1:
          *p = pixel;
          break;

      case 2:
          *(uint16_t *)p = SDL_ext_Alpha16((uint16_t)pixel, (uint16_t)(*(uint32_t *)p), colorA, surface->format);
          break;

      case 3:
          if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
          {
              unsigned int r = (p[0] * (255 - colorA) + colorR * colorA) >> 8;
              unsigned int g = (p[1] * (255 - colorA) + colorG * colorA) >> 8;
              unsigned int b = (p[2] * (255 - colorA) + colorB * colorA) >> 8;

              p[2] = b;
              p[1] = g;
              p[0] = r;
          }
          else
          {
              unsigned int r = (p[2] * (255 - colorA) + colorR * colorA) >> 8;
              unsigned int g = (p[1] * (255 - colorA) + colorG * colorA) >> 8;
              unsigned int b = (p[0] * (255 - colorA) + colorB * colorA) >> 8;

              p[0] = b;
              p[1] = g;
              p[2] = r;
          }
          break;

      case 4:
          *(uint32_t *)p = SDL_ext_Alpha32(pixel, *(uint32_t *)p, colorA);
          break;
    }

    SDL_UnlockSurface(surface);
}

/*===============================*/

void SDL_ext_drawPoint(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x, int y)
{
	if (x < mTarget->clip_rect.x || 
		x > mTarget->clip_rect.x + mTarget->clip_rect.w ||
		y < mTarget->clip_rect.y ||
		y > mTarget->clip_rect.y + mTarget->clip_rect.h ||
		mTarget->clip_rect.w == 0 ||
		mTarget->clip_rect.h == 0)
	{
		return;
	}

    if (colorA != 255)
    {
        SDL_ext_putPixelAlpha(mTarget, x, y, colorR, colorG, colorB, colorA);
    }
    else
    {
        SDL_ext_putPixel(mTarget, x, y, colorR, colorG, colorB);
    }
}

void SDL_ext_fillRectangle(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int areaX, int areaY, int areaWidth, int areaHeight)
{
    if (colorA != 255)
    {
        int x1 = areaX;
        int y1 = areaY;
        int x2 = areaX + areaWidth;
        int y2 = areaY + areaHeight;
        int x, y;

        SDL_LockSurface(mTarget);
        for (y = y1; y < y2; y++)
        {
            for (x = x1; x < x2; x++)
            {
                SDL_ext_putPixelAlpha(mTarget, x, y, 
					colorR, colorG, colorB, colorA);
            }
        }
        SDL_UnlockSurface(mTarget);
    }
    else
    {
        SDL_Rect rect;
        uint32_t color;
		
		rect.x = areaX;
        rect.y = areaY;
        rect.w = areaWidth;
        rect.h = areaHeight;

        color = SDL_MapRGBA(mTarget->format,
			colorR,
			colorG,
			colorB,
			colorA);
        SDL_FillRect(mTarget, &rect, color);
    }
}

void SDL_ext_drawHLine(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x1, int y, int x2)
{
    int bpp;
	uint8_t *p;
	uint32_t pixel;

	//prevent overflow
	if (y < mTarget->clip_rect.y ||
		y > mTarget->clip_rect.y + mTarget->clip_rect.h ||
		mTarget->clip_rect.w == 0 ||
		mTarget->clip_rect.h == 0)
	{
		return;
	}
	if (x1 > x2)
	{
		int temp;
		temp = x1;
		x1 = x2;
		x2 = x1;
	}
	// x1 <= x2
	if (x1 < mTarget->clip_rect.x)
	{
		x1 = mTarget->clip_rect.x;
	}
	if (x2 > mTarget->clip_rect.x + mTarget->clip_rect.w)
	{
		x2 = mTarget->clip_rect.x + mTarget->clip_rect.w;
	}

	bpp = mTarget->format->BytesPerPixel;

    SDL_LockSurface(mTarget);

    p = (uint8_t *)mTarget->pixels + y * mTarget->pitch + x1 * bpp;

    pixel = SDL_MapRGB(mTarget->format,
						colorR,
                        colorG,
                        colorB);
    switch(bpp)
    {
        case 1:
            for (;x1 <= x2; ++x1)
            {
                *(p++) = pixel;
            }
            break;
            
        case 2:
        {
            uint16_t* q = (uint16_t*)p;
            for (;x1 <= x2; ++x1)
            {
                *(q++) = pixel;
            }
            break;
        }
        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            {
                for (;x1 <= x2; ++x1)
                {
                    p[0] = (pixel >> 16) & 0xff;
                    p[1] = (pixel >> 8) & 0xff;
                    p[2] = pixel & 0xff;
                    p += 3;
                }
            }
            else
            {
                for (;x1 <= x2; ++x1)
                {
                    p[0] = pixel & 0xff;
                    p[1] = (pixel >> 8) & 0xff;
                    p[2] = (pixel >> 16) & 0xff;
                    p += 3;
                }
            }
            break;

        case 4:  
        {          
            uint32_t* q = (uint32_t*)p;
            for (;x1 <= x2; ++x1)
            {
                if (colorA != 255)
                {
                    *q = SDL_ext_Alpha32(pixel,*q,colorA);
                    q++;
                }
                else
                {
                    *(q++) = pixel;
                }
            }
            break;
        }
            
    } // end switch

    SDL_UnlockSurface(mTarget);
}


void SDL_ext_drawVLine(SDL_Surface* mTarget,
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x, int y1, int y2)
{
    int bpp;
	uint8_t *p;
	uint32_t pixel;

	//prevent overflow
	if (x < mTarget->clip_rect.x ||
		x > mTarget->clip_rect.x + mTarget->clip_rect.w ||
		mTarget->clip_rect.w == 0 ||
		mTarget->clip_rect.h == 0)
	{
		return;
	}
	if (y1 > y2)
	{
		int temp;
		temp = y1;
		y1 = y2;
		y2 = temp;
	}
	// y1 <= y2
	if (y1 < mTarget->clip_rect.y)
	{
		y1 = mTarget->clip_rect.y;
	}
	if (y2 > mTarget->clip_rect.y + mTarget->clip_rect.h)
	{
		y2 = mTarget->clip_rect.y + mTarget->clip_rect.h;
	}

	bpp = mTarget->format->BytesPerPixel;

    SDL_LockSurface(mTarget);

    p = (uint8_t *)mTarget->pixels + y1 * mTarget->pitch + x * bpp;

    pixel = SDL_MapRGB(mTarget->format, colorR, colorG, colorB);

    switch (bpp)
    {            
      case 1:
          for (;y1 <= y2; ++y1)
          {
              *p = pixel;
              p += mTarget->pitch;
          }
          break;

      case 2:
          for (;y1 <= y2; ++y1)
          {
              *(uint16_t*)p = pixel;
              p += mTarget->pitch;
          }
          break;

      case 3:
          if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
          {
              for (;y1 <= y2; ++y1)
              {
                  p[0] = (pixel >> 16) & 0xff;
                  p[1] = (pixel >> 8) & 0xff;
                  p[2] = pixel & 0xff;
                  p += mTarget->pitch;
              }
          }
          else
          {
              for (;y1 <= y2; ++y1)
              {
                  p[0] = pixel & 0xff;
                  p[1] = (pixel >> 8) & 0xff;
                  p[2] = (pixel >> 16) & 0xff;
                  p += mTarget->pitch;
              }
          }
          break;

      case 4:
          for (;y1 <= y2; ++y1)
          {
              if (colorA != 255)
              {
                  *(uint32_t*)p = SDL_ext_Alpha32(pixel,*(uint32_t*)p,colorA);
              }
              else
              {
                  *(uint32_t*)p = pixel;
              }
              p += mTarget->pitch;
          }
          break;
    } 
	
    SDL_UnlockSurface(mTarget);
}

void SDL_ext_drawLine(SDL_Surface* mTarget,
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int x1, int y1, int x2, int y2)
{
	int dx, dy;

    if (x1 == x2)
    {
        SDL_ext_drawVLine(mTarget, colorR, colorG, colorB, colorA, x1, y1, y2);
        return;
    }
    if (y1 == y2)
    {
        SDL_ext_drawHLine(mTarget, colorR, colorG, colorB, colorA, x1, y1, x2);
        return;
    }

    // Draw a line with Bresenham

    dx = ABS(x2 - x1);
    dy = ABS(y2 - y1);

    if (dx > dy)
    {
        if (x1 > x2)
        {
            // swap x1, x2
            x1 ^= x2;
            x2 ^= x1;
            x1 ^= x2;

            // swap y1, y2
            y1 ^= y2;
            y2 ^= y1;
            y1 ^= y2;
        }

        if (y1 < y2)
        {
            int y = y1;
            int p = 0;
			int x;

            for (x = x1; x <= x2; x++)
            {
                if (colorA != 255)
                {
                    SDL_ext_putPixelAlpha(mTarget, x, y, colorR, colorG, colorB, colorA);
                }
                else
                {
                    SDL_ext_putPixel(mTarget, x, y, colorR, colorG, colorB);
                }

                p += dy;

                if (p * 2 >= dx)
                {
                    y++;
                    p -= dx;
                }
            }
        }
        else
        {
            int y = y1;
            int p = 0;
			int x;

            for (x = x1; x <= x2; x++)
            {
                if (colorA != 255)
                {
                    SDL_ext_putPixelAlpha(mTarget, x, y, colorR, colorG, colorB, colorA);
                }
                else
                {
                    SDL_ext_putPixel(mTarget, x, y, colorR, colorG, colorB);
                }
				
                p += dy;

                if (p * 2 >= dx)
                {
                    y--;
                    p -= dx;
                }
            }
        }
    }
    else
    {
        if (y1 > y2)
        {
            // swap y1, y2
            y1 ^= y2;
            y2 ^= y1;
            y1 ^= y2;

            // swap x1, x2
            x1 ^= x2;
            x2 ^= x1;
            x1 ^= x2;
        }

        if (x1 < x2)
        {
            int x = x1;
            int p = 0;
			int y;

            for (y = y1; y <= y2; y++)
            {
                if (colorA != 255)
                {
                    SDL_ext_putPixelAlpha(mTarget, x, y, colorR, colorG, colorB, colorA);
                }
                else
                {
                    SDL_ext_putPixel(mTarget, x, y, colorR, colorG, colorB);
                }
				
                p += dx;

                if (p * 2 >= dy)
                {
                    x++;
                    p -= dy;
                }
            }
        }
        else
        {
            int x = x1;
            int p = 0;
			int y;

            for (y = y1; y <= y2; y++)
            {
                if (colorA != 255)
                {
                    SDL_ext_putPixelAlpha(mTarget, x, y, colorR, colorG, colorB, colorA);
                }
                else
                {
                    SDL_ext_putPixel(mTarget, x, y, colorR, colorG, colorB);
                }

                p += dx;

                if (p * 2 >= dy)
                {
                    x--;
                    p -= dy;
                }
            }
        }
    }
}

void SDL_ext_drawRectangle(SDL_Surface* mTarget, 
	uint8_t colorR, uint8_t colorG, uint8_t colorB, uint8_t colorA, 
	int areaX, int areaY, int areaWidth, int areaHeight)
{
    int x1 = areaX;
    int x2 = areaX + areaWidth - 1;
    int y1 = areaY;
    int y2 = areaY + areaHeight - 1;

    SDL_ext_drawHLine(mTarget, colorR, colorG, colorB, colorA, 
		x1, y1, x2);
    SDL_ext_drawHLine(mTarget, colorR, colorG, colorB, colorA, 
		x1, y2, x2);

    SDL_ext_drawVLine(mTarget, colorR, colorG, colorB, colorA, 
		x1, y1, y2);
    SDL_ext_drawVLine(mTarget, colorR, colorG, colorB, colorA, 
		x2, y1, y2);
}

void SDL_ext_drawImage(SDL_Surface* mTarget, 
	SDL_Surface* srcSurface, 
	int srcX, int srcY, int dstX, int dstY,
    int width, int height)
{
    SDL_Rect src;
    SDL_Rect dst;

    src.x = srcX;
    src.y = srcY;
    src.w = width;
    src.h = height;
    dst.x = dstX;
    dst.y = dstY;
    dst.w = width;
    dst.h = height;

#if defined(_DEBUG)
	fprintf(stderr, "srcSurface->pixels == %x\n", srcSurface->pixels);
	fprintf(stderr, "mTarget->pixels == %x\n", mTarget->pixels);
#endif

    SDL_BlitSurface(srcSurface, &src, mTarget, &dst);
}

