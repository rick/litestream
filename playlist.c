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
 * $Id: playlist.c,v 1.6 2005/09/06 21:26:02 roundeye Exp $
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "stream_std.h"
#include "playlist.h"

#include "textutils.h"

extern int last_played;

FILE* open_next_file(char* _files[], int _nfiles, int* _curfile) {
	int startfile = *_curfile;
	FILE* fp;
	char message[1024];

	if (last_played) 
	{
	    stream_msg("finished playlist");
	    return(NULL); 
	}

	do {
		fp = fopen(_files[*_curfile], "r");

		if (!fp) {
			sprintf(message, "could not open %s", _files[*_curfile]);
			stream_err(message);
		}
		else {
			sprintf(message, "opened %s", _files[*_curfile]);
			stream_msg(message);
		}

		*_curfile = (*_curfile + 1) % _nfiles;

		/* check for playlist wrap-around */
		if (0 == *_curfile) 
			last_played = 1;

		if (fp)
			break;

		if (*_curfile == startfile) {
			stream_msg("no reasonable files found");
			return(NULL);
		}
	} while (!fp);

	return(fp);
}

playlist* read_playlist(const char* _filename) {
	FILE* fp;
	playlist* list;

	char filename[1024];
	char comparison[1024];

	list = (playlist*) malloc(sizeof(playlist));
	list->used = 0;
	list->size = 16;
	list->names = (char**) malloc(sizeof(char*) * list->size);

	/* test for filename of *.mp3 */
	strncpy(filename, _filename, 1024);
	strncpy(comparison, stripspaces(filename, LEADING | TRAILING), 1024);
	if ((strlen(comparison) > 4) && 
	    !strncmp(comparison + (strlen(comparison)-4), ".mp3", 1020)) {
	    /* found a simple mp3 file */
	    if (list->used == list->size) {
		list->size *= 2;
		list->names = (char**) realloc(list->names, sizeof(char*) * list->size);
	    }
	    list->names[list->used++] = strdup(stripspaces(filename, LEADING | TRAILING));
	} else {
	    if (!(fp = fopen(_filename, "r")))
		return(NULL);

	    while (!feof(fp) && !ferror(fp)) {
		if (!(fgets(filename, sizeof(filename), fp)))
		    break;

		if (list->used == list->size) {
		    list->size *= 2;
		    list->names = (char**) realloc(list->names, sizeof(char*) * list->size);
		}
		list->names[list->used++] = strdup(stripspaces(filename, LEADING | TRAILING));
	    }
	    fclose(fp);
	}
	return(list);
}
