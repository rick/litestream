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
 * $Id: stream_sched.h,v 1.2 2003/03/21 02:05:40 bandix Exp $
 */


#ifndef STREAM_SCHED_H

#define STREAM_SCHED_H

/*

#include "stream_config.h"
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

*/

#ifndef STREAM_CONFIG_H
#error Must include stream_config.h before anything else!
#endif

#define STREAMIN	0x1
#define STREAMOUT	0x2
/*#define STREAMOOB	0x4*/
#define STREAMHUP	0x8
#define STREAMCONNECT	0x10

typedef struct _stream_sched stream_sched;

typedef void (*stream_sched_cb)
(stream_sched* _sched, int _fd, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata);

struct _stream_sched {
	int nfds;	/* Number of file descriptors in this scheduler.
			   Just for book keeping. */

	fd_set r, w;
	int maxfd;
};

stream_sched* stream_sched_new (void);
int stream_sched_run (stream_sched* _sched, int _to_ms);
int stream_sched_add_fd
(stream_sched* _sched, int _fd, stream_sched_cb _cb, void* _privdata);
int stream_sched_add_listen_fd
(stream_sched* _sched, int _fd, stream_sched_cb _cb, void* _privdata);
int stream_sched_add_connect_fd
(stream_sched* _sched, int _fd, stream_sched_cb _cb, void* _privdata);
void stream_sched_del_fd (stream_sched* _sched, int _fd);
void stream_sched_set_fd_ev (stream_sched* _sched, int _fd, short _events);
short stream_sched_get_fd_ev (stream_sched* _sched, int _fd);
void stream_sched_add_fd_ev (stream_sched* _sched, int _fd, short _events);
void stream_sched_sub_fd_ev (stream_sched* _sched, int _fd, short _events);
void stream_sched_post_read
(stream_sched* _sched, int _fd, char* _buf, int _len);
void stream_sched_post_write
(stream_sched* _sched, int _fd, char* _buf, int _len);

#endif /* STREAM_SCHED_H */
