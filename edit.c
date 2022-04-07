#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include "edit.h"

/* While lists were handy for loading the data, in the editor we'd rather
 * have an array that we can quickly index into.
 */
int populate_entities(struct entities *ent, struct list_head *guns,
		      struct list_head *engines, struct list_head *manfs,
		      struct list_head *techs)
{
	struct turret *gun;
	struct engine *eng;
	struct manf *manf;
	struct tech *tech;
	unsigned int i;

	memset(ent, 0, sizeof(*ent));
	/* Count the entities, to size the arrays */
	list_for_each_entry(gun, guns)
		ent->ngun++;
	list_for_each_entry(eng, engines)
		ent->neng++;
	list_for_each_entry(manf, manfs)
		ent->nmanf++;
	list_for_each_entry(tech, techs)
		ent->ntech++;
	/* Allocate the arrays of pointers */
	ent->gun = calloc(ent->ngun, sizeof(gun));
	if (!ent->gun)
		return -errno;
	ent->eng = calloc(ent->neng, sizeof(eng));
	if (!ent->eng)
		return -errno;
	ent->manf = calloc(ent->nmanf, sizeof(manf));
	if (!ent->manf)
		return -errno;
	ent->tech = calloc(ent->ntech, sizeof(tech));
	if (!ent->tech)
		return -errno;
	/* Fill in the arrays */
	i = 0;
	list_for_each_entry(gun, guns)
		ent->gun[i++] = gun;
	i = 0;
	list_for_each_entry(eng, engines)
		ent->eng[i++] = eng;
	i = 0;
	list_for_each_entry(manf, manfs)
		ent->manf[i++] = manf;
	i = 0;
	list_for_each_entry(tech, techs)
		ent->tech[i++] = tech;
	return 0;
}

static int enable_cbreak_mode(struct termios *old)
{
	struct termios cbreak;

	if (tcgetattr(STDIN_FILENO, old) == -1)
		return -errno;
	cbreak = *old;
	cbreak.c_lflag &= ~(ECHO | ICANON);
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &cbreak) == -1)
		return -errno;
	return 0;
}

static int disable_cbreak_mode(const struct termios *old)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, old) == -1)
		return -errno;
	return 0;
}

static int edit_manf(struct bomber *b, struct tech_numbers *tn,
		     const struct entities *ent)
{
	unsigned int i;
	int c;

	printf("Select manufacturer\n");
	for (i = 0; i < ent->nmanf; i++)
		printf("[%c] %s\n", i + '1', ent->manf[i]->name);

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '\x1b') // ESC
			break;

		i = c - '1';
		if (i < 0 || i >= ent->nmanf) {
			putchar('?');
			continue;
		}
		b->manf = ent->manf[i];
		return 0;
	} while (1);

	return -EAGAIN;
}

int edit_loop(struct bomber *b, struct tech_numbers *tn,
	      const struct entities *ent)
{
	struct termios cooked;
	int rc, c;
	bool qc;

	rc = enable_cbreak_mode(&cooked);
	if (rc) {
		fprintf(stderr, "Failed to enable cbreak mode on tty\n");
		return rc;
	}
	dump_bomber_info(b);
	dump_bomber_calcs(b);

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == 'Q') {
			if (qc)
				break;
			qc = true;
			fprintf(stderr, "'Q' again to quit\n");
			continue;
		}
		qc = false;
		switch (c) {
		case '\n':
			dump_bomber_calcs(b);
			continue;
		case 'M':
			rc = edit_manf(b, tn, ent);
			break;
		default: /* ignore unknown inputs */
			continue;
		}
		/* To quote ken,
		 * The experienced user will usually know what's wrong.
		 */
		if (rc)
			putchar('?');
		rc = calc_bomber(b, tn);
		if (rc < 0)
			putchar('!');
		dump_bomber_info(b);
	} while (1);

	if (disable_cbreak_mode(&cooked))
		fprintf(stderr, "Failed to disable cbreak mode on tty\n");
	return rc;
}
