/*
    SDL_mixer:  An audio mixer library based on the SDL library
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

    This file by Ryan C. Gordon (icculus@icculus.org)

    These are some internally supported special effects that use SDL_mixer's
    effect callback API. They are meant for speed over quality.  :)
*/

/* $Id: effect_position.c 3359 2007-07-21 06:37:58Z slouken $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include "SDL.h"
#include "SDL_endian.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"
//#include "SDL_endian.h"

//#define __MIX_INTERNAL_EFFECT__
//#include "effects_internal.h"

typedef struct _Eff_positionargs
{
    volatile float left_f;
    volatile float right_f;
    volatile uint8_t left_u8;
    volatile uint8_t right_u8;
    volatile float left_rear_f;
    volatile float right_rear_f;
    volatile float center_f;
    volatile float lfe_f;
    volatile uint8_t left_rear_u8;
    volatile uint8_t right_rear_u8;
    volatile uint8_t center_u8;
    volatile uint8_t lfe_u8;
    volatile float distance_f;
    volatile uint8_t distance_u8;
    volatile int16_t room_angle;
    volatile int in_use;
    volatile int channels;
} position_args;

static position_args **pos_args_array = NULL;
static position_args *pos_args_global = NULL;
static int position_channels = 0;

void _Eff_PositionDeinit(void)
{
    int i;
    for (i = 0; i < position_channels; i++) {
        free(pos_args_array[i]);
    }

    free(pos_args_global);
    pos_args_global = NULL;
    free(pos_args_array);
    pos_args_array = NULL;
}


/* This just frees up the callback-specific data. */
static void _Eff_PositionDone(int channel, void *udata)
{
    if (channel < 0) {
        if (pos_args_global != NULL) {
            free(pos_args_global);
            pos_args_global = NULL;
        }
    }

    else if (pos_args_array[channel] != NULL) {
        free(pos_args_array[channel]);
        pos_args_array[channel] = NULL;
    }
}


static void _Eff_position_u8(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    uint8_t *ptr = (uint8_t *) stream;
    int i;

        /*
         * if there's only a mono channnel (the only way we wouldn't have
         *  a len divisible by 2 here), then left_f and right_f are always
         *  1.0, and are therefore throwaways.
         */
    if (len % sizeof (uint16_t) != 0) {
        *ptr = (uint8_t) (((float) *ptr) * args->distance_f);
        ptr++;
        len--;
    }

    if (args->room_angle == 0)
    for (i = 0; i < len; i += sizeof (uint8_t) * 2) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
    }
    else for (i = 0; i < len; i += sizeof (uint8_t) * 2) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
    }
}
static void _Eff_position_u8_c4(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    uint8_t *ptr = (uint8_t *) stream;
    int i;

        /*
         * if there's only a mono channnel (the only way we wouldn't have
         *  a len divisible by 2 here), then left_f and right_f are always
         *  1.0, and are therefore throwaways.
         */
    if (len % sizeof (uint16_t) != 0) {
        *ptr = (uint8_t) (((float) *ptr) * args->distance_f);
        ptr++;
        len--;
    }

    if (args->room_angle == 0)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
    }
    else if (args->room_angle == 90)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
    }
    else if (args->room_angle == 180)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
    }
    else if (args->room_angle == 270)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
    }
}


static void _Eff_position_u8_c6(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    uint8_t *ptr = (uint8_t *) stream;
    int i;

        /*
         * if there's only a mono channnel (the only way we wouldn't have
         *  a len divisible by 2 here), then left_f and right_f are always
         *  1.0, and are therefore throwaways.
         */
    if (len % sizeof (uint16_t) != 0) {
        *ptr = (uint8_t) (((float) *ptr) * args->distance_f);
        ptr++;
        len--;
    }

    if (args->room_angle == 0)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->center_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->lfe_f) * args->distance_f) + 128);
        ptr++;
    }
    else if (args->room_angle == 90)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f/2) + 128)
            + (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f/2) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->lfe_f) * args->distance_f) + 128);
        ptr++;
    }
    else if (args->room_angle == 180)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f/2) + 128)
            + (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f/2) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->lfe_f) * args->distance_f) + 128);
        ptr++;
    }
    else if (args->room_angle == 270)
    for (i = 0; i < len; i += sizeof (uint8_t) * 6) {
        /* must adjust the sample so that 0 is the center */
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_rear_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->right_f) * args->distance_f) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_f) * args->distance_f/2) + 128)
            + (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->left_rear_f) * args->distance_f/2) + 128);
        ptr++;
        *ptr = (uint8_t) ((int8_t) ((((float) (int8_t) (*ptr - 128)) 
            * args->lfe_f) * args->distance_f) + 128);
        ptr++;
    }
}


/*
 * This one runs about 10.1 times faster than the non-table version, with
 *  no loss in quality. It does, however, require 64k of memory for the
 *  lookup table. Also, this will only update position information once per
 *  call; the non-table version always checks the arguments for each sample,
 *  in case the user has called Mix_SetPanning() or whatnot again while this
 *  callback is running.
 */
