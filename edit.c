#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include "edit.h"

static int enable_cbreak_mode(struct termios *old)
{
	struct termios cbreak;

	if (tcgetattr(STDIN_FILENO, old) == -1)
		return -errno;
	cbreak = *old;
	cbreak.c_lflag &= ~ICANON;
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

static const char *describe_ftype(enum fuse_type ft)
{
	switch (ft) {
	case FT_NORMAL:
		return "normal";
	case FT_SLENDER:
		return "slender";
	case FT_SLABBY:
		return "slab-sided";
	case FT_GEODETIC:
		return "geodetic";
	default:
		return "error!  unknown type";
	}
}

const char *describe_bbg(enum bb_girth girth)
{
	switch (girth) {
	case BB_SMALL:
		return "small bombs";
	case BB_MEDIUM:
		return "medium bombs";
	case BB_COOKIE:
		return "large bombs";
	default:
		return "error!  unknown girth";
	}
}

static const char *describe_esl(enum elec_level esl)
{
	switch (esl) {
	case ESL_LOW:
		return "low power";
	case ESL_HIGH:
		return "high power";
	case ESL_STABLE:
		return "high power, stable voltage";
	default:
		return "error!  unknown electric supply level";
	}
}

static char crew_to_letter(enum crewpos c)
{
	switch (c) {
	case CCLASS_P:
		return 'P';
	case CCLASS_N:
		return 'N';
	case CCLASS_B:
		return 'B';
	case CCLASS_W:
		return 'W';
	case CCLASS_E:
		return 'E';
	case CCLASS_G:
		return 'G';
	default:
		return '?';
	}
}

static enum crewpos letter_to_crew(char c)
{
	switch (c) {
	case 'P':
		return CCLASS_P;
	case 'N':
		return CCLASS_N;
	case 'B':
		return CCLASS_B;
	case 'W':
		return CCLASS_W;
	case 'E':
		return CCLASS_E;
	case 'G':
		return CCLASS_G;
	default:
		return CREW_CLASSES;
	}
}

static void dump_manf(struct bomber *b)
{
	printf("[M]anufacturer: %s\n", b->manf->name);
}

static void dump_engines(struct bomber *b)
{
	struct engine *e = b->engines.typ;

	printf("[E]ngines: %u × %s %s (%u hp, SCL %u%s)\n",
	       b->engines.number, e->manu, e->name, e->bhp, e->scl,
	       b->engines.egg ? ", Power Egg" : "");
}

static void dump_turrets(struct bomber *b)
{
	bool comma = false;
	unsigned int i;

	printf("[T]urrets:");
	for (i = LXN_NOSE; i < LXN_COUNT; i++) {
		struct turret *t = b->turrets.typ[i];

		if (t) {
			printf("%s %s", comma ? "," : "", t->name);
			comma = true;
		}
	}
	putchar('\n');
}

static void dump_wing_crew(struct bomber *b)
{
	unsigned int i, j;
	char crew[33];

	printf("[W]ing: %u sq ft; aspect ratio %.1f",
	       b->wing.area, b->wing.art / 10.0f);
	j = 0;
	for (i = 0; i < b->crew.n; i++) {
		struct crewman *m = b->crew.men + i;

		crew[j++] = crew_to_letter(m->pos);
		if (m->gun)
			crew[j++] = '*';
	}
	crew[j] = 0;
	printf("; [C]rew: %s\n", crew);
}

static void dump_bay_fuse(struct bomber *b)
{
	printf("[B]omb bay: %ulb, %s", b->bay.cap,
	       describe_bbg(b->bay.girth));
	printf("; [F]uselage: %s\n", describe_ftype(b->fuse.typ));
}

static void dump_elec_fuel(struct bomber *b)
{
	printf("e[L]ectrics: %s", describe_esl(b->elec.esl));
	printf("; f[U]el: %.1f hours%s\n", b->tanks.ht / 10.0f,
	       b->tanks.sst ? ", self-sealing" : "");
}

static void dump_bomber_info(struct bomber *b)
{
	dump_manf(b);
	dump_engines(b);
	dump_turrets(b);
	dump_wing_crew(b);
	dump_bay_fuse(b);
	dump_elec_fuel(b);

	if (b->error)
		printf("Design contains errors!  Please review [X].\n");
	else if (b->warning)
		printf("Design contains warnings!  Consider reviewing [X].\n");
}

static void dump_bomber_calcs(struct bomber *b)
{
	printf("Dimensions: span %.1fft, chord %.1fft\n",
	       b->wing.span, b->wing.chord);
	printf("Weights: tare %.0flb, gross %.0flb; wing loading %.1flb/sq ft, L/D %.1f\n", b->tare, b->gross, b->wing.wl, b->wing.ld);
	printf("\t(core %.0flb, fuse %.0flb, wing %.0flb, eng %.0flb, tanks %.0flb)\n",
	       b->core_tare, b->fuse.tare, b->wing.tare, b->engines.tare,
	       b->tanks.tare);
	printf("\t(gun %.0flb, ammo %.0flb, crew %.0flb, bay %.0flb, fuel %.0flb)\n",
	       b->turrets.tare, b->turrets.ammo,
	       b->crew.gross + b->crew.tare, b->bay.tare, b->tanks.mass);
	printf("Speeds: take-off %.1fmph, max %.1fmph, cruise %.1fmph at %.0fft\n",
	       b->takeoff_spd, b->deck_spd, b->cruise_spd,
	       b->cruise_alt * 1000.0f);
	printf("Service ceiling: %.0fft; range: %.0fmi; initial climb %.0ffpm\n",
	       b->ceiling * 1000.0f, b->range, b->init_climb);
	printf("Survivability %.1f/%.1f; Reliability %.1f; Serviceability %.1f\n",
	       b->defn[0], b->defn[1], (1.0f - b->fail) * 100.0f,
	       b->serv * 100.0f);
	printf("\t(roll %.1f, turn %.1f, evade %.1f, vuln %.2f (fr %.2f))\n",
	       b->roll_pen, b->turn_pen, b->evade_factor, b->vuln, b->tanks.ratio);
	printf("Cost: %.0f funds\n", b->cost);
	printf("\t(core %.0f, fuse %.0f, wing %.0f, eng %.0f, gun %.0f, elec %.0f)\n",
	       b->core_cost, b->fuse.cost, b->wing.cost, b->engines.cost,
	       b->turrets.cost, b->elec.cost);
	printf("Prototype in %.0f days for %.0f funds\n",
	       b->tproto, b->cproto);
	printf("Production in %.0f days for %.0f funds\n",
	       b->tprod, b->cprod);
}

static int edit_manf(struct bomber *b, struct tech_numbers *tn,
		     const struct entities *ent)
{
	unsigned int i;
	int c;

	printf(">Select manufacturer (0 to cancel)\n");
	for (i = 0; i < ent->nmanf; i++)
		printf("[%c] %s\n", i + '1', ent->manf[i]->name);

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		i = c - '1';
		if (i < 0 || i >= ent->nmanf) {
			putchar('?');
			continue;
		}
		b->manf = ent->manf[i];
		putchar('>');
		dump_manf(b);
		return 0;
	} while (1);

	return -EIO;
}

