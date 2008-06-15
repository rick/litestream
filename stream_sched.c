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
 * $Id: stream_sched.c,v 1.4 2005/09/06 21:26:02 roundeye Exp $
 */


#include "stream_config.h"

#include <sys/time.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "stream_sched.h"
#include "stream_std.h"

typedef struct _stream_buf {
	char* buf;
	int bytes;
	int len;
} stream_buf;

typedef struct _stream_fd {
	stream_sched_cb cb;
	void* privdata;
} stream_fd;

/*
 * Global file descriptor table. I cannot imagine an operating system with
 * a maximum file descriptor number greater than 100000, but if there is
 * one out there, we need to bump this number.  Not for export!
 */
int fd_setsize = FD_SETSIZE;

static stream_fd fdtable[FD_SETSIZE];
static stream_buf wbufs[FD_SETSIZE], rbufs[FD_SETSIZE];
static char fdtypes[FD_SETSIZE];
#define FDTYPE_NORMAL	0
#define FDTYPE_LISTEN	1
#define FDTYPE_CONNECT	2

/*
 * Due to the way sockets work, we NEVER turn off STREAMIN.  That's the
 * only reliable way to be notified of a hangup.  Well...if you really
 * know what you're doing...  STREAMIN can be used to throttle.
 */

stream_sched* stream_sched_new (void) {
	stream_sched* sched;

	sched = (stream_sched*) malloc(sizeof(stream_sched));

	sched->nfds = 0;
	sched->maxfd = 0;

	FD_ZERO(&sched->r);
	FD_ZERO(&sched->w);

	return(sched);
}

int stream_sched_run(stream_sched* _sched, int _to_ms) {
	struct timeval tv;
	short events;
	fd_set tr, tw;

	int i;
	int n;
	int nactive;

	int bytes;


	memcpy(&tr, &_sched->r, sizeof(fd_set));
	memcpy(&tw, &_sched->w, sizeof(fd_set));

	tv.tv_sec = _to_ms / 1000;
	tv.tv_usec = (_to_ms - tv.tv_sec * 1000) * 1000;
	nactive = select(_sched->maxfd, &tr, &tw, NULL,
	                 (_to_ms == -1) ? NULL : &tv);
	if (nactive == -1 && errno != EINTR) {
		stream_err("stream_sched_run: select");
		return(-1);
	}

	for (i = 0, n = 0; n < nactive && i < _sched->maxfd; i++) {
		events = 0;
		if (FD_ISSET(i, &tr)) {
			if (fdtypes[i] == FDTYPE_LISTEN) {
				events |= STREAMCONNECT;
				goto activate;
			}

			bytes = read(i,
			             rbufs[i].buf + rbufs[i].bytes,
			             rbufs[i].len - rbufs[i].bytes);
			switch (bytes) {
				case -1:
					if (errno != EAGAIN) {
						stream_err("read error");
						events |= STREAMHUP;
					}
					break;
				case 0:
					events |= STREAMHUP;
					break;
				default:
					rbufs[i].bytes += bytes;
					events |= STREAMIN;
					break;
			}
		}
		if (FD_ISSET(i, &tw)) {
			if (fdtypes[i] == FDTYPE_CONNECT) {
				fdtypes[i] = FDTYPE_NORMAL;
				stream_sched_set_fd_ev(_sched, i, 0);
				events |= STREAMCONNECT;
				if (!wbufs[i].len)
					goto activate;
			}

			bytes = write(i,
						  wbufs[i].buf + wbufs[i].bytes,
						  wbufs[i].len - wbufs[i].bytes);
			switch (bytes) {
				case -1:
					if (errno != EAGAIN) {
						stream_err("write error");
						events |= STREAMHUP;
					}
					break;
				case 0:
					stream_msg("Strange. 0 lengthed write.");
					break;
				default:
					wbufs[i].bytes += bytes;
					break;
			}

			if (wbufs[i].bytes == wbufs[i].len) {
				events |= STREAMOUT;
				stream_sched_sub_fd_ev(_sched, i, STREAMOUT);
			}
		}

		if (!events)
			continue;

activate:
		n++;
		(*fdtable[i].cb)(_sched, i, events,
		                 rbufs[i].buf, rbufs[i].bytes,
		                 wbufs[i].buf, wbufs[i].bytes,
		                 fdtable[i].privdata);
	}

	return(nactive);
}

