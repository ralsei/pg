/*
 * pg pg.c git 04/23/2016 dead cat <dcat@iotek.org>
 * ix <arcetera@openmailbox.org>
 * boobah <anon>
 */

#include <sys/queue.h>
#include <sys/ioctl.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define CSI "\033["

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
	char 		str[BUFSIZ];
	size_t 		len;
	int 		num;
	TAILQ_ENTRY   (ln_s) entries;
};

static struct winsize scr;
static struct termios oldt, newt;
static struct ln_s *top;
static TAILQ_HEAD(lnhead, ln_s) head;

void
redraw(void)
{
	int 		i, last;
	struct ln_s    *p = top;

	last = top->num + scr.ws_row;
	puts(CSI "[H" CSI "2J");

	for (i = 0; i < scr.ws_row; i++) {
		printf("\r\n%s", p->str);

		if (p == TAILQ_LAST(&head, lnhead))
			for (; i < scr.ws_row - 1; i++)
				puts("~\r");

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
		puts(CSI "?25l");
	case UPDATE:
		ioctl(1, TIOCGWINSZ, &scr);
		redraw();
		redraw();
		break;
	case CLEAN:
		tcsetattr(0, TCSANOW, &oldt);
		puts(CSI "?25h");
		break;
	}
}

void
scroll(int dir, int times)
{
	int 		i;
	struct ln_s    *p;

	while (times--) {
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
	char 		buf[BUFSIZ];
	struct ln_s    *p;
	int 		i, d, done, c, n = 1;
	char           *ttydev;
	FILE           *in;

	if (argc > 1) {
		if(strncmp(argv[1], "-", 1)) {
			in = fopen(argv[1], "r");
			if (NULL == in)
				errx(1, "unable to open file");
		} else
			in = stdin;
	} else {
		in = stdin;
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
		case '\r':
		case 'j':
			scroll(DOWN, 1);
			break;
		case 'k':
			scroll(UP, 1);
			break;
		/* ^C */
		case 3:
		case 'q':
		case EOF:
			done++;
			break;
		case ' ':
			scroll(DOWN, scr.ws_row / 4);
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
