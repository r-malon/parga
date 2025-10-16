.POSIX:

include config.mk

PROG = parga
SRCS = main.c
OBJS = $(SRCS:.c=.o)

all: $(PROG)

main.o: parga.h

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

$(PROG): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	mkdir -p $(DESTDIR)$(MANPREFIX)/man$(MANSECTION)
	install -m 755 $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)
	install -m 644 $(PROG).$(MANSECTION) \
		$(DESTDIR)$(MANPREFIX)/man$(MANSECTION)/$(PROG).$(MANSECTION)

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/$(PROG)
	rm $(DESTDIR)$(MANPREFIX)/man$(MANSECTION)/$(PROG).$(MANSECTION)

clean:
	-rm -f $(OBJS) $(PROG)

.PHONY: all clean install uninstall
