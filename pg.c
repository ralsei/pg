/* pg pg.c git 04/23/2016 dead cat <dcat@iotek.org>
 * ix <arcetera@openmailbox.org>
 * boobah <anon>
 */

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

#include "config.h"
#include "pg.h"

unsigned preserve;

unsigned int
getcup(FILE *tty, unsigned int *y, unsigned int *x)
{ /* query the terminal for the cursor position. return > 0 on error. */
    fputs(CSI "6n", stdout);
    fflush(stdout);

    *x = *y = 0;
    return (fscanf(tty, CSI "%u;%uR", y, x) < 0) ? 1 : 0;
}

void
redraw(void)
{ /* redraw the screen */
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
{ /* 'screen control' - initialize/update/clean screen */
    switch (sig) {
    case INIT:
        tcgetattr(0, &t_old);
        t_new = t_old;

        cfmakeraw(&t_new);
        tcsetattr(0, TCSANOW, &t_new);
        getcup(in, &y, &x);

        fputs(CSI "?25l" CSI "?47h", stdout);
        fflush(stdout);
    case UPDATE:
        ioctl(1, TIOCGWINSZ, &scr);
        redraw();
        break;
    case CLEAN:
        tcsetattr(0, TCSANOW, &t_old);
		fprintf(stdout, CSI "?25h" CSI "?47l");
        if (preserve) {
            redraw();
            fputs(CSI "2K\r", stdout);
        } else
            fprintf(stdout, CSI "%u;%uH", y, x);
        fflush(stdout);
    }
}

void
scroll(int dir, int times)
{ /* scroll, duh */
    int          i;
    struct ln_s *p;

    /* normalize multiplier */
    if (times < 1)
        times = 1;

    while (times-- > 0) {
        switch (dir) {
        case UP:
            if (top == TAILQ_FIRST(&head))
                break;

            top = TAILQ_PREV(top, lnhead, entries);
            break;
        case DOWN:
            p = top;

            for (i = 0; i < scr.ws_row - 1; i++) {
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

void
usage(void)
{
    fprintf(stderr, "usage: %s [file]\n", basename(argv0));
}

int
main(int argc, char **argv)
{
#ifdef __OpenBSD__
    if (pledge("stdio rpath tty", NULL) == -1)
        err(1, "pledge");
#endif

    char         buf[BUFSIZ];
    struct ln_s *p;
    int          c, d, done, i, mult, n;
    char        *ttydev;

    argv0 = argv[0];

    if (argc > 1) {
        in = fopen(argv[1], "r");
        if (NULL == in)
            errx(1, "unable to open file");
    } else
        in = stdin;

    if (isatty(0) && argc < 2) {
        usage();
        return 1;
    }

    TAILQ_INIT(&head);
    ttydev = ttyname(1);

    /* allocate all lines */
    for (n = 1; (fgets(buf, BUFSIZ, in)) != NULL; n++) {
        p = malloc(sizeof(struct ln_s));

        if (p == NULL)
            err(1, "malloc()");

        strncpy(p->str, buf, BUFSIZ);
        p->len = strnlen(p->str, BUFSIZ);
        p->str[p->len - 1] = 0;
        p->num = n;

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

    for (done = mult = preserve = 0; !done; ) {
        redraw();
        c = getc(in);

        /* handle numerical key multipler input */
        if (c >= '0' && c <= '9') {
            mult = mult * 10 + c - '0';
            continue;
        }

        /* handle all other keys */
        switch (c) {
        case 'b':
            /* scroll up by one page */
            scroll(UP, scr.ws_row - 1);
            break;
        case 'd':
            /* scroll down by half a page */
            scroll(DOWN, scr.ws_row / 2);
            break;
        case 'f':
        case ' ':
            /* scroll down by one page */
            scroll(DOWN, scr.ws_row - 1);
            break;
        case 'g':
        case '<':
            top = TAILQ_FIRST(&head);
            break;
go_end:
        case 'G':
        case '>':
            p = TAILQ_LAST(&head, lnhead);
            i = scr.ws_row - 1;

            while (i--)
                p = TAILQ_PREV(p, lnhead, entries);

            top = p;
            break;
        case K_DOWN :
        case '\n'   : /* LF */
        case '\r'   : /* CR */
            scroll(DOWN, mult);
            break;
        case K_UP :
        case   8  : /* BS  */
        case 127  : /* DEL */
            scroll(UP, mult);
            break;
        case 'Q' :
            preserve = 1;
        case 'q' :
        case   3 : /* ^C */
        case EOF :
            done++;
            break;
        case 'u':
            /* scroll up by half a page */
            scroll(UP, scr.ws_row / 2);
            break;
        /* ESC */
        case 27:
            if ((c = getc(in)) != '[')
                break;

            switch ((c = getc(in))) {
            /* Home */
            case 'H':
                top = TAILQ_FIRST(&head);
                break;
            /* End */
            case '4':
                if ((c = getc(in)) == '~')
                    goto go_end;
                break;
            /* PgUp */
            case '5':
                if ((c = getc(in)) == '~')
                    scroll(UP, scr.ws_row - 1);
                break;
            /* PgDn */
            case '6':
                if ((c = getc(in)) == '~')
                    scroll(DOWN, scr.ws_row - 1);
                break;
            /* Up */
            case 'A':
                scroll(UP, mult);
                break;
            /* Down */
            case 'B':
                scroll(DOWN, mult);
                break;
            }

            break;
        default:
            /* ring bell for unrecognized keys */
            if (b_errorbell)
                fputc('\a', stdout);
        }

        /* reset multiplier for next command */
        mult = 0;
    }

    scrctl(CLEAN);

    /* free all lines */
    while (!TAILQ_EMPTY(&head)) {
        struct ln_s    *entry = TAILQ_FIRST(&head);
        TAILQ_REMOVE(&head, entry, entries);
        free(entry);
    }

    return 0;
}
/* vim set ts=4 et: */
