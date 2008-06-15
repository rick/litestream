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
 * $Id: stream_cli.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


#include "stream_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#include "stream_sched.h"
#include "stream_inet.h"
#include "stream_std.h"

stream_cli* stream_cli_new
(stream_sched* _sched, char* _name_or_ip, u_int16_t _port, void* _privdata, stream_sched_cb _cb) {
	stream_cli* client;
	struct sockaddr_in addr;
	struct hostent* hent;
	int one = 1;
	struct linger linger = { 0, 0 };


	client = (stream_cli*) malloc(sizeof(stream_cli));

	client->s = socket(AF_INET, SOCK_STREAM, 0);
	if (client->s == -1) {
		stream_err("socket");
		goto bad;
	}
	client->privdata = _privdata;

	if (fcntl(client->s, F_SETFL, O_NONBLOCK) == -1) {
		stream_err("fcntl");
		goto bad1;
	}

	if (setsockopt(client->s, SOL_SOCKET, SO_OOBINLINE, &one, sizeof(one)) == -1) {
		stream_err("setsockopt oobinline");
		goto bad1;
	}
	if (setsockopt(client->s, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one)) == -1) {
		stream_err("setsockopt keepalive");
		goto bad1;
	}
	if (setsockopt(client->s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
		stream_err("setsockopt linger");
		goto bad1;
	}

	hent = gethostbyname(_name_or_ip);
	if (!hent)
		addr.sin_addr.s_addr = inet_addr(_name_or_ip);
	else
		memcpy(&addr.sin_addr, hent->h_addr, hent->h_length);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(_port);

	if (connect(client->s, (struct sockaddr*) &addr, sizeof(addr)) == -1) {
		if (errno != EINPROGRESS) {
			stream_err("connect");
			goto bad1;
		}
	}

	if (stream_sched_add_connect_fd(_sched, client->s, _cb, client) == -1) {
		stream_err("add_connect_fd");
		goto bad1;
	}

	return(client);

bad1:	close(client->s);
bad:	free(client);
	return(NULL);
}

void stream_cli_close (stream_sched* _sched, stream_cli* _client) {
	stream_sched_del_fd(_sched, _client->s);
	close(_client->s);
	free(_client);
	return;
}
