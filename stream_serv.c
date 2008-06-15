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
 * $Id: stream_serv.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
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

#include "stream_sched.h"
#include "stream_inet.h"
#include "stream_std.h"

stream_serv* stream_serv_new
(stream_sched* _sched, u_int16_t _port, int _backlog, void* _privdata, stream_sched_cb _cb) {
	stream_serv* server;
	struct sockaddr_in sin;
	int one = 1;
	struct linger linger = { 0, 0 };

	server = (stream_serv*) malloc(sizeof(stream_serv));

	server->s = socket(PF_INET, SOCK_STREAM, 0);
	if (server->s == -1) {
		stream_err("socket");
		goto bad;
	}
	server->privdata = _privdata;

	if (fcntl(server->s, F_SETFL, O_NONBLOCK) == -1) {
		stream_err("fcntl");
		goto bad1;
	}

	if (setsockopt(server->s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1) {
		stream_err("setsockopt reuseaddr");
		goto bad1;
	}
	if (setsockopt(server->s, SOL_SOCKET, SO_OOBINLINE, &one, sizeof(one)) == -1) {
		stream_err("setsockopt oobinline");
		goto bad1;
	}
	if (setsockopt(server->s, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof(one)) == -1) {
		stream_err("setsockopt keepalive");
		goto bad1;
	}
	if (setsockopt(server->s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
		stream_err("setsockopt linger");
		goto bad1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(_port);
	if (bind(server->s, (struct sockaddr*) &sin, sizeof(sin)) == -1) {
		stream_err("bind");
		goto bad1;
	}

	if (listen(server->s, _backlog) == -1) {
		stream_err("listen");
		goto bad1;
	}

	if (stream_sched_add_listen_fd(_sched, server->s, _cb, server) == -1) {
		stream_err("add_listen_fd");
		goto bad1;
	}

	return(server);

bad1:	close(server->s);
bad:	free(server);
	return(NULL);
}

void stream_serv_close (stream_sched* _sched, stream_serv* _server) {
	stream_sched_del_fd(_sched, _server->s);
	close(_server->s);
	free(_server);
	return;
}

stream_serv* stream_serv_accept
(stream_sched* _sched, int _s, stream_sched_cb _cb, void* _privdata) {
	int news;
	stream_serv* server;

	server = (stream_serv*) malloc(sizeof(stream_serv));

	server->addrlen = sizeof(server->addr);
	news = accept(_s, (struct sockaddr*) &server->addr, &server->addrlen);

	if (news == -1)
		goto bad;

	server->s = news;
	if (stream_sched_add_fd(_sched, server->s, _cb, server) == -1) {
		stream_msg("out of file descriptors\n");
		goto bad;
	}

	server->privdata = _privdata;

	return(server);

bad:
	free(server);
	return(NULL);
}
