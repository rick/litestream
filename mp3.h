/*
 * Copyright (C) 2000 Gene Kan.  All rights reserved.  genehkan@xcf.berkeley.edu
 * Copyright (C) 2002 Litestream.org: 
 *                    Rick Bradley (rick@rickbradley.com), 
 *                    Sean Jewett  (sean@rimboy.com),
 *                    Brandon D. Valentine (brandon@dvalentine.com).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. The names Gene Kan, Litestream.org, Rick Bradley, Sean Jewett, or
 *    Brandon D. Valentine may not be used to endorse products derived
 *    from this software or portions of this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENE KAN, LITESTREAM.ORG, RICK BRADLEY,
 * SEAN JEWETT, AND BRANDON D. VALENTINE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROVIDERS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE OR
 * PORTIONS OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * $Id: mp3.h,v 1.2 2003/03/21 02:05:39 bandix Exp $
 */


#ifndef MP3_H

#define MP3_H

typedef struct _mp3_header {
	unsigned int sync;
	unsigned int version;
	unsigned int layer;
	unsigned int protect;
	unsigned int bitrate;
	unsigned int freq;
	unsigned int padding;
	unsigned int ext;
	unsigned int mode;
	unsigned int mode_ext;
	unsigned int copyright;
	unsigned int original;
	unsigned int emphasis;
} mp3_header;

typedef struct _mp3_frame {
	mp3_header hdr;
	unsigned char frame[8192];	/* 8192 is way larger than the max. */
	int framelen;
	long time;			/* frame duration in nanos. */
} mp3_frame;

#define explode_mp3_header(hdr,ui32) { \
	(hdr)->sync =		(ui32 & 0xffe00000) >> 21; \
	(hdr)->version =	3 - ((ui32 & 0x00180000) >> 19); \
	(hdr)->layer =		3 - ((ui32 & 0x00060000) >> 17); \
	(hdr)->protect =	(ui32 & 0x00010000) >> 16; \
	(hdr)->bitrate =	((ui32 & 0x0000f000) >> 12) - 1; \
	(hdr)->freq =		(ui32 & 0x00000c00) >> 10; \
	(hdr)->padding =	(ui32 & 0x00000200) >> 9; \
	(hdr)->ext =		(ui32 & 0x00000100) >> 8; \
	(hdr)->mode =		(ui32 & 0x000000c0) >> 6; \
	(hdr)->mode_ext =	(ui32 & 0x00000030) >> 4; \
	(hdr)->copyright =	(ui32 & 0x00000008) >> 3; \
	(hdr)->original =	(ui32 & 0x00000004) >> 2; \
	(hdr)->emphasis =	(ui32 & 0x00000003); \
}

void print_mp3_header (mp3_header* header) ;
int mp3_next_frame (FILE* _fp, mp3_frame* _frame);

#endif