static int edit_eng(struct bomber *b, struct tech_numbers *tn,
		    const struct entities *ent)
{
	unsigned int i;
	int c;

	printf(">Select engine (0 to cancel) or number\n");
	for (i = 0; i < ent->neng; i++)
		if (ent->eng[i]->unlocked)
			printf("[%c] %s %s\n", i + 'A', ent->eng[i]->manu,
			       ent->eng[i]->name);

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		i = c - '0';
		if (i >= 1 && i < 9) {
			b->engines.number = i;
			putchar('>');
			dump_engines(b);
			return 0;
		}
		i = c - 'A';
		if (i < 0 || i >= ent->neng || !ent->eng[i]->unlocked) {
			putchar('?');
			continue;
		}
		b->engines.typ = ent->eng[i];
		putchar('>');
		dump_engines(b);
		return 0;
	} while (1);

	return -EIO;
}

static bool gun_conflict(const struct bomber *b, const struct turret *t)
{
	if (t->lxn == LXN_NOSE && b->engines.odd)
		return true;
	if (t->slb && b->fuse.typ != FT_SLABBY)
		return true;
	return b->turrets.typ[t->lxn] != NULL;
}

static int edit_guns(struct bomber *b, struct tech_numbers *tn,
		     const struct entities *ent)
{
	unsigned int i;
	int c;

	printf(">Select turret to add, number to remove, or 0 to cancel\n");
	for (i = 0; i < ent->ngun; i++)
		if (ent->gun[i]->unlocked && !gun_conflict(b, ent->gun[i]))
			printf("[%c] %s\n", i + 'A', ent->gun[i]->name);
	for (i = LXN_NOSE; i < LXN_COUNT; i++)
		if (b->turrets.typ[i])
			printf("[%d] %s\n", i, b->turrets.typ[i]->name);

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}
		if (c == 'Z') {
			for (i = LXN_NOSE; i < LXN_COUNT; i++)
				b->turrets.typ[i] = NULL;
			putchar('>');
			dump_turrets(b);
			return 0;
		}

		i = c - '0';
		if (i >= LXN_NOSE && i < LXN_COUNT && b->turrets.typ[i]) {
			b->turrets.typ[i] = NULL;
			putchar('>');
			dump_turrets(b);
			return 0;
		}
		i = c - 'A';
		if (i < 0 || i >= ent->ngun || !ent->gun[i]->unlocked ||
		    gun_conflict(b, ent->gun[i])) {
			putchar('?');
			continue;
		}
		b->turrets.typ[ent->gun[i]->lxn] = ent->gun[i];
		putchar('>');
		dump_turrets(b);
		return 0;
	} while (1);

	return -EIO;
}

