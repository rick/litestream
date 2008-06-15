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
 * $Id: restream.c,v 1.6 2005/09/06 21:26:02 roundeye Exp $
 */

#include "stream_config.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/param.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>

#include "stream_sched.h"
#include "stream_inet.h"
#include "stream_std.h"

#include "icy.h"
#include "yp.h"

int		g_time_to_die = 0;

time_t	g_start_time;
int		g_tick = 1;

unsigned char	g_source_buf[131072];

icy_info		g_icy_info;

char*	g_mirrortag;

char*	g_srchost;
int		g_srcport;
char*	g_strhost;
int		g_strport;

int		g_maxlisteners;

char*	g_yp_host = NULL;
int		g_yp_port;
icy_response	g_yp_info;

stream_cli*	g_source = NULL;

typedef struct _source_privdata {
	int state;
#define SRC_SNDPROLOGUE		0
#define SRC_RCVPROLOGUEACK	20
#define SRC_STREAMING		40
	
	char rbuf[MAX_MSGLEN];
	char wbuf[MAX_MSGLEN];
} source_privdata;

typedef struct _server_stats {
	unsigned long long int total_stream_received;
	unsigned long long int total_stream_sent;
	unsigned long long int overruns;
	unsigned long long int overrun_bytes;

	int peak_servers;
	int total_servers;
} server_stats;

server_stats stats;

typedef struct _server_privdata {
	int state;
#define STR_RCVREQUEST	0
#define STR_SNDHEADER	1
#define STR_SNDREPLY	2
#define STR_STREAMING	3

	char rbuf[MAX_MSGLEN];
	char wbuf[MAX_MSGLEN];
} server_privdata;

stream_serv* strservers[FD_SETSIZE];
int nstrservers = 0;

void clear_strtable (stream_serv* _server) {
	int i;

	for (i = 0; i < nstrservers; i++) {
		if (strservers[i] == _server)
			break;
	}
	if (i == nstrservers) {
		stream_msg("failed to find");
		return;
	}

	nstrservers--;

	memmove(&strservers[i],
	        &strservers[i + 1],
	        sizeof(strservers[0]) * (nstrservers - i));
	return;
}

