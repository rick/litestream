# $Id: Makefile,v 1.8 2005/09/06 21:26:02 roundeye Exp $

INCLUDES = -Iinclude
AR=	ar
RANLIB=	sh ranlib.sh

CFLAGS = $(INCLUDES) -Wall -g -DVERSION="\"Litestream 1.2\""

LDFLAGS = # -lnsl -lsocket

all: litestream literestream source client server 

.depend: $(WILDCARD *.c)
	$(CC) -MM $(CFLAGS) -DDEPEND *.c > .depend

client: stream_cli.o stream_sched.o hexdump.o client.o stream_log.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

server: stream_serv.o stream_sched.o hexdump.o server.o stream_log.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

literestream: stream_serv.o stream_sched.o restream.o hexdump.o icy.o yp.o stream_cli.o http.o textutils.o stream_log.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

litestream: stream_serv.o stream_sched.o stream.o hexdump.o icy.o yp.o stream_cli.o http.o textutils.o stream_log.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

source: stream_sched.o stream_cli.o http.o stream_log.o source.o mp3.o playlist.o textutils.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o source server client litestream literestream .depend *.core

include .depend