static int edit_uint(unsigned int *res)
{
	char buf[80];

	putchar('>');
	if (!fgets(buf, sizeof(buf), stdin))
		return -EIO;
	if (sscanf(buf, "%u", res) == 1)
		return 0;
	return -EINVAL;
}

static int edit_area(struct bomber *b)
{
	unsigned int v;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter wing area in sq ft, or 0 to cancel\n");
	} while (rc);
	putchar('>');
	if (v) {
		b->wing.area = v;
		dump_wing_crew(b);
	}
	return 0;
}

static int edit_aspect(struct bomber *b)
{
	unsigned int v;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter aspect ratio in tenths, or 0 to cancel\n");
	} while (rc);
	putchar('>');
	if (v) {
		b->wing.art = v;
		dump_wing_crew(b);
	}
	return 0;
}

static int edit_wing(struct bomber *b)
{
	int c;

	printf(">Edit [A]rea or aspect [R]atio (0 to cancel)\n");
	do {
		c = getchar();
		if (c == EOF)
			return -EIO;
		switch (c) {
		case '0':
			putchar('>');
			return 0;
		case 'a':
		case 'A':
			return edit_area(b);
		case 'r':
		case 'R':
			return edit_aspect(b);
		default:
			putchar('?');
			break;
		}
	} while (1);
}

static int edit_crew(struct bomber *b, struct tech_numbers *tn)
{
	struct crewman *m;
	enum crewpos pos;
	bool gun = false;
	unsigned int i;
	int c;

	printf(">[PNBWEG] to add crew, number to remove, *num to toggle gunner, or 0 to cancel\n");
	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		if (c == 'Z') {
			b->crew.n = 0;
			putchar('>');
			dump_wing_crew(b);
			return 0;
		}
		if (c == '*') {
			gun = !gun;
			continue;
		}

		i = c - '1';
		if (i < b->crew.n) {
			m = &b->crew.men[i];
			if (gun) {
				if (m->gun || m->pos == CCLASS_B ||
				    m->pos == CCLASS_W) {
					m->gun = !m->gun;
					putchar('>');
					dump_wing_crew(b);
					return 0;
				}
			} else if (b->crew.n) {
				for (; ++i < b->crew.n;)
					b->crew.men[i - 1] = b->crew.men[i];
				b->crew.n--;
				putchar('>');
				dump_wing_crew(b);
				return 0;
			}
		}
		pos = letter_to_crew(c);
		if (b->crew.n >= MAX_CREW || pos >= CREW_CLASSES) {
			putchar('?');
			continue;
		}
		m = &b->crew.men[b->crew.n++];
		m->pos = pos;
		m->gun = false;
		putchar('>');
		dump_wing_crew(b);
		return 0;
	} while (1);

	return -EIO;
}

