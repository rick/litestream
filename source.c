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
 * $Id: source.c,v 1.5 2005/09/06 21:26:02 roundeye Exp $
 */


#include "stream_config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "stream_sched.h"
#include "stream_inet.h"
#include "stream_std.h"

#include "http.h"
#include "icy.h"
#include "mp3.h"
#include "playlist.h"

#define MAX_FRAME_ADVANCE 5

char g_source_buf[16384];

typedef struct _source_privdata {
	int state;
#define SRC_SNDPROLOGUE	0
#define SRC_RCVACK		1
#define SRC_STREAMING	2

	FILE* fp;
	long long next_write_usecs;
	
	char prologue[MAX_MSGLEN];
} source_privdata;

stream_cli*	g_source;

int		g_time_to_die = 0;
int		g_tick = 1;

char**	g_files;
int		g_nfiles;
int		g_curfile;

int last_played = 0;

mp3_frame g_frame;

void source_cb (stream_sched* _sched, int _fd, short _events,
                char* _rbuf, int _rbytes,
                char* _wbuf, int _wbytes,
                void* _privdata) {
	stream_cli* source = (stream_cli*) _privdata;
	source_privdata* privdata = (source_privdata*) source->privdata;
	int done;
	struct timeval time_now;
	long long int time_now_usecs;
	struct timespec sleep_time;
	long diff_usecs;
	long frame_time_usecs;


	if (_events & STREAMHUP) {
		stream_msg("server disconnected");
		stream_cli_close(_sched, source);
		g_source = NULL;
		return;
	}

	for (done = 0; !done; ) {
		switch (privdata->state) {
			case SRC_SNDPROLOGUE:
				privdata->state = SRC_RCVACK;
				stream_sched_post_read(_sched, source->s,
				                       g_source_buf, sizeof(g_source_buf));
				done = 1;
				break;
			case SRC_RCVACK:
				if (!strstr(_rbuf, "\r\n\r\n")) {
					done = 1;
					break;
				}

				privdata->state = SRC_STREAMING;
				break;
			case SRC_STREAMING:
				if (!privdata->fp) {
					if (!(privdata->fp = open_next_file(g_files, g_nfiles, &g_curfile))) {
						stream_err("no valid files");
						exit(1);
					}
					break;
				}

				switch (mp3_next_frame(privdata->fp, &g_frame)) {
					case -1:
					case 1:
						fclose(privdata->fp);
						privdata->fp = NULL;
						break;
				}

				gettimeofday(&time_now, NULL);
				time_now_usecs = time_now.tv_sec * (long long int) 1000000;
				time_now_usecs += time_now.tv_usec;

				if (!privdata->next_write_usecs)
					privdata->next_write_usecs = time_now_usecs;

				frame_time_usecs = g_frame.time / 1000L;

				diff_usecs = privdata->next_write_usecs - time_now_usecs;
				/* Just don't let the stream get more than MAX_FRAME_ADVANCE
				   frames ahead. */
				if (diff_usecs > frame_time_usecs * MAX_FRAME_ADVANCE) {
					sleep_time.tv_sec = diff_usecs / 1000000;
					sleep_time.tv_nsec = (diff_usecs - sleep_time.tv_sec * 1000000) * 1000;

					while (sleep_time.tv_nsec > g_frame.time * MAX_FRAME_ADVANCE &&
					       nanosleep(&sleep_time, &sleep_time) == -1 &&
					       errno == EINTR) {
					}
				}

				privdata->next_write_usecs += (long long int) frame_time_usecs;

				stream_sched_post_write(_sched, source->s, g_frame.frame, g_frame.framelen);

				done = 1;
				break;
		}
	}
	return;
}

void sigint_handler (int x) {
	g_time_to_die = 1;
}

void sigalrm_handler (int x) {
	g_tick = 1;
}

int main (int argc, char* argv[]) {
	stream_sched* sched;
	source_privdata* privdata;

	char* host;
	int port;

	time_t time_now, last_attempt = 0;
	struct itimerval itimer = { { 10, 0 }, { 10, 0 } }; /* Every 10 seconds. */

	icy_info icy;

	playlist* list;


	if (argc < 13) {
		fprintf(stderr, "Usage: %s <ip or hostname> <port> <name> <genre> <url> <irc> <icq> <aim> <public? (0, 1)> <reported bitrate (16, 18, 56, 128, etc.)> <playlist.txt or individual mp3> <log ident>\n", argv[0]);
		return -1;
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGTRAP, SIG_IGN);
	signal(SIGINT, sigint_handler);
	signal(SIGALRM, sigalrm_handler);

	setitimer(ITIMER_REAL, &itimer, NULL);

	host = argv[1];
	sscanf(argv[2], "%d", &port);

	strcpy(icy.name, argv[3]);
	strcpy(icy.genre, argv[4]);
	strcpy(icy.url, argv[5]);
	strcpy(icy.irc, http_encode(argv[6]));
	strcpy(icy.icq, http_encode(argv[7]));
	strcpy(icy.aim, http_encode(argv[8]));
	icy.pub = atoi(argv[9]);
	strcpy(icy.br, argv[10]);

	list = read_playlist(argv[11]);

	if (!list) {
		perror("Failed reading playlist.");
		exit(1);
	}

	stream_openlog(argv[12]);
	stream_msg("source started");

	g_files = list->names;
	g_nfiles = list->used;
	g_curfile = 0;

	sched = stream_sched_new();

	privdata = (source_privdata*) malloc(sizeof(source_privdata));
	sprintf(privdata->prologue,
			"GET / HTTP/1.0\r\n"
			"icy-name: %s\r\n"
			"icy-genre: %s\r\n"
			"icy-url: %s\r\n"
			"icy-irc: %s\r\n"
			"icy-icq: %s\r\n"
			"icy-aim: %s\r\n"
			"icy-pub: %d\r\n"
			"icy-br: %s\r\n"
			"\r\n",
			icy.name,
			icy.genre,
			icy.url,
			icy.irc,
			icy.icq,
			icy.aim,
			icy.pub,
			icy.br);
	privdata->fp = NULL;

	while (!g_time_to_die) {
		if (g_tick) {

			g_tick = 0;
			time_now = time(NULL);

			if (!g_source && time_now - last_attempt > 30) {

				last_attempt = time_now;

				stream_msg("attempt reconnect");
				privdata->state = SRC_SNDPROLOGUE;
				privdata->next_write_usecs = 0;

				g_source = stream_cli_new(sched, host, port + 1, privdata, source_cb);
				if (g_source)
					stream_sched_post_write(sched,
					                        g_source->s,
					                        privdata->prologue,
					                        strlen(privdata->prologue));
			}
		}

		stream_sched_run(sched, 10000);
	}

	stream_msg("source exited normally");
	stream_closelog();

	return 0;
}
