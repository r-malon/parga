VERSION = 0.0.1

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
MANSECTION = 1

CC = cc
CFLAGS = -fanalyzer -g -Wall -Wextra -pedantic -O0 -fwrapv
CPPFLAGS = -DDEBUG
LDFLAGS = 