static void _Eff_position_table_u8(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    uint8_t *ptr = (uint8_t *) stream;
    uint32_t *p;
    int i;
    uint8_t *l = ((uint8_t *) _Eff_volume_table) + (256 * args->left_u8);
    uint8_t *r = ((uint8_t *) _Eff_volume_table) + (256 * args->right_u8);
    uint8_t *d = ((uint8_t *) _Eff_volume_table) + (256 * args->distance_u8);

    if (args->room_angle == 180) {
	    uint8_t *temp = l;
	    l = r;
	    r = temp;
    }
        /*
         * if there's only a mono channnel, then l[] and r[] are always
         *  volume 255, and are therefore throwaways. Still, we have to
         *  be sure not to overrun the audio buffer...
         */
    while (len % sizeof (uint32_t) != 0) {
        *ptr = d[l[*ptr]];
        ptr++;
        if (args->channels > 1) {
            *ptr = d[r[*ptr]];
            ptr++;
        }
        len -= args->channels;
    }

    p = (uint32_t *) ptr;

    for (i = 0; i < len; i += sizeof (uint32_t)) {
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        *p = (d[l[(*p & 0xFF000000) >> 24]] << 24) |
             (d[r[(*p & 0x00FF0000) >> 16]] << 16) |
             (d[l[(*p & 0x0000FF00) >>  8]] <<  8) |
             (d[r[(*p & 0x000000FF)      ]]      ) ;
#else
        *p = (d[r[(*p & 0xFF000000) >> 24]] << 24) |
             (d[l[(*p & 0x00FF0000) >> 16]] << 16) |
             (d[r[(*p & 0x0000FF00) >>  8]] <<  8) |
             (d[l[(*p & 0x000000FF)      ]]      ) ;
#endif
        ++p;
    }
}


static void _Eff_position_s8(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    int8_t *ptr = (int8_t *) stream;
    int i;

        /*
         * if there's only a mono channnel (the only way we wouldn't have
         *  a len divisible by 2 here), then left_f and right_f are always
         *  1.0, and are therefore throwaways.
         */
    if (len % sizeof (int16_t) != 0) {
        *ptr = (int8_t) (((float) *ptr) * args->distance_f);
        ptr++;
        len--;
    }

    if (args->room_angle == 180)
    for (i = 0; i < len; i += sizeof (int8_t) * 2) {
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f);
        ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f);
        ptr++;
    }
    else
    for (i = 0; i < len; i += sizeof (int8_t) * 2) {
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f);
        ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f);
        ptr++;
    }
}
static void _Eff_position_s8_c4(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    int8_t *ptr = (int8_t *) stream;
    int i;

        /*
         * if there's only a mono channnel (the only way we wouldn't have
         *  a len divisible by 2 here), then left_f and right_f are always
         *  1.0, and are therefore throwaways.
         */
    if (len % sizeof (int16_t) != 0) {
        *ptr = (int8_t) (((float) *ptr) * args->distance_f);
        ptr++;
        len--;
    }

    for (i = 0; i < len; i += sizeof (int8_t) * 4) {
      switch (args->room_angle) {
       case 0:
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
	break;
       case 90:
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
	break;
       case 180:
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
	break;
       case 270:
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
	break;
      }
    }
}
static void _Eff_position_s8_c6(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    int8_t *ptr = (int8_t *) stream;
    int i;

        /*
         * if there's only a mono channnel (the only way we wouldn't have
         *  a len divisible by 2 here), then left_f and right_f are always
         *  1.0, and are therefore throwaways.
         */
    if (len % sizeof (int16_t) != 0) {
        *ptr = (int8_t) (((float) *ptr) * args->distance_f);
        ptr++;
        len--;
    }

    for (i = 0; i < len; i += sizeof (int8_t) * 6) {
      switch (args->room_angle) {
       case 0:
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->center_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->lfe_f) * args->distance_f); ptr++;
	break;
       case 90:
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f / 2)
           + (int8_t)((((float) *ptr) * args->right_f) * args->distance_f / 2); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->lfe_f) * args->distance_f); ptr++;
	break;
       case 180:
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f / 2)
           + (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f / 2); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->lfe_f) * args->distance_f); ptr++;
	break;
       case 270:
        *ptr = (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_rear_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->right_f) * args->distance_f); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->left_f) * args->distance_f / 2)
           + (int8_t)((((float) *ptr) * args->left_rear_f) * args->distance_f / 2); ptr++;
        *ptr = (int8_t)((((float) *ptr) * args->lfe_f) * args->distance_f); ptr++;
	break;
      }
    }
}


/*
 * This one runs about 10.1 times faster than the non-table version, with
 *  no loss in quality. It does, however, require 64k of memory for the
 *  lookup table. Also, this will only update position information once per
 *  call; the non-table version always checks the arguments for each sample,
 *  in case the user has called Mix_SetPanning() or whatnot again while this
 *  callback is running.
 */
