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
 * $Id: server.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


#include "stream_config.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "stream_sched.h"
#include "stream_inet.h"
#include "stream_std.h"

extern void hexdump (unsigned char* buf, int len);

typedef struct _server_privdata {
	char buf[16384];
} server_privdata;

void server_cb
(stream_sched* _sched, int _fd, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata) {
	stream_serv* server = (stream_serv*) _privdata;
	server_privdata* privdata = (server_privdata*) server->privdata;

	if (_events & STREAMHUP) {
		stream_msg("client disconnected");
		stream_serv_close(_sched, server);
		free(privdata);
		return;
	}

	_rbuf[_rbytes] = '\0';

	hexdump(_rbuf, _rbytes);
	return;
}

void listen_cb
(stream_sched* _sched, int _fd, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata) {
	stream_serv* server;
	server_privdata* privdata;


	stream_msg("connected");

	privdata = (server_privdata*) malloc(sizeof(server_privdata));

	server = stream_serv_accept(_sched,
				    _fd,
				    server_cb,
				    privdata);

	if (!server) {
		stream_err("dead connect\n");
		return;
	}

	stream_sched_post_read(_sched, server->s, privdata->buf, sizeof(privdata->buf));
	return;
}

int main (int argc, char* argv[]) {
	stream_sched* sched;
	stream_serv* server;
	int port;


	if (argc != 2) {
		fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		return -1;
	}

	port = atoi(argv[1]);

	sched = stream_sched_new();

	server = stream_serv_new(sched, port, 100, NULL, listen_cb);
	if (!server) {
		stream_err("server create");
		return 1;
	}

	for ( ; ; )
		stream_sched_run(sched, -1);
}
