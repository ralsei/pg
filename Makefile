PREFIX = /usr/local
CFLAGS = -g -Wall

pg: config.h pg.h pg.c
	$(CC) $(CFLAGS) -o pg pg.c

clean:
	rm -f pg

install:
	install -m755 pg   $(DESTDIR)$(PREFIX)/bin/pg
	install -m644 pg.1 $(DESTDIR)$(PREFIX)/man/man1/pg.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/pg
	rm -f $(DESTDIR)$(PREFIX)/man/man1/pg.1