static void _Eff_position_table_s8(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    int8_t *ptr = (int8_t *) stream;
    uint32_t *p;
    int i;
    int8_t *l = ((int8_t *) _Eff_volume_table) + (256 * args->left_u8);
    int8_t *r = ((int8_t *) _Eff_volume_table) + (256 * args->right_u8);
    int8_t *d = ((int8_t *) _Eff_volume_table) + (256 * args->distance_u8);

    if (args->room_angle == 180) {
	    int8_t *temp = l;
	    l = r;
	    r = temp;
    }


    while (len % sizeof (uint32_t) != 0) {
        *ptr = d[l[*ptr]];
        ptr++;
        if (args->channels > 1) {
            *ptr = d[r[*ptr]];
            ptr++;
        }
        len -= args->channels;
    }

    p = (uint32_t *) ptr;

    for (i = 0; i < len; i += sizeof (uint32_t)) {
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
        *p = (d[l[((int16_t)(int8_t)((*p & 0xFF000000) >> 24))+128]] << 24) |
             (d[r[((int16_t)(int8_t)((*p & 0x00FF0000) >> 16))+128]] << 16) |
             (d[l[((int16_t)(int8_t)((*p & 0x0000FF00) >>  8))+128]] <<  8) |
             (d[r[((int16_t)(int8_t)((*p & 0x000000FF)      ))+128]]      ) ;
#else
        *p = (d[r[((int16_t)(int8_t)((*p & 0xFF000000) >> 24))+128]] << 24) |
             (d[l[((int16_t)(int8_t)((*p & 0x00FF0000) >> 16))+128]] << 16) |
             (d[r[((int16_t)(int8_t)((*p & 0x0000FF00) >>  8))+128]] <<  8) |
             (d[l[((int16_t)(int8_t)((*p & 0x000000FF)      ))+128]]      ) ;
#endif
        ++p;
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


static void _Eff_position_u16lsb(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    uint16_t *ptr = (uint16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (uint16_t) * 2) {
        int16_t sampl = (int16_t) (SDL_SwapLE16(*(ptr+0)) - 32768);
        int16_t sampr = (int16_t) (SDL_SwapLE16(*(ptr+1)) - 32768);
        
        uint16_t swapl = (uint16_t) ((int16_t) (((float) sampl * args->left_f)
                                    * args->distance_f) + 32768);
        uint16_t swapr = (uint16_t) ((int16_t) (((float) sampr * args->right_f)
                                    * args->distance_f) + 32768);

	if (args->room_angle == 180) {
        	*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        	*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
	}
	else {
        	*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        	*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
	}
    }
}
static void _Eff_position_u16lsb_c4(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    uint16_t *ptr = (uint16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (uint16_t) * 4) {
        int16_t sampl = (int16_t) (SDL_SwapLE16(*(ptr+0)) - 32768);
        int16_t sampr = (int16_t) (SDL_SwapLE16(*(ptr+1)) - 32768);
        int16_t samplr = (int16_t) (SDL_SwapLE16(*(ptr+2)) - 32768);
        int16_t samprr = (int16_t) (SDL_SwapLE16(*(ptr+3)) - 32768);
        
        uint16_t swapl = (uint16_t) ((int16_t) (((float) sampl * args->left_f)
                                    * args->distance_f) + 32768);
        uint16_t swapr = (uint16_t) ((int16_t) (((float) sampr * args->right_f)
                                    * args->distance_f) + 32768);
        uint16_t swaplr = (uint16_t) ((int16_t) (((float) samplr * args->left_rear_f)
                                    * args->distance_f) + 32768);
        uint16_t swaprr = (uint16_t) ((int16_t) (((float) samprr * args->right_rear_f)
                                    * args->distance_f) + 32768);

	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
			break;
		case 90:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
			break;
		case 180:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
			break;
		case 270:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
			break;
	}
    }
}
static void _Eff_position_u16lsb_c6(int chan, void *stream, int len, void *udata)
{
    volatile position_args *args = (volatile position_args *) udata;
    uint16_t *ptr = (uint16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (uint16_t) * 6) {
        int16_t sampl = (int16_t) (SDL_SwapLE16(*(ptr+0)) - 32768);
        int16_t sampr = (int16_t) (SDL_SwapLE16(*(ptr+1)) - 32768);
        int16_t samplr = (int16_t) (SDL_SwapLE16(*(ptr+2)) - 32768);
        int16_t samprr = (int16_t) (SDL_SwapLE16(*(ptr+3)) - 32768);
        int16_t sampce = (int16_t) (SDL_SwapLE16(*(ptr+4)) - 32768);
        int16_t sampwf = (int16_t) (SDL_SwapLE16(*(ptr+5)) - 32768);

        uint16_t swapl = (uint16_t) ((int16_t) (((float) sampl * args->left_f)
                                    * args->distance_f) + 32768);
        uint16_t swapr = (uint16_t) ((int16_t) (((float) sampr * args->right_f)
                                    * args->distance_f) + 32768);
        uint16_t swaplr = (uint16_t) ((int16_t) (((float) samplr * args->left_rear_f)
                                    * args->distance_f) + 32768);
        uint16_t swaprr = (uint16_t) ((int16_t) (((float) samprr * args->right_rear_f)
                                    * args->distance_f) + 32768);
        uint16_t swapce = (uint16_t) ((int16_t) (((float) sampce * args->center_f)
                                    * args->distance_f) + 32768);
        uint16_t swapwf = (uint16_t) ((int16_t) (((float) sampwf * args->lfe_f)
                                    * args->distance_f) + 32768);

	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapce);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapwf);
			break;
		case 90:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr)/2 + (uint16_t) SDL_SwapLE16(swaprr)/2;
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapwf);
			break;
		case 180:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr)/2 + (uint16_t) SDL_SwapLE16(swaplr)/2;
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapwf);
			break;
		case 270:
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapl)/2 + (uint16_t) SDL_SwapLE16(swaplr)/2;
        		*(ptr++) = (uint16_t) SDL_SwapLE16(swapwf);
			break;
	}
    }
}