void stream_cb
(stream_sched* _sched, int _s, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata) {
	stream_serv* server = (stream_serv*) _privdata;
	server_privdata* privdata = (server_privdata*) server->privdata;
	char message[MAX_MSGLEN];

	extern int fd_setsize; /* from stream_sched.c */


	if (_events & STREAMHUP) {
		sprintf(message, "client %s disconnected",
		        in2ascii(server->addr));
		stream_msg(message);

		if (privdata->state == STR_STREAMING)
			clear_strtable(server);
		stream_serv_close(_sched, server);
		free(privdata);
		return;
	}

	switch (privdata->state) {
		case STR_RCVREQUEST:
			_rbuf[_rbytes] = '\0';

			if (!strstr(_rbuf, "\r\n\r\n")) {
				if (_rbytes == sizeof(privdata->rbuf)) {
					sprintf(message, "client %s invalid request",
					        in2ascii(server->addr));
					stream_msg(message);

					stream_serv_close(_sched, server);
					free(privdata);
					return;
				}

				break;
			}

			if (strstr(_rbuf, "play.pls")) {
				privdata->state = STR_SNDREPLY;

				sprintf(message,
				        "[playlist]\n"
				        "numberofentries=1\n"
				        "File1=http://%s:%d\r\n\r\n",
				        g_strhost, g_strport);

				sprintf(privdata->wbuf,
						"HTTP/1.0 200 OK\r\n"
   						"Content-Length: %d\r\n"
						"Content-Type: audio/x-scpls\r\n\r\n"
   						"\r\n"
   						"%s",
   						strlen(message), message);

				stream_msg("STR_RCVREQUEST request pls");

			}
			else if  (strstr(_rbuf, "stats.html")){
				
				struct rusage usage;
				struct rlimit lim;
				time_t diff_time, h, m, s;

				privdata->state = STR_SNDREPLY;

				getrusage(RUSAGE_SELF, &usage);
				getrlimit(RLIMIT_NOFILE, &lim);
				diff_time = time(NULL) - g_start_time;
				h = diff_time / 3600;
				m = (diff_time - (h * 3600)) / 60;
				s = diff_time - (h * 3600) - (m * 60);

				sprintf(message,
						"<html><head>\r\n"
						"<title>Litestream statistics: %s</title>\r\n"
						"</head>\r\n"
 						"<body>\r\n"
						"<h1><a href=\"%s\">%s</a> (%s) %s Kb</h1>\r\n"
						"<h3>%s mirror of <a href=\"http://%s:%d\">http://%s:%d</a></h3>\r\n"
						"<center><table width=\"50%%\">\r\n"
						"<tr><td>Source</td><td>%s</td></tr>\r\n"
						"<tr><td>Uptime</td><td align=\"right\">%02ld:%02ld:%02ld</td></tr>\r\n"
						"<tr><td>Max fd</td><td align=\"right\">%d</td></tr>\r\n"
						"<tr><td>Current/peak/total/max clients</td><td align=\"right\">%d/%d/%d/%ld</td></tr>\r\n"
 						"<tr><td>I/O bytes</td><td align=\"right\">%qu/%qu</td></tr>\r\n"
						"<tr><td>I/O messages</td><td align=\"right\">%ld/%ld</td></tr>\r\n"
 						"<tr><td>Overrun packets/bytes</td><td align=\"right\">%qu/%qu</td></tr>\r\n"
						"<tr><td>User time</td><td align=\"right\">%ld sec %ld usec</td></tr>\r\n"
						"<tr><td>System time</td><td align=\"right\">%ld sec %ld usec</td></tr>\r\n"
						"<tr><td>Max RSS</td><td align=\"right\">%ld</td></tr>\r\n"
						"<tr><td>I/O blocks</td><td align=\"right\">%ld/%ld</td></tr>\r\n"
						"<tr><td>Signals</td><td align=\"right\">%ld</td></tr>\r\n"
						"<tr><td>V/I context switches</td><td align=\"right\">%ld/%ld</td></tr>\r\n"
						"</table></center>\r\n"
 						"</body></html>\r\n",
						g_icy_info.name,
						g_icy_info.url,
						g_icy_info.name,
				        g_icy_info.genre, g_icy_info.br,
						g_mirrortag, g_srchost, g_srcport, g_srchost, g_srcport,
						g_source ? "Connected" : "Not connected",
						h, m, s,
						_sched->maxfd,
 						nstrservers, stats.peak_servers, stats.total_servers, (long) MIN(g_maxlisteners, MIN(fd_setsize, lim.rlim_cur)),
 						stats.total_stream_received, stats.total_stream_sent,
						usage.ru_msgrcv, usage.ru_msgsnd,
 						stats.overruns, stats.overrun_bytes,
						usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
						usage.ru_stime.tv_sec, usage.ru_stime.tv_usec,
						usage.ru_maxrss,
						usage.ru_inblock, usage.ru_oublock,
						usage.ru_nsignals,
						usage.ru_nvcsw, usage.ru_nivcsw);

				sprintf(privdata->wbuf,
						"HTTP/1.0 200 OK\r\n"
   						"Content-Length: %d\r\n"
   						"Content-Type: text/html\r\n"
   						"\r\n"
						"%s",
						strlen(message), message);

				stream_msg("STR_RCVREQUEST request stats");
			}else {

				privdata->state = STR_SNDHEADER;

				if (g_source)
					sprintf(privdata->wbuf,
					        "HTTP/1.0 200 OK\r\n"
						    "Server: " VERSION "\r\n"
						    "icy-name:%s\r\n"
						    "icy-genre:%s\r\n"
							"icy-url:%s\r\n"
							"icy-irc:%s\r\n"
							"icy-icq:%s\r\n"
							"icy-aim:%s\r\n"
							"icy-pub:%d\r\n"
							"icy-br:%s\r\n"
							"\r\n",
							g_icy_info.name,
							g_icy_info.genre,
							g_icy_info.url,
							g_icy_info.irc,
							g_icy_info.icq,
							g_icy_info.aim,
							g_icy_info.pub,
							g_icy_info.br);
				else
					strcpy(privdata->wbuf,
					       "HTTP/1.0 200 OK\r\n"
						   "Server: " VERSION "\r\n"
						   "\r\n");

				stream_msg("STR_RCVREQUEST request stream");
			}


			stream_sched_post_write(_sched, server->s, privdata->wbuf, strlen(privdata->wbuf));
			break;
		case STR_SNDREPLY:
			stream_msg("STR_SNDREPLY done");
			stream_serv_close(_sched, server);
			free(privdata);
			break;
		case STR_SNDHEADER:
			if (nstrservers == g_maxlisteners) {
				stream_msg("STR_SNDHEADER max listeners reached");
				stream_serv_close(_sched, server);
				free(privdata);
				return;
			}
			privdata->state = STR_STREAMING;

			strservers[nstrservers++] = server;

			stats.total_servers++;
			if (nstrservers > stats.peak_servers)
				stats.peak_servers = nstrservers;

			stream_sched_post_read(_sched, server->s, privdata->rbuf, sizeof(privdata->rbuf));
			break;
		case STR_STREAMING:
			stream_sched_post_read(_sched, server->s, privdata->rbuf, sizeof(privdata->rbuf));
			break;
	}
	return;
}

