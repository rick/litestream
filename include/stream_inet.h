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
 * $Id: stream_inet.h,v 1.2 2003/03/21 02:05:40 bandix Exp $
 */


#ifndef STREAM_INET_H

#define STREAM_INET_H

/*

#include <sys/types.h>
#include <sys/socket.h>

*/

#define in2ascii(saddr_in) inet_ntoa(saddr_in.sin_addr)

/*

   This stuff is designed to run with stream_scheduler.

   This is a little spaghetti.  Could be fixed with another level of
   indirection and *UGH* an extra callback layer.

*/

/* fdx is the index at which the server/client socket is held in the
   stream_sched. */

typedef struct _stream_serv {
	int s;
	struct sockaddr_in addr;
	int addrlen;

	void* privdata;
} stream_serv;

stream_serv* stream_serv_new
(stream_sched* _sched, u_int16_t _port, int _backlog,  void* _privdata, stream_sched_cb _cb);
stream_serv* stream_serv_accept (stream_sched* _sched, int _fd, stream_sched_cb _cb, void* _privdata);
void stream_serv_close (stream_sched* _cb, stream_serv* _server);

typedef struct _stream_cli {
	int s;
	void* privdata;
} stream_cli;

stream_cli* stream_cli_new
(stream_sched* _sched, char* _name_or_ip, u_int16_t _port, void* _privdata, stream_sched_cb _cb);
void stream_cli_close (stream_sched* _sched, stream_cli* _client);

#endif /* STREAM_INET_H */