static void _Eff_position_s16lsb(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 2 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    int16_t *ptr = (int16_t *) stream;
    int i;

#if 0
    if (len % (sizeof(int16_t) * 2)) {
	    fprintf(stderr,"Not an even number of frames! len=%d\n", len);
	    return;
    }
#endif

    for (i = 0; i < len; i += sizeof (int16_t) * 2) {
        int16_t swapl = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+0))) *
                                    args->left_f) * args->distance_f);
        int16_t swapr = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+1))) *
                                    args->right_f) * args->distance_f);
	if (args->room_angle == 180) {
        	*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        	*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
	}
	else {
        	*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        	*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
	}
    }
}
static void _Eff_position_s16lsb_c4(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 4 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    int16_t *ptr = (int16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 4) {
        int16_t swapl = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+0))) *
                                    args->left_f) * args->distance_f);
        int16_t swapr = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+1))) *
                                    args->right_f) * args->distance_f);
        int16_t swaplr = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+1))) *
                                    args->left_rear_f) * args->distance_f);
        int16_t swaprr = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+2))) *
                                    args->right_rear_f) * args->distance_f);
	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
			break;
		case 90:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
			break;
		case 180:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
			break;
		case 270:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
			break;
	}
    }
}

static void _Eff_position_s16lsb_c6(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 6 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    int16_t *ptr = (int16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 6) {
        int16_t swapl = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+0))) *
                                    args->left_f) * args->distance_f);
        int16_t swapr = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+1))) *
                                    args->right_f) * args->distance_f);
        int16_t swaplr = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+2))) *
                                    args->left_rear_f) * args->distance_f);
        int16_t swaprr = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+3))) *
                                    args->right_rear_f) * args->distance_f);
        int16_t swapce = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+4))) *
                                    args->center_f) * args->distance_f);
        int16_t swapwf = (int16_t) ((((float) (int16_t) SDL_SwapLE16(*(ptr+5))) *
                                    args->lfe_f) * args->distance_f);
	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapce);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapwf);
			break;
		case 90:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr)/2 + (int16_t) SDL_SwapLE16(swaprr)/2;
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapwf);
			break;
		case 180:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr)/2 + (int16_t) SDL_SwapLE16(swaplr)/2;
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapwf);
			break;
		case 270:
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapl)/2 + (int16_t) SDL_SwapLE16(swaplr)/2;
        		*(ptr++) = (int16_t) SDL_SwapLE16(swapwf);
			break;
	}
    }
}

static void _Eff_position_u16msb(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 2 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    uint16_t *ptr = (uint16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 2) {
        int16_t sampl = (int16_t) (SDL_SwapBE16(*(ptr+0)) - 32768);
        int16_t sampr = (int16_t) (SDL_SwapBE16(*(ptr+1)) - 32768);
        
        uint16_t swapl = (uint16_t) ((int16_t) (((float) sampl * args->left_f)
                                    * args->distance_f) + 32768);
        uint16_t swapr = (uint16_t) ((int16_t) (((float) sampr * args->right_f)
                                    * args->distance_f) + 32768);

	if (args->room_angle == 180) {
        	*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        	*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
	}
	else {
        	*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        	*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
	}
    }
}
static void _Eff_position_u16msb_c4(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 4 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    uint16_t *ptr = (uint16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 4) {
        int16_t sampl = (int16_t) (SDL_SwapBE16(*(ptr+0)) - 32768);
        int16_t sampr = (int16_t) (SDL_SwapBE16(*(ptr+1)) - 32768);
        int16_t samplr = (int16_t) (SDL_SwapBE16(*(ptr+2)) - 32768);
        int16_t samprr = (int16_t) (SDL_SwapBE16(*(ptr+3)) - 32768);
        
        uint16_t swapl = (uint16_t) ((int16_t) (((float) sampl * args->left_f)
                                    * args->distance_f) + 32768);
        uint16_t swapr = (uint16_t) ((int16_t) (((float) sampr * args->right_f)
                                    * args->distance_f) + 32768);
        uint16_t swaplr = (uint16_t) ((int16_t) (((float) samplr * args->left_rear_f)
                                    * args->distance_f) + 32768);
        uint16_t swaprr = (uint16_t) ((int16_t) (((float) samprr * args->right_rear_f)
                                    * args->distance_f) + 32768);

	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
			break;
		case 90:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
			break;
		case 180:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
			break;
		case 270:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
			break;
	}
    }
}
static void _Eff_position_u16msb_c6(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 6 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    uint16_t *ptr = (uint16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 6) {
        int16_t sampl = (int16_t) (SDL_SwapBE16(*(ptr+0)) - 32768);
        int16_t sampr = (int16_t) (SDL_SwapBE16(*(ptr+1)) - 32768);
        int16_t samplr = (int16_t) (SDL_SwapBE16(*(ptr+2)) - 32768);
        int16_t samprr = (int16_t) (SDL_SwapBE16(*(ptr+3)) - 32768);
        int16_t sampce = (int16_t) (SDL_SwapBE16(*(ptr+4)) - 32768);
        int16_t sampwf = (int16_t) (SDL_SwapBE16(*(ptr+5)) - 32768);
        
        uint16_t swapl = (uint16_t) ((int16_t) (((float) sampl * args->left_f)
                                    * args->distance_f) + 32768);
        uint16_t swapr = (uint16_t) ((int16_t) (((float) sampr * args->right_f)
                                    * args->distance_f) + 32768);
        uint16_t swaplr = (uint16_t) ((int16_t) (((float) samplr * args->left_rear_f)
                                    * args->distance_f) + 32768);
        uint16_t swaprr = (uint16_t) ((int16_t) (((float) samprr * args->right_rear_f)
                                    * args->distance_f) + 32768);
        uint16_t swapce = (uint16_t) ((int16_t) (((float) sampce * args->center_f)
                                    * args->distance_f) + 32768);
        uint16_t swapwf = (uint16_t) ((int16_t) (((float) sampwf * args->lfe_f)
                                    * args->distance_f) + 32768);

	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapce);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapwf);
			break;
		case 90:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr)/2 + (uint16_t) SDL_SwapBE16(swaprr)/2;
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapwf);
			break;
		case 180:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr)/2 + (uint16_t) SDL_SwapBE16(swaplr)/2;
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapwf);
			break;
		case 270:
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapl)/2 + (uint16_t) SDL_SwapBE16(swaplr)/2;
        		*(ptr++) = (uint16_t) SDL_SwapBE16(swapwf);
			break;
	}
    }
}

