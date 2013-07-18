# Makefile for systems with GNU tools
CC      = gcc
CXX     = g++
INSTALL = install

CFLAGS  = -I.
LDFLAGS = -lpthread
PREDEF  = -D_FILE_OFFSET_BITS=64

ifeq ($(DEBUG), 1)
    CFLAGS += -O0 -g
    WARN   += -Wall -Wextra -Wno-comment -Wformat -Wimplicit -Wparentheses -Wswitch \
		     -Wunused
else
    CFLAGS += -O2 -g
    WARN   += -Wall -Wextra -Wno-comment -Wformat -Wimplicit -Wparentheses -Wswitch -Wuninitialized \
		     -Wunused
endif

ifndef ($(INSTALLDIR))
	INSTALLDIR = /usr
endif

OBJS    = syslog-safer.o ringbuffer.o

syslog-safer: $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o:%.cc
	$(CXX) -o $@ $(WARN) $(CFLAGS) $(PREDEF) -c $<

install:
	$(INSTALL) -D syslog-safer $(DESTDIR)$(INSTALLDIR)/sbin/syslog-safer
	$(INSTALL) -D syslog-safer.init $(DESTDIR)/etc/init.d/syslog-safer
	$(INSTALL) -D -m 0644 syslog-safer.sysconfig $(DESTDIR)/etc/sysconfig/syslog-safer
	$(INSTALL) -D logger $(DESTDIR)$(INSTALLDIR)/bin/logger

clean:
	rm -f ./*.o