static int edit_cap(struct bomber *b)
{
	unsigned int v;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter bomb capacity in lb, or 0 to cancel\n");
	} while (rc);
	putchar('>');
	if (v) {
		b->bay.cap = v;
		dump_bay_fuse(b);
	}
	return 0;
}

static int edit_bay(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int i;
	int c;

	printf(">[B]omb capacity or [");
	for (i = 0; i < BB_COUNT; i++)
		if (tn->bt[i])
			putchar("SMC"[i]);
	printf("] to set girth\n");
	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		switch (c) {
		case 'b':
		case 'B':
			return edit_cap(b);
		case 's':
		case 'S':
			i = BB_SMALL;
			break;
		case 'm':
		case 'M':
			i = BB_MEDIUM;
			break;
		case 'c':
		case 'C':
			i = BB_COOKIE;
			break;
		default:
			putchar('?');
			continue;
		}
		if (tn->bt[i]) {
			b->bay.girth = i;
			putchar('>');
			dump_bay_fuse(b);
			return 0;
		}
		putchar('?');
	} while (1);

	return -EIO;
}

static int edit_fuse(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int i;
	int c;

	printf(">Select fuselage type\n");
	for (i = 0; i < FT_COUNT; i++)
		if (i != FT_GEODETIC || b->manf->geo)
			printf("[%c] %s\n", i + '1', describe_ftype(i));

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		i = c - '1';
		if (i < 0 || i >= FT_COUNT ||
		    (i == FT_GEODETIC && !b->manf->geo)) {
			putchar('?');
			continue;
		}
		b->fuse.typ = i;
		putchar('>');
		dump_bay_fuse(b);
		return 0;
	} while (1);

	return -EIO;
}

static int edit_elec(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int i;
	int c;

	printf(">Select electrics level\n");
	for (i = 0; i < ESL_COUNT; i++)
		if (i <= tn->esl)
			printf("[%c] %s\n", i + '1', describe_esl(i));

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		i = c - '1';
		if (i < 0 || i >= ESL_COUNT || i > tn->esl) {
			putchar('?');
			continue;
		}
		b->elec.esl = i;
		putchar('>');
		dump_elec_fuel(b);
		return 0;
	} while (1);

	return -EIO;
}

static int edit_tanks(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int v;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter fuel in tenths hours, 1 or 2 to dis/enable self-sealing, or 0 to cancel\n");
	} while (rc);
	if (v == 1) {
		b->tanks.sst = false;
		putchar('>');
		dump_elec_fuel(b);
		return 0;
	}
	if (v == 2) {
		if (b->tanks.sst)
			return 0;
		if (tn->sft) {
			b->tanks.sst = true;
			putchar('>');
			dump_elec_fuel(b);
			return 0;
		}
		fprintf(stderr, "Self sealing tanks not developed yet!\n");
		return -EINVAL;
	}
	if (v) {
		b->tanks.ht = v;
		putchar('>');
		dump_elec_fuel(b);
	}
	return 0;
}

static int dump_block(struct bomber *b, const struct entities *ent)
{
	unsigned int i, j;

	putchar('>');
	if (b->error)
		return -EINVAL;
	putchar('\n');
	for (i = 0; i < ent->nmanf; i++)
		if (b->manf == ent->manf[i])
			printf("M%c", i + '1');
	printf("TZ");
	printf("F%c", b->fuse.typ + '1');
	printf("L%c", b->elec.esl + '1');
	printf("E%d", b->engines.number);
	for (i = 0; i < ent->neng; i++)
		if (b->engines.typ == ent->eng[i])
			printf("E%c", i + 'A');
	for (i = LXN_NOSE; i < LXN_COUNT; i++)
		for (j = 0; j < ent->ngun; j++)
			if (b->turrets.typ[i] == ent->gun[j])
				printf("T%c", j + 'A');
	printf("WA%u\nWR%u\n", b->wing.area, b->wing.art);
	printf("CZ");
	for (i = 0; i < b->crew.n; i++) {
		printf("C%c", crew_to_letter(b->crew.men[i].pos));
		if (b->crew.men[i].gun)
			printf("C*%c", i + '1');
	}
	printf("BB%u\n", b->bay.cap);
	switch (b->bay.girth) {
	case BB_SMALL:
		printf("BS");
		break;
	case BB_MEDIUM:
		printf("BM");
		break;
	case BB_COOKIE:
		printf("BC");
		break;
	default:
		return -EINVAL;
	}
	printf("U%u\n", b->tanks.ht);
	printf("U%u\n", b->tanks.sst ? 2 : 1);
	return 0;
}