static void _Eff_position_s16msb(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 2 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    int16_t *ptr = (int16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 2) {
        int16_t swapl = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+0))) *
                                    args->left_f) * args->distance_f);
        int16_t swapr = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+1))) *
                                    args->right_f) * args->distance_f);
        *(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        *(ptr++) = (int16_t) SDL_SwapBE16(swapr);
    }
}
static void _Eff_position_s16msb_c4(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 4 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    int16_t *ptr = (int16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 4) {
        int16_t swapl = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+0))) *
                                    args->left_f) * args->distance_f);
        int16_t swapr = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+1))) *
                                    args->right_f) * args->distance_f);
        int16_t swaplr = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+2))) *
                                    args->left_rear_f) * args->distance_f);
        int16_t swaprr = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+3))) *
                                    args->right_rear_f) * args->distance_f);
	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
			break;
		case 90:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
			break;
		case 180:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
			break;
		case 270:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
			break;
	}
    }
}
static void _Eff_position_s16msb_c6(int chan, void *stream, int len, void *udata)
{
    /* 16 signed bits (lsb) * 6 channels. */
    volatile position_args *args = (volatile position_args *) udata;
    int16_t *ptr = (int16_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (int16_t) * 6) {
        int16_t swapl = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+0))) *
                                    args->left_f) * args->distance_f);
        int16_t swapr = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+1))) *
                                    args->right_f) * args->distance_f);
        int16_t swaplr = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+2))) *
                                    args->left_rear_f) * args->distance_f);
        int16_t swaprr = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+3))) *
                                    args->right_rear_f) * args->distance_f);
        int16_t swapce = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+4))) *
                                    args->center_f) * args->distance_f);
        int16_t swapwf = (int16_t) ((((float) (int16_t) SDL_SwapBE16(*(ptr+5))) *
                                    args->lfe_f) * args->distance_f);

	switch (args->room_angle) {
		case 0:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapce);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapwf);
			break;
		case 90:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr)/2 + (int16_t) SDL_SwapBE16(swaprr)/2;
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapwf);
			break;
		case 180:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr)/2 + (int16_t) SDL_SwapBE16(swaplr)/2;
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapwf);
			break;
		case 270:
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaplr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swaprr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapr);
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapl)/2 + (int16_t) SDL_SwapBE16(swaplr)/2;
        		*(ptr++) = (int16_t) SDL_SwapBE16(swapwf);
			break;
	}
    }
}

static void init_position_args(position_args *args)
{
    memset(args, '\0', sizeof (position_args));
    args->in_use = 0;
    args->room_angle = 0;
    args->left_u8 = args->right_u8 = args->distance_u8 = 255;
    args->left_f  = args->right_f  = args->distance_f  = 1.0f;
    args->left_rear_u8 = args->right_rear_u8 = args->center_u8 = args->lfe_u8 = 255;
    args->left_rear_f = args->right_rear_f = args->center_f = args->lfe_f = 1.0f;
    Mix_QuerySpec(NULL, NULL, (int *) &args->channels);
}


