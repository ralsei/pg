/* pg pg.h git 04/30/2016 ix <arcetera@openmailbox.org */

#ifndef PG_H__
#define PG_H__

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
	char        str[BUFSIZ];
	size_t      len;
	int         num;
	TAILQ_ENTRY (ln_s) entries;
};

char         *argv0;
FILE         *in;
unsigned int  x, y;  /* initial cursor position */

static struct winsize  scr;
static struct termios  t_new, t_old;
static struct ln_s    *top;

static TAILQ_HEAD(lnhead, ln_s) head;

/* function declarations */
unsigned int getcup(FILE *, unsigned int *, unsigned int *);
void         redraw(void);
void         scrctl(int);
void         scroll(int, int);
void         usage(void);

#endif /* end of include guard: PG_H__ */
