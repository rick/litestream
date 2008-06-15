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
 * $Id: icy.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "textutils.h"
#include "icy.h"

void parse_icy_info (char* _buf, int _len, icy_info* _info) {
	char* tok, *last;

	memset(_info, 0, sizeof(*_info));

	for (tok = strtok_r(_buf, "\r\n", &last);
	     tok;
	     tok = strtok_r(NULL, "\r\n", &last)) {
		char* tok1, *last1;

		tok1 = strtok_r(tok, ":", &last1);
		if (!strcmp(tok1, "icy-name"))
			stripspaces(strcpy(_info->name, strtok_r(NULL, "", &last1) ? : ""), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-genre"))
			stripspaces(strcpy(_info->genre, strtok_r(NULL, "", &last1) ? : ""), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-url"))
			stripspaces(strcpy(_info->url, strtok_r(NULL, "", &last1) ? : ""), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-irc"))
			stripspaces(strcpy(_info->irc, strtok_r(NULL, "", &last1) ? : ""), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-icq"))
			stripspaces(strcpy(_info->icq, strtok_r(NULL, "", &last1) ? : ""), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-aim"))
			stripspaces(strcpy(_info->aim, strtok_r(NULL, "", &last1) ? : ""), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-pub"))
			_info->pub = atoi(strtok_r(NULL, "", &last1) ? : "0");
		else if (!strcmp(tok1, "icy-br"))
			stripspaces(strcpy(_info->br, strtok_r(NULL, "", &last1) ? : ""), LEADING | TRAILING);
	}
}

void parse_icy_response (char* _buf, int _len, icy_response* _resp) {
	char* tok, *last;

	memset(_resp, 0, sizeof(*_resp));

	_resp->tchfrq = 3;	/* Default is 3 minutes delay between touches. */

	for (tok = strtok_r(_buf, "\r\n", &last);
	     tok;
	     tok = strtok_r(NULL, "\r\n", &last)) {
		char* tok1, *last1;

		tok1 = strtok_r(tok, ":", &last1);
		if (!strcmp(tok1, "icy-response"))
			stripspaces(strcpy(_resp->response, strtok_r(NULL, "", &last1)), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-id"))
			stripspaces(strcpy(_resp->id, strtok_r(NULL, "", &last1)), LEADING | TRAILING);
		else if (!strcmp(tok1, "icy-tchfrq"))
			_resp->tchfrq = atoi(strtok_r(NULL, "", &last1));
	}
}