static position_args *get_position_arg(int channel)
{
    void *rc;
    int i;

    if (channel < 0) {
        if (pos_args_global == NULL) {
            pos_args_global = malloc(sizeof (position_args));
            if (pos_args_global == NULL) {
                //Mix_SetError("Out of memory");
				fprintf(stderr, "%s\n", "Out of memory");
				return(NULL);
            }
            init_position_args(pos_args_global);
        }

        return(pos_args_global);
    }

    if (channel >= position_channels) {
        rc = realloc(pos_args_array, (channel + 1) * sizeof (position_args *));
        if (rc == NULL) {
            //Mix_SetError("Out of memory");
            fprintf(stderr, "%s\n", "Out of memory");
			return(NULL);
        }
        pos_args_array = (position_args **) rc;
        for (i = position_channels; i <= channel; i++) {
            pos_args_array[i] = NULL;
        }
        position_channels = channel + 1;
    }

    if (pos_args_array[channel] == NULL) {
        pos_args_array[channel] = (position_args *)malloc(sizeof(position_args));
        if (pos_args_array[channel] == NULL) {
            //Mix_SetError("Out of memory");
            fprintf(stderr, "%s\n", "Out of memory");
			return(NULL);
        }
        init_position_args(pos_args_array[channel]);
    }

    return(pos_args_array[channel]);
}


static Mix_EffectFunc_t get_position_effect_func(uint16_t format, int channels)
{
    Mix_EffectFunc_t f = NULL;

    switch (format) {
        case AUDIO_U8:
	    switch (channels) {
		    case 1:
		    case 2:
            		f = (_Eff_build_volume_table_u8()) ? _Eff_position_table_u8 :
                                                 		_Eff_position_u8;
	    		break;
	    	    case 4:
                        f = _Eff_position_u8_c4;
	    		break;
	    	    case 6:
                        f = _Eff_position_u8_c6;
	    		break;
	    }
            break;

        case AUDIO_S8:
	    switch (channels) {
		    case 1:
		    case 2:
            		f = (_Eff_build_volume_table_s8()) ? _Eff_position_table_s8 :
                                                 		_Eff_position_s8;
	    		break;
	    	    case 4:
                        f = _Eff_position_s8_c4;
	    		break;
	    	    case 6:
                        f = _Eff_position_s8_c6;
	    		break;
	    }
            break;

        case AUDIO_U16LSB:
	    switch (channels) {
		    case 1:
		    case 2:
            		f = _Eff_position_u16lsb;
	    		break;
	    	    case 4:
            		f = _Eff_position_u16lsb_c4;
	    		break;
	    	    case 6:
            		f = _Eff_position_u16lsb_c6;
	    		break;
	    }
            break;

        case AUDIO_S16LSB:
	    switch (channels) {
		    case 1:
		    case 2:
            		f = _Eff_position_s16lsb;
	    		break;
	    	    case 4:
            		f = _Eff_position_s16lsb_c4;
	    		break;
	    	    case 6:
            		f = _Eff_position_s16lsb_c6;
	    		break;
	    }
            break;

        case AUDIO_U16MSB:
	    switch (channels) {
		    case 1:
		    case 2:
            		f = _Eff_position_u16msb;
	    		break;
	    	    case 4:
            		f = _Eff_position_u16msb_c4;
	    		break;
	    	    case 6:
            		f = _Eff_position_u16msb_c6;
	    		break;
	    }
            break;

        case AUDIO_S16MSB:
	    switch (channels) {
		    case 1:
		    case 2:
            		f = _Eff_position_s16msb;
	    		break;
	    	    case 4:
            		f = _Eff_position_s16msb_c4;
	    		break;
	    	    case 6:
            		f = _Eff_position_s16msb_c6;
	    		break;
	    }
            break;

        default:
            //Mix_SetError("Unsupported audio format");
			fprintf(stderr, "%s\n", "Out of memory");
	}

    return(f);
}

static uint8_t speaker_amplitude[6];