static int edit_techyear(struct tech_numbers *tn, const struct entities *ent)
{
	unsigned int v, i;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter year to set tech for, or 0 to cancel\n");
	} while (rc);
	putchar('>');
	if (v) {
		for (i = 0; i < ent->ntech; i++)
			ent->tech[i]->unlocked = ent->tech[i]->year <= v;
		return apply_techs(ent, tn);
	}
	return 0;
}

static int edit_tech(struct tech_numbers *tn, const struct entities *ent)
{
	unsigned int i, j;
	struct tech *t;
	int c;

	printf(">Select tech to toggle, @ for year, or 0 to cancel\n");
	for (i = 0; i < ent->ntech; i++) {
		char l, s;

		t = ent->tech[i];
		if (!t->year) {
			j = i + 1;
			continue;
		}
		if (!t->have_reqs && !t->unlocked)
			continue;
		if (i - j < 26)
			l = (i - j) + 'A';
		else
			l = (i - j - 26) + 'a';
		s = ' ';
		if (t->unlocked) {
			if (t->have_reqs)
				s = '*';
			else
				s = '!';
		}
		printf("[%c] %c %s (%d)\n", l, s, t->name, t->year);
	}

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}
		if (c == '@')
			return edit_techyear(tn, ent);

		i = c - 'A';
		if (i >= 0 && i < 26) {
			t = ent->tech[i + j];
		} else {
			i = c - 'a';
			if (i < 0 || i + j + 26 >= ent->ntech) {
				putchar('?');
				continue;
			}
			t = ent->tech[i + j + 26];
		}
		t->unlocked = !t->unlocked;
		putchar('>');
		return apply_techs(ent, tn);
	} while (1);

	return -EIO;
}

int edit_loop(struct bomber *b, struct tech_numbers *tn,
	      const struct entities *ent)
{
	const char *help = "[METWCBFLU] to edit, I to show, D to dump, Return to recalculate, QQ to quit\n";
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
	printf(help);

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == 'Q') {
			if (qc)
				break;
			qc = true;
			printf(">press again to quit\n");
			continue;
		}
		qc = false;
		switch (c) {
		case '\n':
			dump_bomber_calcs(b);
			continue;
		case 'h':
		case 'H':
			putchar('>');
			printf(help);
			continue;
		case 'i':
		case 'I':
			printf(">Info on current design\n");
			dump_bomber_info(b);
			continue;
		case 'm':
		case 'M':
			rc = edit_manf(b, tn, ent);
			break;
		case 'e':
		case 'E':
			rc = edit_eng(b, tn, ent);
			break;
		case 't':
		case 'T':
			rc = edit_guns(b, tn, ent);
			break;
		case 'w':
		case 'W':
			rc = edit_wing(b);
			break;
		case 'c':
		case 'C':
			rc = edit_crew(b, tn);
			break;
		case 'b':
		case 'B':
			rc = edit_bay(b, tn);
			break;
		case 'f':
		case 'F':
			rc = edit_fuse(b, tn);
			break;
		case 'l':
		case 'L':
			rc = edit_elec(b, tn);
			break;
		case 'u':
		case 'U':
			rc = edit_tanks(b, tn);
			break;
		case 'd':
		case 'D':
			rc = dump_block(b, ent);
			if (rc)
				putchar('?');
			continue;
		case 'r':
		case 'R':
			rc = edit_tech(tn, ent);
			break;
		case 'x':
		case 'X':
			/* Just a way to redisplay errors/warnings */
			putchar('>');
			putchar('\n');
			rc = 0;
			break;
		default:
			putchar('?');
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
	} while (1);

	putchar('\n');
	if (disable_cbreak_mode(&cooked))
		fprintf(stderr, "Failed to disable cbreak mode on tty\n");
	return rc;
}
