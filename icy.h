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
 * $Id: icy.h,v 1.2 2003/03/21 02:05:38 bandix Exp $
 */


#ifndef ICY_H

#define ICY_H

#define ICY_FIELDSIZE 1024

typedef struct _icy_info {
	char	name[ICY_FIELDSIZE];
	char	genre[ICY_FIELDSIZE];
	char	url[ICY_FIELDSIZE];
	char	irc[ICY_FIELDSIZE];
	char	icq[ICY_FIELDSIZE];
	char	aim[ICY_FIELDSIZE];

	int	pub;

	char	br[ICY_FIELDSIZE];
} icy_info;

typedef struct _icy_response {
	char	response[ICY_FIELDSIZE];
	char	id[ICY_FIELDSIZE];
	int	tchfrq;
} icy_response;

void parse_icy_info (char* _buf, int _len, icy_info* _info);
void parse_icy_response (char* _buf, int _len, icy_response* _resp);

#endif