static void set_amplitudes(int channels, int angle, int room_angle)
{
    int left = 255, right = 255;
    int left_rear = 255, right_rear = 255, center = 255;

        /* unwind the angle...it'll be between 0 and 359. */
    while (angle >= 360) angle -= 360;
    while (angle < 0) angle += 360;

    if (channels == 2)
    {
        /*
         * We only attenuate by position if the angle falls on the far side
         *  of center; That is, an angle that's due north would not attenuate
         *  either channel. Due west attenuates the right channel to 0.0, and
         *  due east attenuates the left channel to 0.0. Slightly east of
         *  center attenuates the left channel a little, and the right channel
         *  not at all. I think of this as occlusion by one's own head.  :)
         *
         *   ...so, we split our angle circle into four quadrants...
         */
        if (angle < 90) {
            left = 255 - ((int) (255.0f * (((float) angle) / 89.0f)));
        } else if (angle < 180) {
            left = (int) (255.0f * (((float) (angle - 90)) / 89.0f));
        } else if (angle < 270) {
            right = 255 - ((int) (255.0f * (((float) (angle - 180)) / 89.0f)));
        } else {
            right = (int) (255.0f * (((float) (angle - 270)) / 89.0f));
        }
    }

    if (channels == 4 || channels == 6)
    {
        /*
         *  An angle that's due north does not attenuate the center channel.
         *  An angle in the first quadrant, 0-90, does not attenuate the RF.
         *
         *   ...so, we split our angle circle into 8 ...
	 *
	 *             CE
	 *             0
	 *     LF      |         RF
	 *             |
	 *  270<-------|----------->90
	 *             |
	 *     LR      |         RR
	 *            180
	 *   
         */
        if (angle < 45) {
            left = ((int) (255.0f * (((float) (180 - angle)) / 179.0f)));
            left_rear = 255 - ((int) (255.0f * (((float) (angle + 45)) / 89.0f)));
            right_rear = 255 - ((int) (255.0f * (((float) (90 - angle)) / 179.0f)));
        } else if (angle < 90) {
            center = ((int) (255.0f * (((float) (225 - angle)) / 179.0f)));
            left = ((int) (255.0f * (((float) (180 - angle)) / 179.0f)));
            left_rear = 255 - ((int) (255.0f * (((float) (135 - angle)) / 89.0f)));
            right_rear = ((int) (255.0f * (((float) (90 + angle)) / 179.0f)));
        } else if (angle < 135) {
            center = ((int) (255.0f * (((float) (225 - angle)) / 179.0f)));
            left = 255 - ((int) (255.0f * (((float) (angle - 45)) / 89.0f)));
            right = ((int) (255.0f * (((float) (270 - angle)) / 179.0f)));
            left_rear = ((int) (255.0f * (((float) (angle)) / 179.0f)));
        } else if (angle < 180) {
            center = 255 - ((int) (255.0f * (((float) (angle - 90)) / 89.0f)));
            left = 255 - ((int) (255.0f * (((float) (225 - angle)) / 89.0f)));
            right = ((int) (255.0f * (((float) (270 - angle)) / 179.0f)));
            left_rear = ((int) (255.0f * (((float) (angle)) / 179.0f)));
        } else if (angle < 225) {
            center = 255 - ((int) (255.0f * (((float) (270 - angle)) / 89.0f)));
            left = ((int) (255.0f * (((float) (angle - 90)) / 179.0f)));
            right = 255 - ((int) (255.0f * (((float) (angle - 135)) / 89.0f)));
            right_rear = ((int) (255.0f * (((float) (360 - angle)) / 179.0f)));
        } else if (angle < 270) {
            center = ((int) (255.0f * (((float) (angle - 135)) / 179.0f)));
            left = ((int) (255.0f * (((float) (angle - 90)) / 179.0f)));
            right = 255 - ((int) (255.0f * (((float) (315 - angle)) / 89.0f)));
            right_rear = ((int) (255.0f * (((float) (360 - angle)) / 179.0f)));
        } else if (angle < 315) {
            center = ((int) (255.0f * (((float) (angle - 135)) / 179.0f)));
            right = ((int) (255.0f * (((float) (angle - 180)) / 179.0f)));
            left_rear = ((int) (255.0f * (((float) (450 - angle)) / 179.0f)));
            right_rear = 255 - ((int) (255.0f * (((float) (angle - 225)) / 89.0f)));
        } else {
            right = ((int) (255.0f * (((float) (angle - 180)) / 179.0f)));
            left_rear = ((int) (255.0f * (((float) (450 - angle)) / 179.0f)));
            right_rear = 255 - ((int) (255.0f * (((float) (405 - angle)) / 89.0f)));
        }
    }

    if (left < 0) left = 0; if (left > 255) left = 255;
    if (right < 0) right = 0; if (right > 255) right = 255;
    if (left_rear < 0) left_rear = 0; if (left_rear > 255) left_rear = 255;
    if (right_rear < 0) right_rear = 0; if (right_rear > 255) right_rear = 255;
    if (center < 0) center = 0; if (center > 255) center = 255;

    if (room_angle == 90) {
    	speaker_amplitude[0] = (uint8_t)left_rear;
    	speaker_amplitude[1] = (uint8_t)left;
    	speaker_amplitude[2] = (uint8_t)right_rear;
    	speaker_amplitude[3] = (uint8_t)right;
    }
    else if (room_angle == 180) {
	if (channels == 2) {
    	    speaker_amplitude[0] = (uint8_t)right;
    	    speaker_amplitude[1] = (uint8_t)left;
	}
	else {
    	    speaker_amplitude[0] = (uint8_t)right_rear;
    	    speaker_amplitude[1] = (uint8_t)left_rear;
    	    speaker_amplitude[2] = (uint8_t)right;
    	    speaker_amplitude[3] = (uint8_t)left;
	}
    }
    else if (room_angle == 270) {
    	speaker_amplitude[0] = (uint8_t)right;
    	speaker_amplitude[1] = (uint8_t)right_rear;
    	speaker_amplitude[2] = (uint8_t)left;
    	speaker_amplitude[3] = (uint8_t)left_rear;
    }
    else {
    	speaker_amplitude[0] = (uint8_t)left;
    	speaker_amplitude[1] = (uint8_t)right;
    	speaker_amplitude[2] = (uint8_t)left_rear;
    	speaker_amplitude[3] = (uint8_t)right_rear;
    }
    speaker_amplitude[4] = (uint8_t)center;
    speaker_amplitude[5] = 255;
}

int Mix_SetPosition(int channel, int16_t angle, uint8_t distance);