/*
 * A file descriptor may belong to multiple schedulers, but it may only
 * have one callback and privdata.
 *
 * special denotes whether this is a connecting/accepting socket or just
 * a regular socket.
 *
 * Returns 0 if okay or -1 if error.  The only error is if the fd is out of
 * range.
 */
static int private_add_fd
(stream_sched* _sched, int _fd, int _fdtype, stream_sched_cb _cb,
 void* _privdata) {

	/* Protect ourselves. */
	if (_fd < 0 || _fd >= FD_SETSIZE)
		return(-1);

	fdtypes[_fd] = _fdtype;

	fdtable[_fd].cb = _cb;
	fdtable[_fd].privdata = _privdata;

	rbufs[_fd].buf = NULL; rbufs[_fd].len = 0;
	wbufs[_fd].buf = NULL; wbufs[_fd].len = 0;

	return(0);
}

int stream_sched_add_fd
(stream_sched* _sched, int _fd, stream_sched_cb _cb, void* _privdata) {
	return(private_add_fd(_sched, _fd, FDTYPE_NORMAL, _cb, _privdata));
}

int stream_sched_add_listen_fd
(stream_sched* _sched, int _fd, stream_sched_cb _cb, void* _privdata) {
	if (private_add_fd(_sched, _fd, FDTYPE_LISTEN, _cb, _privdata) == -1)
		return(-1);

	stream_sched_set_fd_ev(_sched, _fd, STREAMIN);

	return(0);
}

int stream_sched_add_connect_fd
(stream_sched* _sched, int _fd, stream_sched_cb _cb, void* _privdata) {
	if (private_add_fd(_sched, _fd, FDTYPE_CONNECT, _cb, _privdata) == -1)
		return(-1);

	stream_sched_set_fd_ev(_sched, _fd, STREAMOUT);

	return(0);
}

void stream_sched_del_fd (stream_sched* _sched, int _fd) {
	/* Protect ourselves. */
	if (_fd < 0 || _fd >= FD_SETSIZE)
		return;

	rbufs[_fd].buf = NULL;
	wbufs[_fd].buf = NULL;

	/*
	 * Do not clear the stuff out of the fdtable because a file
	 * descriptor may belong to multiple schedulers.
	 */
	stream_sched_set_fd_ev(_sched, _fd, 0);
	return;
}

void stream_sched_set_fd_ev (stream_sched* _sched, int _fd, short _events) {
	int i;


	/* Protect ourselves. */
	if (_fd < 0 || _fd >= FD_SETSIZE)
		return;

	FD_CLR(_fd, &_sched->r);
	FD_CLR(_fd, &_sched->w);

	if (_events & STREAMIN) FD_SET(_fd, &_sched->r);
	if (_events & STREAMOUT) FD_SET(_fd, &_sched->w);

	_sched->maxfd = 0;
	for (i = 0; i < FD_SETSIZE; i++) {
		if (FD_ISSET(i, &_sched->r) || FD_ISSET(i, &_sched->w))
			_sched->maxfd = i + 1;
	}
	return;
}

short stream_sched_get_fd_ev (stream_sched* _sched, int _fd) {
	short events;

	events = 0;

	if (FD_ISSET(_fd, &_sched->r)) events |= STREAMIN;
	if (FD_ISSET(_fd, &_sched->w)) events |= STREAMOUT;

	return(events);
}

void stream_sched_add_fd_ev (stream_sched* _sched, int _fd, short _events) {
	stream_sched_set_fd_ev(_sched, _fd,
						   _events | stream_sched_get_fd_ev(_sched, _fd));
	return;
}

void stream_sched_sub_fd_ev (stream_sched* _sched, int _fd, short _events) {
	stream_sched_set_fd_ev(_sched, _fd,
						   ~_events & stream_sched_get_fd_ev(_sched, _fd));
	return;
}

void stream_sched_post_read
(stream_sched* _sched, int _fd, char* _buf, int _len) {
	rbufs[_fd].buf = _buf;
	rbufs[_fd].len = _len;
	rbufs[_fd].bytes = 0;

	stream_sched_add_fd_ev(_sched, _fd, STREAMIN);
	return;
}

void stream_sched_post_write
(stream_sched* _sched, int _fd, char* _buf, int _len) {
	wbufs[_fd].buf = _buf;
	wbufs[_fd].len = _len;
	wbufs[_fd].bytes = 0;

	stream_sched_add_fd_ev(_sched, _fd, STREAMOUT);
	return;
}
