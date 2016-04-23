PREFIX = /usr/local
CFLAGS = -g

pg: pg.c

clean:
	rm -f pg

install:
	install -m755 -d $(DESTDIR)$(PREFIX)/bin
	install -m755 pg $(DESTDIR)$(PREFIX)/bin/pg

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/pg
