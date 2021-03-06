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

/* $Id: effect_stereoreverse.c 2936 2007-01-15 16:14:04Z icculus $ */

#include <stdio.h>
#include <stdlib.h>

//#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"

//#define __MIX_INTERNAL_EFFECT__
//#include "effects_internal.h"

static void _Eff_reversestereo16(int chan, void *stream, int len, void *udata)
{
    uint32_t *ptr = (uint32_t *) stream;
    int i;

    for (i = 0; i < len; i += sizeof (uint32_t), ptr++) {
        *ptr = (((*ptr) & 0xFFFF0000) >> 16) | (((*ptr) & 0x0000FFFF) << 16);
    }
}

static void _Eff_reversestereo8(int chan, void *stream, int len, void *udata)
{
    uint32_t *ptr = (uint32_t *) stream;
    int i;

    if (len % sizeof (uint32_t) != 0) {
        uint16_t *p = (uint16_t *) (((uint8_t *) stream) + (len - 2));
        *p = (uint16_t)((((*p) & 0xFF00) >> 8) | (((*ptr) & 0x00FF) << 8));
        len -= 2;
    }

    for (i = 0; i < len; i += sizeof (uint32_t), ptr++) {
        *ptr = (((*ptr) & 0x0000FF00) >> 8) | (((*ptr) & 0x000000FF) << 8) |
               (((*ptr) & 0xFF000000) >> 8) | (((*ptr) & 0x00FF0000) << 8);
    }
}

int Mix_SetReverseStereo(int channel, int flip)
{
    Mix_EffectFunc_t f = NULL;
    int channels;
    uint16_t format;

    Mix_QuerySpec(NULL, &format, &channels);

    if (channels == 2) {
        if ((format & 0xFF) == 16)
            f = _Eff_reversestereo16;
        else if ((format & 0xFF) == 8)
            f = _Eff_reversestereo8;
        else {
            //Mix_SetError("Unsupported audio format");
            fprintf(stderr, "%s\n", "Unsupported audio format");
			return(0);
        }

        if (!flip) {
            return(Mix_UnregisterEffect(channel, f));
        } else {
            return(Mix_RegisterEffect(channel, f, NULL, NULL));
        }
    }

    return(1);
}
