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
 * $Id: yp.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


#include "stream_config.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/param.h>

#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "stream_sched.h"
#include "stream_inet.h"
#include "stream_std.h"

#include "icy.h"
#include "http.h"
#include "textutils.h"

typedef struct _yp_privdata {
	int state;
#define YP_SND 0
#define YP_RCV 20

	char rbuf[MAX_MSGLEN];
	char wbuf[MAX_MSGLEN];
} yp_privdata;

static void yp_touch_cb
(stream_sched* _sched, int _s, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata) {
	stream_cli* yp = (stream_cli*) _privdata;
	yp_privdata* privdata = (yp_privdata*) yp->privdata;

	if (_events & STREAMHUP) {
		stream_msg("disconnected");
		stream_cli_close(_sched, yp);
		free(privdata);
		return;
	}

	switch (privdata->state) {
		case YP_SND:
			privdata->state = YP_RCV;
			stream_sched_post_read(_sched, yp->s, privdata->rbuf, sizeof(privdata->rbuf));
			break;
		case YP_RCV:
			stream_sched_post_read(_sched, yp->s, privdata->rbuf, sizeof(privdata->rbuf));
			break;
	}
	return;
}

int yp_touch (stream_sched* _sched, char* _host, int _port, char* _s) {
	stream_cli* yp;
	yp_privdata* privdata;

	privdata = (yp_privdata*) malloc(sizeof(yp_privdata));
	privdata->state = YP_SND;

	strcpy(privdata->wbuf, "GET /tchsrv?");

	strcat(privdata->wbuf, _s);

	strcat(privdata->wbuf, " HTTP/1.0\r\n");
	strcat(privdata->wbuf, "Host: ");
	strcat(privdata->wbuf, _host);
	strcat(privdata->wbuf, "\r\n");
	strcat(privdata->wbuf, "Accept: */*\r\n\r\n");

	yp = stream_cli_new(_sched, _host, _port, privdata, yp_touch_cb);
	if (!yp) {
		stream_err("dead connect");
		free(privdata);
		return(1);
	}

	stream_sched_post_write(_sched, yp->s, privdata->wbuf, strlen(privdata->wbuf));

	return(0);
}

static void yp_cb
(stream_sched* _sched, int _s, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata) {
	extern icy_response g_yp_info;

	stream_cli* yp = (stream_cli*) _privdata;
	yp_privdata* privdata = (yp_privdata*) yp->privdata;

	if (_events & STREAMHUP) {
		_rbuf[_rbytes] = '\0';

		if (strstr(_rbuf, "\r\n\r\n"))
			parse_icy_response(_rbuf, _rbytes, &g_yp_info);

		stream_cli_close(_sched, yp);
		free(privdata);
		return;
	}

	switch (privdata->state) {
		case YP_SND:
			privdata->state = YP_RCV;
			stream_sched_post_read(_sched, yp->s, privdata->rbuf, sizeof(privdata->rbuf));
			break;
		case YP_RCV:
			break;
	}
	return;
}

int yp_create (stream_sched* _sched, char* _host, int _port, icy_info* _info) {
	extern int g_strport;
	extern int g_maxlisteners;

	stream_cli* yp;
	yp_privdata* privdata;

	char* s[3];
	int i;


	privdata = (yp_privdata*) malloc(sizeof(yp_privdata));
	privdata->state = YP_SND;

	sprintf(privdata->wbuf,
			"GET /addsrv?v=1&"
			"br=%s&"
			"p=%d&"
			"m=%d&"
			"t=%s&"
			"g=%s&"
			"url=%s&"
			"irc=%s&"
			"icq=%s&"
			"aim=%s"
			" HTTP/1.0\r\n"
			"Host: %s\r\n"
			"Accept: */*\r\n\r\n",
			_info->br,
			g_strport,
			g_maxlisteners,
			s[0] = strdup(http_encode(_info->name)),
			s[1] = strdup(http_encode(_info->genre)),
			s[2] = strdup(http_encode(_info->url)),
			_info->irc,
			_info->icq,
			_info->aim,
			_host);

	for (i = 0; i < sizeof(s) / sizeof(*s); i++)
		free(s[i]);

	yp = stream_cli_new(_sched, _host, _port, privdata, yp_cb);
	if (!yp) {
		stream_err("dead connect");
		free(privdata);
		return(1);
	}

	stream_sched_post_write(_sched, yp->s, privdata->wbuf, strlen(privdata->wbuf));

	return(0);
}
