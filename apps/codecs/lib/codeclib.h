/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef __CODECLIB_H__
#define __CODECLIB_H__

#include "config.h"
#include "codecs.h"
#include <sys/types.h>

extern struct codec_api *ci;
extern size_t mem_ptr;
extern size_t bufsize;
extern unsigned char* mp3buf;     /* The actual MP3 buffer from Rockbox                 */
extern unsigned char* mallocbuf;  /* The free space after the codec in the codec buffer */
extern unsigned char* filebuf;    /* The rest of the MP3 buffer                         */

/* Standard library functions that are used by the codecs follow here */

/* Get these functions 'out of the way' of the standard functions. Not doing
 * so confuses the cygwin linker, and maybe others. These functions need to
 * be implemented elsewhere */
#define malloc(x) codec_malloc(x)
#define calloc(x,y) codec_calloc(x,y)
#define realloc(x,y) codec_realloc(x,y)
#define free(x) codec_free(x)
#define alloca(x) __builtin_alloca(x)

void* codec_malloc(size_t size);
void* codec_calloc(size_t nmemb, size_t size);
void* codec_realloc(void* ptr, size_t size);
void codec_free(void* ptr);

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
void *memmove(void *s1, const void *s2, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
int strcmp(const char *, const char *);

void qsort(void *base, size_t nmemb, size_t size, int(*compar)(const void *, const void *));

/*MDCT library functions*/

extern void mdct_backward(int n, int32_t *in, int32_t *out);

#if defined(CPU_ARM) && (ARM_ARCH == 4)
/* optimised unsigned integer division for ARMv4, in IRAM */
unsigned udiv32_arm(unsigned a, unsigned b);
#define UDIV32(a, b) udiv32_arm(a, b)
#else
/* default */
#define UDIV32(a, b) (a / b)
#endif

/* TODO figure out if we really need to care about calculating
   av_log2(0) */
#if (defined(CPU_ARM) && (ARM_ARCH > 4))
static inline unsigned int av_log2(uint32_t v)
{
    unsigned int lz = __builtin_clz(v);
    return 31 - lz + (lz >> 5); /* make sure av_log2(0) returns 0 */
}
#else
/* From libavutil/common.h */
extern const uint8_t ff_log2_tab[256] ICONST_ATTR;

static inline unsigned int av_log2(unsigned int v)
{
    int n;

    n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}
#endif

/* Various codec helper functions */

int codec_init(void);
void codec_set_replaygain(struct mp3entry* id3);

#ifdef RB_PROFILE
void __cyg_profile_func_enter(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;
void __cyg_profile_func_exit(void *this_fn, void *call_site)
    NO_PROF_ATTR ICODE_ATTR;
#endif

#endif /* __CODECLIB_H__ */
