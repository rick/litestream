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
 * $Id: client.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
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
#include <signal.h>

#include "stream_sched.h"
#include "stream_inet.h"
#include "stream_std.h"

extern void hexdump (unsigned char* buf, int len);

char trashbuf[524288];

typedef struct _client_privdata {
	int state;
#define SNDREQ	0
#define RCVREQ	20
	
	char wbuf[163840];
} client_privdata;

void client_cb (stream_sched* _sched, int _fd, short _events,
                char* _rbuf, int _rbytes,
                char* _wbuf, int _wbytes,
                void* _privdata) {
	stream_cli* client = (stream_cli*) _privdata;
	client_privdata* privdata = (client_privdata*) client->privdata;


	if (_events & STREAMHUP) {
		stream_msg("server disconnected");
		stream_cli_close(_sched, client);
		free(privdata);
		return;
	}

	switch (privdata->state) {
		case SNDREQ:
			privdata->state = RCVREQ;
			stream_sched_post_read(_sched, client->s, trashbuf, sizeof(trashbuf));
			break;
		case RCVREQ:
			/*hexdump(trashbuf, _rbytes);*/
			stream_sched_post_read(_sched, client->s, trashbuf, sizeof(trashbuf));

			break;
	}
	return;
}

void sigpipe_handler(int sig) {
	printf("SIGPIPE\n");
}

int main (int argc, char* argv[]) {
	stream_sched* sched;
	stream_cli* client;
	client_privdata* privdata;
	int i;

	char* host;
	int port;
	int clients;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s <ip or hostname> <port> <n clients>\n", argv[0]);
		return(-1);
	}

	signal(SIGPIPE, sigpipe_handler);

	host = argv[1];
	sscanf(argv[2], "%d", &port);
	sscanf(argv[3], "%d", &clients);

	sched = stream_sched_new();

	for (i = 0; i < clients; i++) {
		privdata = (client_privdata*) malloc(sizeof(client_privdata));
		strcpy(privdata->wbuf, "GET /\r\nUser-Agent: Winamp\r\n\r\n");
		privdata->state = SNDREQ;

		client = stream_cli_new(sched, host, port, privdata, client_cb);
		if (!client) {
			stream_err("client");
			return(1);
		}

		stream_sched_post_write(sched, client->s, privdata->wbuf, strlen(privdata->wbuf));
	}

	for ( ; ; )
		stream_sched_run(sched, -1);
}
