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
 * $Id: mp3.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


#include <sys/types.h>

#include <stdio.h>

#include <netinet/in.h>

#include "mp3.h"

int mp3_bitrates[2][3][14] = {
	/* MPEG 1.0 */
	{
		{ 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
		{ 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 },
		{ 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
	},
	/* MPEG 2.0 */
	{
		{ 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 },
		{ 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 },
		{ 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160 },
	},
};

int mp3_freqs[4][3] = {
	/* MPEG 1.0 */ { 44100, 48000, 32000 },
	/* MPEG 2.0 */ { 22050, 24000, 16000 },
	/* MPEG 1.5 */ { 22050, 24000, 16000 },
	/* MPEG 2.5 */ { 11025, 12000, 8000 },
};

int mp3_sizemultiple[3] = { 48000, 144000, 144000 };

void print_mp3_header (mp3_header* header) {
	printf("sync: %x\n"
	       "version: %u\n"
		   "layer: %u\n"
		   "protect: %u\n"
		   "bitrate: %u\n"
		   "freq: %u\n"
		   "pad: %u\n"
		   "ext: %u\n"
		   "mode: %u\n"
		   "mode_ext: %u\n"
		   "copyright: %u\n"
		   "original: %u\n"
		   "emphasis: %u\n"
		   "framesize: %u\n",
	       header->sync,
	       header->version,
	       header->layer,
		   header->protect,
	       mp3_bitrates[header->version][header->layer][header->bitrate],
	       mp3_freqs[header->version][header->freq],
	       header->padding,
		   header->ext,
		   header->mode,
		   header->mode_ext,
		   header->copyright,
		   header->original,
		   header->emphasis,
	       (mp3_sizemultiple[header->layer] * mp3_bitrates[header->version][header->layer][header->bitrate]) / mp3_freqs[header->version][header->freq] + header->padding);
	       return;
}

/* 0 is okay. 1 is a file error. 2 is a corrupt file (or ID3?). */
int mp3_next_frame (FILE* _fp, mp3_frame* _frame) {
	u_int32_t ui32;
	int bitrate;
	double frametime;
	int okay;

	/* Look for a valid frame. */
	do {
		okay = 1;

		if (fread(&ui32, sizeof(ui32), 1, _fp) != 1) /* EOF or error */
			return(1);

		ui32 = ntohl(ui32);
		explode_mp3_header(&_frame->hdr, ui32);

		if (_frame->hdr.sync != 0x7ff)
			okay = 0;
		if (_frame->hdr.version > 1)
			okay = 0;
		if (_frame->hdr.layer > 2)
			okay = 0;
		if (_frame->hdr.bitrate > 13)
			okay = 0;

		if (!okay && fseek(_fp, -3, SEEK_CUR) == -1)
			return(1);
	} while (!okay);

	bitrate = mp3_bitrates[_frame->hdr.version][_frame->hdr.layer][_frame->hdr.bitrate];

	_frame->framelen =
		(mp3_sizemultiple[_frame->hdr.layer] * bitrate) /
		mp3_freqs[_frame->hdr.version][_frame->hdr.freq] +
		_frame->hdr.padding;

	/* frametime = framelen * 0.008(bits/byte) / bitrate(kilobits/sec) * 1000000000(nanos/sec) */

	frametime = (_frame->framelen * 8000000.0) / bitrate;
	_frame->time = (long) frametime;

	if (fseek(_fp, -1 * sizeof(ui32), SEEK_CUR) == -1)
		return(1);
	if (fread(_frame->frame, _frame->framelen, 1, _fp) != 1)
		return(1);

	return(0);
}
