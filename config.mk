VERSION = 0.0.1

PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man
MANSECTION = 1

CC = cc
CFLAGS = -fanalyzer -g3 -Wall -Wextra -Wconversion -pedantic -O0
CPPFLAGS = -DDEBUG
LDFLAGS = 