void stream_listen_cb
(stream_sched* _sched, int _s, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata) {
	stream_serv* server;
	server_privdata* privdata;
	char message[MAX_MSGLEN];


	privdata = (server_privdata*) malloc(sizeof(server_privdata));
	privdata->state = STR_RCVREQUEST;

	server = stream_serv_accept(_sched,
	                            _s,
	                            stream_cb,
	                            privdata);
	if (!server) {
		stream_err("dead connect");
		free(privdata);
		return;
	}

	sprintf(message, "client %s connected",
			in2ascii(server->addr));
	stream_msg(message);

	stream_sched_post_read(_sched, server->s, privdata->rbuf, sizeof(privdata->rbuf));
	return;
}

void source_cb
(stream_sched* _sched, int _s, short _events,
 char* _rbuf, int _rbytes,
 char* _wbuf, int _wbytes,
 void* _privdata) {
	stream_cli* source = (stream_cli*) _privdata;
	source_privdata* privdata = (source_privdata*) source->privdata;
	int i;
	int bytes;

	struct sockaddr_in peer;
	int peer_len;
	int flags;


	if (_events & STREAMHUP) {
		stream_msg("source disconnected");
		stream_cli_close(_sched, source);
		free(privdata);
		g_source = NULL;
		return;
	}

	switch (privdata->state) {
		case SRC_SNDPROLOGUE:
			privdata->state = SRC_RCVPROLOGUEACK;
			stream_sched_post_read(_sched, source->s, privdata->rbuf, sizeof(privdata->rbuf));
			break;
		case SRC_RCVPROLOGUEACK:
			_rbuf[_rbytes] = '\0';

/* omaha */
			if (!strstr(_rbuf, "\r\n\r\n") 
				&& !strstr(_rbuf, "\n\n")) {
				if (_rbytes == sizeof(privdata->rbuf)) {
					stream_msg("invalid prologue ack");

					stream_cli_close(_sched, source);
					free(privdata);
					g_source = NULL;
					return;
				}

				break;
			}

			parse_icy_info(_rbuf, _rbytes, &g_icy_info);

			strcat(g_icy_info.name, " (");
			strcat(g_icy_info.name, g_mirrortag);
			strcat(g_icy_info.name, " mirror)");

			if (g_yp_host && g_icy_info.pub)
				yp_create(_sched, g_yp_host, g_yp_port, &g_icy_info);

			stream_msg("source stream begin");
			privdata->state = SRC_STREAMING;
			stream_sched_post_read(_sched, source->s, g_source_buf, sizeof(g_source_buf));
			break;
		case SRC_STREAMING:
			stats.total_stream_received += _rbytes;

			for (i = 0; i < nstrservers; i++) {
				if (((server_privdata*) strservers[i]->privdata)->state == STR_STREAMING) {
					/* get peer address information */
					peer_len = sizeof(peer);
					if(-1 != getpeername(strservers[i]->s, (struct sockaddr *)&peer, &peer_len)) {
					}

					/* there is a problem with fcntl(O_NONBLOCK) on Linux
					 * for whatever reason the setting doesn't stick.  On
					 * Linux servers we have to re-set the O_NONBLOCK bit
					 * or else we will block for all clients if one 
					 * client pulls the plug.  Linux is for bitches. 
					 */
					if ((flags = fcntl(strservers[i]->s, F_GETFL, 0)) < 0) {
					    stream_err("fcntl F_GETFL failed");
					} else {
					    if (!(flags & O_NONBLOCK)) {
						flags |= O_NONBLOCK;
						if (fcntl(strservers[i]->s, F_SETFL, flags) < 0) {
						}
					    }
					}

					bytes = write(strservers[i]->s, _rbuf, _rbytes);
					if (bytes == -1) {
						if (errno == EAGAIN) {
							stats.overruns++;
							stats.overrun_bytes += _rbytes;
						}
/* An interesting problem. If we close the stream here, it might still
   be active on the read side, which could cause a double-close/double-free.
   And I didn't even free the privdata here to begin with!
*/
/*						else {
							stream_err("SRC_STREAMING stream write");
							clear_strtable(strservers[i]);
							stream_serv_close(_sched, strservers[i]);
						}*/
						continue;
					} else {
					}
					stats.total_stream_sent += bytes;
				}
			}

			stream_sched_post_read(_sched, source->s, g_source_buf, sizeof(g_source_buf));
			break;
	}
	return;
}

