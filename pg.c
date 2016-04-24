/*
 * pg pg.c git 04/23/2016 dead cat <dcat@iotek.org>
 * ix <arcetera@openmailbox.org>
 * boobah <anon>
 */

#define CSI    "\033["
#define BLANK  CSI "30m~" CSI "m%s"

#include <sys/queue.h>
#include <sys/ioctl.h>

#include <err.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

enum {
	UP,
	DOWN
};

enum {
	INIT,
	UPDATE = SIGWINCH,
	CLEAN
};

struct ln_s {
	char        str[BUFSIZ];
	size_t      len;
	int         num;
	TAILQ_ENTRY (ln_s) entries;
};

char *argv0;

static struct winsize scr;
static struct termios oldt, newt;
static struct ln_s *top;
static TAILQ_HEAD(lnhead, ln_s) head;

void
usage(void)
{
	fprintf(stderr, "usage: %s [file]\n", basename(argv0));
}

void
redraw(void)
{
	int          i, last;
	struct ln_s *p = top;

	last = top->num + scr.ws_row;
	fputs(CSI "H" CSI "2J", stdout);

	for (i = 0; i < scr.ws_row; i++) {
		printf("%s%s", p->str, (i == scr.ws_row - 1) ? "" : "\r\n");

		/* if EOF, fill the remainder of the view with blanks */
		if (p == TAILQ_LAST(&head, lnhead))
			for (; i < scr.ws_row - 1; i++)
				printf(BLANK "%s", (i == scr.ws_row - 2) ? "" : "\r\n");

		p = TAILQ_NEXT(p, entries);
	}
}

void
scrctl(int sig)
{
	switch (sig) {
	case INIT:
		tcgetattr(0, &oldt);
		newt = oldt;

		cfmakeraw(&newt);
		tcsetattr(0, TCSANOW, &newt);
		puts(CSI "?25l" CSI "?47h");
	case UPDATE:
		ioctl(1, TIOCGWINSZ, &scr);
		redraw();
		break;
	case CLEAN:
		tcsetattr(0, TCSANOW, &oldt);
		puts(CSI "?25h" CSI "?47l");
		break;
	}
}

void
scroll(int dir, int times)
{
	int          i;
	struct ln_s *p;

	while (times-- > 0) {
		switch (dir) {
		case UP:
			if (top == TAILQ_FIRST(&head))
				break;

			top = TAILQ_PREV(top, lnhead, entries);
			break;
		case DOWN:
			p = top;

			for (i = 0; i < scr.ws_row - 4; i++) {
				p = TAILQ_NEXT(p, entries);

				if (p != TAILQ_LAST(&head, lnhead))
					continue;

				return;
			}

			top = TAILQ_NEXT(top, entries);
			break;
		}
	}
}

int
main(int argc, char *argv[])
{
#ifdef __OpenBSD__
	if (pledge("stdio rpath tty", NULL) == -1)
		err(1, "pledge");
#endif
	char         buf[BUFSIZ];
	struct ln_s *p;
	int          i, d, done, c, n = 1;
	char        *ttydev;
	FILE        *in;

	argv0 = argv[0];

	if (argc > 1) {
		in = fopen(argv[1], "r");
		if (NULL == in)
			errx(1, "unable to open file");
	} else {
		in = stdin;
	}

	if (isatty(0) && argc < 2) {
		usage();
		return 1;
	}

	TAILQ_INIT(&head);
	ttydev = ttyname(1);

	/* allocate all lines */
	while ((fgets(buf, BUFSIZ, in)) != NULL) {
		p = malloc(sizeof(struct ln_s));

		if (p == NULL)
			err(1, "malloc()");

		strncpy(p->str, buf, BUFSIZ);
		p->len = strnlen(p->str, BUFSIZ);
		p->str[p->len - 1] = 0;
		p->num = n++;

		TAILQ_INSERT_TAIL(&head, p, entries);
	}

	if (TAILQ_FIRST(&head) == NULL)
		errx(1, "no input");

	/* reopening stdin after reading EOF */
	in = freopen(ttydev, "r", in);
	close(STDIN_FILENO);
	d = open(ttydev, O_RDONLY);

	top = TAILQ_FIRST(&head);

	scrctl(INIT);
	signal(SIGWINCH, scrctl);

	if (d)
		err(1, "open() opened something, but not stdin");

	done = 0;
	while (!done) {
		redraw();

		c = getc(in);
		switch (c) {
		case 'g':
			top = TAILQ_FIRST(&head);
			break;
		case 'G':
			p = TAILQ_LAST(&head, lnhead);
			i = scr.ws_row - 4;

			while (i--)
				p = TAILQ_PREV(p, lnhead, entries);

			top = p;
			break;
		case '\n':
		case '\r':
		case 'j':
			scroll(DOWN, 1);
			break;
		case 'k':
			scroll(UP, 1);
			break;
		case 3:   /* ^C */
		case 'q':
		case EOF:
			done++;
			break;
		case ' ':
			/* scroll down by one "page" */
			scroll(DOWN, scr.ws_row - 1);
			break;
		/* alt */
		case 27:
			if ((c = getc(in)) == '[')
				switch ((c = getc(in))) {
				case 'A':
					scroll(UP, 1);
					break;
				case 'B':
					scroll(DOWN, 1);
					break;
				}
		}
	}

	/* free all lines */
	while (!TAILQ_EMPTY(&head)) {
		struct ln_s    *entry = TAILQ_FIRST(&head);
		TAILQ_REMOVE(&head, entry, entries);
		free(entry);
	}

	scrctl(CLEAN);

	return 0;
}