int Mix_SetPanning(int channel, uint8_t left, uint8_t right)
{
    Mix_EffectFunc_t f = NULL;
    int channels;
    uint16_t format;
    position_args *args = NULL;
    Mix_QuerySpec(NULL, &format, &channels);

    if (channels != 2 && channels != 4 && channels != 6)    /* it's a no-op; we call that successful. */
        return(1);

    if (channels > 2) {
        /* left = right = 255 => angle = 0, to unregister effect as when channels = 2 */
    	/* left = 255 =>  angle = -90;  left = 0 => angle = +89 */
        int angle = 0;
        if ((left != 255) || (right != 255)) {
	    angle = (int)left;
    	    angle = 127 - angle;
	    angle = -angle;
    	    angle = angle * 90 / 128; /* Make it larger for more effect? */
        }
        return( Mix_SetPosition(channel, (int16_t)angle, 0) );
    }

    f = get_position_effect_func(format, channels);
    if (f == NULL)
        return(0);

    args = get_position_arg(channel);
    if (!args)
        return(0);

        /* it's a no-op; unregister the effect, if it's registered. */
    if ((args->distance_u8 == 255) && (left == 255) && (right == 255)) {
        if (args->in_use) {
            return(Mix_UnregisterEffect(channel, f));
        } else {
	  return(1);
        }
    }

    args->left_u8 = left;
    args->left_f = ((float) left) / 255.0f;
    args->right_u8 = right;
    args->right_f = ((float) right) / 255.0f;
    args->room_angle = 0;

    if (!args->in_use) {
        args->in_use = 1;
        return(Mix_RegisterEffect(channel, f, _Eff_PositionDone, (void *) args));
    }

    return(1);
}


int Mix_SetDistance(int channel, uint8_t distance)
{
    Mix_EffectFunc_t f = NULL;
    uint16_t format;
    position_args *args = NULL;
    int channels;

    Mix_QuerySpec(NULL, &format, &channels);
    f = get_position_effect_func(format, channels);
    if (f == NULL)
        return(0);

    args = get_position_arg(channel);
    if (!args)
        return(0);

    distance = 255 - distance;  /* flip it to our scale. */

        /* it's a no-op; unregister the effect, if it's registered. */
    if ((distance == 255) && (args->left_u8 == 255) && (args->right_u8 == 255)) {
        if (args->in_use) {
            return(Mix_UnregisterEffect(channel, f));
        } else {
            return(1);
        }
    }

    args->distance_u8 = distance;
    args->distance_f = ((float) distance) / 255.0f;
    if (!args->in_use) {
        args->in_use = 1;
        return(Mix_RegisterEffect(channel, f, _Eff_PositionDone, (void *) args));
    }

    return(1);
}


int Mix_SetPosition(int channel, int16_t angle, uint8_t distance)
{
    Mix_EffectFunc_t f = NULL;
    uint16_t format;
    int channels;
    position_args *args = NULL;
    int16_t room_angle = 0;

    Mix_QuerySpec(NULL, &format, &channels);
    f = get_position_effect_func(format, channels);
    if (f == NULL)
        return(0);

        /* unwind the angle...it'll be between 0 and 359. */
    while (angle >= 360) angle -= 360;
    while (angle < 0) angle += 360;

    args = get_position_arg(channel);
    if (!args)
        return(0);

        /* it's a no-op; unregister the effect, if it's registered. */
    if ((!distance) && (!angle)) {
        if (args->in_use) {
            return(Mix_UnregisterEffect(channel, f));
        } else {
	  return(1);
	}
    }

    if (channels == 2)
    {
	if (angle > 180)
		room_angle = 180; /* exchange left and right channels */
	else room_angle = 0;
    }

    if (channels == 4 || channels == 6)
    {
	if (angle > 315) room_angle = 0;
	else if (angle > 225) room_angle = 270;
	else if (angle > 135) room_angle = 180;
	else if (angle > 45) room_angle = 90;
	else room_angle = 0;
    }


    distance = 255 - distance;  /* flip it to scale Mix_SetDistance() uses. */

    set_amplitudes(channels, angle, room_angle);

    args->left_u8 = speaker_amplitude[0];
    args->left_f = ((float) speaker_amplitude[0]) / 255.0f;
    args->right_u8 = speaker_amplitude[1];
    args->right_f = ((float) speaker_amplitude[1]) / 255.0f;
    args->left_rear_u8 = speaker_amplitude[2];
    args->left_rear_f = ((float) speaker_amplitude[2]) / 255.0f;
    args->right_rear_u8 = speaker_amplitude[3];
    args->right_rear_f = ((float) speaker_amplitude[3]) / 255.0f;
    args->center_u8 = speaker_amplitude[4];
    args->center_f = ((float) speaker_amplitude[4]) / 255.0f;
    args->lfe_u8 = speaker_amplitude[5];
    args->lfe_f = ((float) speaker_amplitude[5]) / 255.0f;
    args->distance_u8 = distance;
    args->distance_f = ((float) distance) / 255.0f;
    args->room_angle = room_angle;
    if (!args->in_use) {
        args->in_use = 1;
        return(Mix_RegisterEffect(channel, f, _Eff_PositionDone, (void *) args));
    }

    return(1);
}