stream_cli* new_source
(stream_sched* _sched, char* _source_ip, int _port) {
	stream_cli* source;
	source_privdata* privdata;
	static char message[] = "GET /\r\nUser-Agent: xmms\r\n\r\n";


	privdata = (source_privdata*) malloc(sizeof(source_privdata));
	privdata->state = SRC_SNDPROLOGUE;

	source = stream_cli_new(_sched, _source_ip, _port, privdata, source_cb);
	if (!source) {
		stream_err("");
		goto bad;
	}

	stream_sched_post_write(_sched, source->s, message, strlen(message));

	return(source);

bad:
	free(privdata);
	return(NULL);
}

void sigint_handler (int x) {
	g_time_to_die = 1;
}

void sigalrm_handler (int x) {
	g_tick = 1;
}

int main (int argc, char* argv[]) {
	stream_sched* sched;
	stream_serv* stream;

	time_t time_now, last_touch = 0, last_attempt = 0;
	struct itimerval itimer = { { 10, 0 }, { 10, 0 } }; /* Every 10 seconds. */

	char* log_ident;


	if (argc < 8 || argc > 10) {
		fprintf(stderr, "Usage: %s <mirror tag> <source host> <source port> <stream host> <stream port> <max listeners> <log ident> [<yp host> <yp port>]\n", argv[0]);
		return(-1);
	}

	g_mirrortag = argv[1];
	g_srchost = argv[2];
	g_srcport = atoi(argv[3]);
	g_strhost = argv[4];
	g_strport = atoi(argv[5]);
	g_maxlisteners = atoi(argv[6]);
	log_ident = argv[7];

	if (argc == 10) {
		g_yp_host = argv[8];
		g_yp_port = atoi(argv[9]);
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGTRAP, SIG_IGN);
	signal(SIGINT, sigint_handler);
	signal(SIGALRM, sigalrm_handler);

	setitimer(ITIMER_REAL, &itimer, NULL);

	stream_openlog(log_ident);
	stream_msg("server started");

	g_start_time = time(NULL);

	sched = stream_sched_new();

	stream = stream_serv_new(sched, g_strport, 100, NULL, stream_listen_cb);
	if (!stream) {
		stream_err("stream create");
		return(1);
	}

	while (!g_time_to_die) {
		if (g_tick) {

			g_tick = 0;
			time_now = time(NULL);

			if (!g_source && time_now - last_attempt > 30) {
				last_attempt = time_now;
				g_source = new_source (sched, g_srchost, g_srcport);
			}

			if (g_source && strlen(g_yp_info.id) &&
				(time_now - last_touch) > (g_yp_info.tchfrq * 60)) {

				char args[MAX_MSGLEN];

				last_touch = time_now;

				sprintf(args,
				        "id=%s&p=%d&li=%d&alt=0",
				        g_yp_info.id, g_strport, nstrservers);
				yp_touch(sched, g_yp_host, g_yp_port, args);
			}
		}
		stream_sched_run(sched, 10000);
	}

	stream_msg("server exited normally");
	stream_closelog();
	exit(0);
}
