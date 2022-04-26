#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <math.h>
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

const char *describe_esl(enum elec_level esl)
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

const char *describe_navaid(enum nav_aid na)
{
	switch (na) {
	case NA_GEE:
		return "GEE";
	case NA_H2S:
		return "H₂S";
	case NA_OBOE:
		return "OBOE";
	default:
		return "error!  unknown navaid";
	}
}

const char *describe_refit(enum refit_level refit)
{
	switch (refit) {
	case REFIT_FRESH:
		return "Clean-sheet";
	case REFIT_MARK:
		return "Mark";
	case REFIT_MOD:
		return "Mod";
	case REFIT_DOCTRINE:
		return "Doctrine";
	default:
		return "error!  unknown refit";
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
	struct engine *m = b->engines.mou;

	printf("[E]ngines: %u × %s %s (%u hp, SCL %u",
	       b->engines.number, e->manu, e->name, e->bhp, e->scl);
	if (b->engines.egg)
		printf(", Power Egg");
	if (m != e)
		printf(", mounts for %s", m->name);
	putchar(')');
	putchar('\n');
}

static void dump_turrets(struct bomber *b)
{
	bool comma = false;
	unsigned int i;

	printf("[T]urrets:");
	for (i = LXN_NOSE; i < LXN_COUNT; i++) {
		const struct turret *t = b->turrets.typ[i];
		const struct turret *m = b->turrets.mou[i];

		if (t || m) {
			if (comma)
				printf(",");
			if (t)
				printf(" %s", t->name);
			if (t != m)
				printf(" (mount for %s)",
				       m ? m->name : "???");
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
	if (b->bay.load == b->bay.cap)
		printf("[B]omb bay: %ulb, %s", b->bay.cap,
		       describe_bbg(b->bay.girth));
	else
		printf("[B]omb bay: %u/%ulb, %s", b->bay.load, b->bay.cap,
		       describe_bbg(b->bay.girth));
	if (b->bay.csbs)
		printf(", CSBS");
	printf("; [F]uselage: %s\n", describe_ftype(b->fuse.typ));
}

static void dump_elec(struct bomber *b)
{
	unsigned int i;

	printf("e[L]ectrics: %s", describe_esl(b->elec.esl));
	for (i = 0; i < NA_COUNT; i++)
		if (b->elec.navaid[i])
			printf(", %s", describe_navaid(i));
	putchar('\n');
}

static void dump_fuel(struct bomber *b)
{
	printf("f[U]el: %u00lb%s, %d%% full\n", b->tanks.hlb,
	       b->tanks.sst ? ", self-sealing" : "",
	       b->tanks.pct);
}

static void dump_bomber_info(struct bomber *b)
{
	dump_manf(b);
	dump_engines(b);
	dump_turrets(b);
	dump_wing_crew(b);
	dump_bay_fuse(b);
	dump_elec(b);
	dump_fuel(b);

	if (b->refit)
		printf("Design is a %s refit; changes limited.\n",
		       describe_refit(b->refit));
	if (b->refit >= REFIT_MOD)
		printf("Max. take-off weight %ulb\n", b->mtow);

	if (b->error)
		printf("Design contains errors!  Please review [X].\n");
	else if (b->new)
		printf("Design contains warnings!  Consider reviewing [X].\n");
}

static void dump_bomber_ew(struct bomber *b)
{
	unsigned int i;

	for (i = 0; i < b->new; i++)
		printf(b->ew[i]);
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
	printf("Service ceiling: %.0fft; range: %.0fmi (%.1fhr); initial climb %.0ffpm\n",
	       b->ceiling * 1000.0f, b->range, b->tanks.hours, b->init_climb);
	printf("Ratings: defn %.1f/%.1f; fail %.1f; svp %.1f; accu %.1f\n",
	       b->defn[0], b->defn[1], b->fail * 100.0f,
	       b->serv * 100.0f, b->accu * 100.0f);
	printf("\t(roll %.1f, turn %.1f, evade %.1f, vuln %.2f (fr %.2f))\n",
	       b->roll_pen, b->turn_pen, b->evade_factor, b->vuln, b->tanks.ratio);
	printf("Cost: %.0f funds (core %.0f, fuse %.0f, wing %.0f, eng %.0f)\n",
	       b->cost, b->core_cost, b->fuse.cost, b->wing.cost,
	       b->engines.cost);
	printf("\t(tanks %.0f, gun %.0f, elec %.0f)\n",
	       b->tanks.cost, b->turrets.cost, b->elec.cost);
	printf("Prototype in %.0f days for %.0f funds\n",
	       b->tproto, b->cproto);
	printf("Production in %.0f days for %.0f funds\n",
	       b->tprod, b->cprod);
	dump_bomber_ew(b);
}

static void dump_limits(struct bomber *b, struct tech_numbers *tn)
{
	printf("Structure m.g.t.o.w.: %ulb\n", b->mtow);
	printf("Grass runways:    m.p.t.o.w. %u000lb; take-off speed %umph\n",
	       tn->rgg, tn->rgs);
	if (tn->rcs)
		printf("Concrete runways: m.p.t.o.w. %u000lb; take-off speed %umph\n",
		       tn->rcg, tn->rcs);
}

static int edit_manf(struct bomber *b, struct tech_numbers *tn,
		     const struct entities *ent)
{
	unsigned int i;
	int c;

	if (b->refit) {
		fprintf(stderr, "Cannot change manufacturer on refit!\n");
		return -EINVAL;
	}

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

static int edit_eng_mount(struct bomber *b, struct tech_numbers *tn,
			  const struct entities *ent)
{
	unsigned int i;
	int c;

	printf(">Overbuild mounts for engine (0 to cancel)\n");
	for (i = 0; i < ent->neng; i++)
		if (ent->eng[i] == b->engines.typ ||
		    ent->eng[i]->u == b->engines.typ)
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

		i = c - 'A';
		if (i < ent->neng && (ent->eng[i] == b->engines.typ ||
				      ent->eng[i]->u == b->engines.typ)) {
			b->engines.mou = ent->eng[i];
			putchar('>');
			dump_engines(b);
			return 0;
		}
		putchar('?');
	} while (1);

	return -EIO;
}

static int edit_eng(struct bomber *b, struct tech_numbers *tn,
		    const struct entities *ent)
{
	const struct bomber *p = b->parent;
	unsigned int i;
	int c;

	if (b->refit == REFIT_MOD && b->parent->engines.mou != b->parent->engines.typ) {
		/* Mod can upgrade to mounts' type */
		printf(">Select engine, or 0 to cancel\n");
	} else if (b->refit >= REFIT_MOD) {
		fprintf(stderr, "%s refit cannot change engines.\n", describe_refit(b->refit));
		return -EINVAL;
	} else {
		printf(">Select engine, @ for mounts, ");
		if (tn->ees)
			printf("+/- for power egg, ");
		/* Mark can change type but not number */
		if (!b->refit)
			printf("number, ");
		printf("or 0 to cancel\n");
	}
	for (i = 0; i < ent->neng; i++) {
		const struct engine *e = ent->eng[i];

		if (e->unlocked && (b->refit < REFIT_MOD ||
				    p->engines.typ == e ||
				    p->engines.mou == e))
			printf("[%c] %s %s\n", i + 'A', e->manu, e->name);
	}

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		if (c == '@' && b->refit < REFIT_MOD)
			return edit_eng_mount(b, tn, ent);
		if (c == '+' && tn->ees) {
			b->engines.egg = true;
			putchar('>');
			dump_engines(b);
			return 0;
		}
		if (c == '-') {
			b->engines.egg = false;
			putchar('>');
			dump_engines(b);
			return 0;
		}
		i = c - '0';
		if (i >= 1 && i < 9 && !b->refit) {
			b->engines.number = i;
			putchar('>');
			dump_engines(b);
			return 0;
		}
		i = c - 'A';
		if (i < 0 || i >= ent->neng || !ent->eng[i]->unlocked ||
		    (b->refit == REFIT_MOD &&
		     p->engines.typ != ent->eng[i] &&
		     p->engines.mou != ent->eng[i])) {
			putchar('?');
			continue;
		}
		b->engines.typ = ent->eng[i];
		b->engines.mou = ent->eng[i];
		putchar('>');
		dump_engines(b);
		return 0;
	} while (1);

	return -EIO;
}

static bool gun_mount_conflict(const struct bomber *b, const struct turret *t)
{
	if (t->lxn == LXN_NOSE && b->engines.odd)
		return true;
	if (t->lxn == LXN_VENTRAL && b->elec.navaid[NA_H2S])
		return true;
	if (t->slb && b->fuse.typ != FT_SLABBY)
		return true;
	return b->turrets.typ[t->lxn] &&
	       b->turrets.typ[t->lxn]->twt > t->twt;
}

static int edit_gun_mount(struct bomber *b, struct tech_numbers *tn,
			  const struct entities *ent)
{
	unsigned int i;
	int c;

	printf(">Prepare mounts for turret, or 0 to cancel\n");
	for (i = 0; i < ent->ngun; i++)
		if (!gun_mount_conflict(b, ent->gun[i]))
			printf("[%c] %s\n", i + 'A', ent->gun[i]->name);

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		i = c - 'A';
		if (i < 0 || i >= ent->ngun ||
		    gun_mount_conflict(b, ent->gun[i])) {
			putchar('?');
			continue;
		}
		b->turrets.mou[ent->gun[i]->lxn] = ent->gun[i];
		putchar('>');
		dump_turrets(b);
		return 0;
	} while (1);

	return -EIO;
}

static bool gun_conflict(const struct bomber *b, const struct turret *t)
{
	if (t->lxn == LXN_NOSE && b->engines.odd)
		return true;
	if (t->lxn == LXN_VENTRAL && b->elec.navaid[NA_H2S])
		return true;
	if (t->slb && b->fuse.typ != FT_SLABBY)
		return true;
	if (b->refit >= REFIT_MOD && (!b->turrets.mou[t->lxn] ||
				      b->turrets.mou[t->lxn]->twt < t->twt))
		return true;
	return b->turrets.typ[t->lxn] != NULL;
}

static int edit_guns(struct bomber *b, struct tech_numbers *tn,
		     const struct entities *ent)
{
	enum turret_location lxn;
	unsigned int i;
	int c;

	/* Mark can freely alter turrets.
	 * Mod can remove turrets, or install turrets into existing mounts.
	 * Doctrine can't alter turrets at all.
	 */

	if (b->refit >= REFIT_DOCTRINE) {
		fprintf(stderr, "%s refit cannot change turrets.\n", describe_refit(b->refit));
		return -EINVAL;
	}

	printf(">Select turret to add, number to remove, @ for mounts, or 0 to cancel\n");
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
			for (i = LXN_NOSE; i < LXN_COUNT; i++) {
				b->turrets.typ[i] = NULL;
				if (b->refit < REFIT_MOD)
					b->turrets.mou[i] = NULL;
			}
			putchar('>');
			dump_turrets(b);
			return 0;
		}

		if (c == '@' && b->refit < REFIT_MOD)
			return edit_gun_mount(b, tn, ent);
		i = c - '0';
		if (i >= LXN_NOSE && i < LXN_COUNT && b->turrets.typ[i]) {
			b->turrets.typ[i] = NULL;
			if (b->refit < REFIT_MOD)
				b->turrets.mou[i] = NULL;
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
		lxn = ent->gun[i]->lxn;
		b->turrets.typ[lxn] = ent->gun[i];
		if (b->refit < REFIT_MOD)
			b->turrets.mou[lxn] = ent->gun[i];
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

	if (b->refit) {
		fprintf(stderr, "Refit cannot change wing design.\n");
		return -EINVAL;
	}

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
	unsigned int pcount[CREW_CLASSES];
	unsigned int count[CREW_CLASSES];
	bool can_add[CREW_CLASSES];
	unsigned int on = 0;
	struct crewman *m;
	enum crewpos pos;
	bool gun = false;
	unsigned int i;
	char opts[7];
	int c;

	if (b->refit >= REFIT_MOD) {
		count_crew(&b->parent->crew, pcount);
		count_crew(&b->crew, count);
	}

	for (i = 0; i < CREW_CLASSES; i++) {
		if (b->refit >= REFIT_DOCTRINE) {
			can_add[i] = false;
		} else if (b->refit >= REFIT_MOD) {
			can_add[i] = count[i] < pcount[i];
		} else {
			can_add[i] = true;
		}
		if (can_add[i])
			opts[on++] = crew_to_letter(i);
	}
	if (on) {
		opts[on++] = 0;
		printf(">[%s] to add crew, number to remove, *num to toggle gunner, or 0 to cancel\n",
		       opts);
	} else {
		if (b->refit >= REFIT_DOCTRINE)
			printf(">*num to toggle gunner, or 0 to cancel\n");
		else
			printf(">Number to remove crew, *num to toggle gunner, or 0 to cancel\n");
	}
	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		if (c == 'Z' && b->refit < REFIT_DOCTRINE) {
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
				if (m->gun || (m->pos != CCLASS_E &&
					       m->pos != CCLASS_G)) {
					m->gun = !m->gun;
					putchar('>');
					dump_wing_crew(b);
					return 0;
				}
			} else if (b->crew.n && b->refit < REFIT_DOCTRINE) {
				for (; ++i < b->crew.n;)
					b->crew.men[i - 1] = b->crew.men[i];
				b->crew.n--;
				putchar('>');
				dump_wing_crew(b);
				return 0;
			}
		}
		pos = letter_to_crew(c);
		if (b->crew.n < MAX_CREW && pos < CREW_CLASSES &&
		    can_add[pos]) {
			m = &b->crew.men[b->crew.n++];
			m->pos = pos;
			m->gun = false;
			putchar('>');
			dump_wing_crew(b);
			return 0;
		}
		putchar('?');
	} while (1);

	return -EIO;
}

static int edit_bomb_load(struct bomber *b)
{
	unsigned int v;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter bombload in lb, or 0 to cancel\n");
	} while (rc);
	if (!v) {
		putchar('>');
		return 0;
	}
	if (b->refit) {
		if (v > b->bay.cap) {
			fprintf(stderr, "Cannot exceed capacity of %ulb\n",
				b->bay.cap);
			return -EINVAL;
		}
		putchar('>');
		b->bay.load = v;
		dump_bay_fuse(b);
		return 0;
	}
	putchar('>');
	b->bay.load = b->bay.cap = v;
	dump_bay_fuse(b);
	return 0;
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

static int edit_girth(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int i;
	int c;

	printf(">[");
	for (i = 0; i < BB_COUNT; i++)
		if (tn->bt[i])
			putchar("SMC"[i]);
	printf("] to set girth, or 0 to cancel\n");
	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		switch (c) {
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

static int edit_bay(struct bomber *b, struct tech_numbers *tn)
{
	bool sight = b->refit < REFIT_DOCTRINE;
	bool cap = !b->refit;
	int c;

	printf(">[B]ombload, ");
	if (cap)
		printf("ca[P]acity, [G]irth, ");
	if (sight && tn->csb)
		printf("[R]egular or [C]S bomb sight, ");
	printf("or 0 to cancel\n");

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
			return edit_bomb_load(b);
		case 'p':
		case 'P':
			if (cap)
				return edit_cap(b);
			break;
		case 'g':
		case 'G':
			if (cap)
				return edit_girth(b, tn);
			break;
		case 'r':
		case 'R':
			if (sight) {
				b->bay.csbs = false;
				putchar('>');
				dump_bay_fuse(b);
				return 0;
			}
			break;
		case 'c':
		case 'C':
			if (sight && tn->csb) {
				b->bay.csbs = true;
				putchar('>');
				dump_bay_fuse(b);
				return 0;
			}
			break;
		default:
			break;
		}
		putchar('?');
	} while (1);

	return -EIO;
}

static int edit_fuse(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int i;
	int c;

	if (b->refit) {
		fprintf(stderr, "%s refit cannot change fuselage type.\n", describe_refit(b->refit));
		return -EINVAL;
	}

	printf(">Select fuselage type, or 0 to cancel\n");
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

static int edit_nav(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int i;
	int c;

	if (b->refit >= REFIT_DOCTRINE) {
		fprintf(stderr, "%s refit cannot change navaids.\n", describe_refit(b->refit));
		return -EINVAL;
	}

	printf(">Select navaid letter to add, number to remove, or 0 to cancel\n");
	for (i = 0; i < NA_COUNT; i++)
		if (tn->na[i] && !b->elec.navaid[i])
			printf("[%c] %s\n", i + 'A', describe_navaid(i));
	for (i = 0; i < NA_COUNT; i++)
		if (b->elec.navaid[i])
			printf("[%c] %s\n", i + '1', describe_navaid(i));

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		if (c == 'Z') {
			for (i = 0; i < NA_COUNT; i++)
				b->elec.navaid[i] = false;
			putchar('>');
			dump_elec(b);
			return 0;
		}
		i = c - 'A';
		if (i >= 0 && i < NA_COUNT && tn->na[i]) {
			b->elec.navaid[i] = true;
			putchar('>');
			dump_elec(b);
			return 0;
		}
		i = c - '1';
		if (i >= 0 && i < NA_COUNT && b->elec.navaid[i]) {
			b->elec.navaid[i] = false;
			putchar('>');
			dump_elec(b);
			return 0;
		}
		putchar('?');
	} while (1);

	return -EIO;
}

static int edit_elec(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int i;
	int c;

	if (b->refit >= REFIT_DOCTRINE) {
		fprintf(stderr, "%s refit cannot change electrics.\n", describe_refit(b->refit));
		return -EINVAL;
	}
	if (b->refit >= REFIT_MOD) {
		printf(">Edit [N]avaids, or 0 to cancel\n");
	} else {
		printf(">Select electrics level, edit [N]avaids, or 0 to cancel\n");
		for (i = 0; i < ESL_COUNT; i++)
			if (i <= tn->esl)
				printf("[%c] %s\n", i + '1', describe_esl(i));
	}

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		if (c == 'N') {
			return edit_nav(b, tn);
		}
		i = c - '1';
		if (i < ESL_COUNT && i <= tn->esl && b->refit < REFIT_MOD) {
			b->elec.esl = i;
			putchar('>');
			dump_elec(b);
			return 0;
		}
		putchar('?');
	} while (1);

	return -EIO;
}

static int edit_fuel(struct bomber *b)
{
	unsigned int v;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter fuel capacity in hundreds lb, or 0 to cancel\n");
	} while (rc);

	if (v) {
		b->tanks.hlb = v;
		putchar('>');
		dump_fuel(b);
	}
	return 0;
}

static int edit_fuel_pct(struct bomber *b)
{
	unsigned int v;
	int rc;

	do {
		rc = edit_uint(&v);
		if (rc == -EIO)
			return rc;
		if (rc)
			printf("Enter fill level in percent, or 0 to cancel\n");
	} while (rc);

	if (v) {
		b->tanks.pct = v;
		putchar('>');
		dump_fuel(b);
	}
	return 0;
}

static int edit_tanks(struct bomber *b, struct tech_numbers *tn)
{
	bool sft = b->refit < REFIT_DOCTRINE && tn->sft;
	bool cap = b->refit < REFIT_MOD;
	int c;

	putchar('>');
	if (cap)
		printf("f[U]el capacity, ");
	printf("fuel [P]ercent, ");
	if (sft)
		printf("[R]egular or [S]elf-sealing, ");
	printf("or 0 to cancel\n");

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0') {
			putchar('>');
			return 0;
		}

		switch (c) {
		case 'u':
		case 'U':
			if (cap)
				return edit_fuel(b);
			break;
		case 'p':
		case 'P':
			return edit_fuel_pct(b);
		case 'r':
		case 'R':
			b->tanks.sst = false;
			putchar('>');
			dump_fuel(b);
			return 0;
		case 's':
		case 'S':
			if (sft) {
				b->tanks.sst = true;
				putchar('>');
				dump_fuel(b);
				return 0;
			}
			break;
		default:
			break;
		}
		putchar('?');
	} while (1);

	return -EIO;
}

static int auto_doctrine(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int mptow, mts, mtg;
	bool concrete = tn->rcs;
	int delta, c;

	/* Not in DOCTRINE refit mode, go away */
	if (b->refit < REFIT_DOCTRINE)
		return -EINVAL;
	if (concrete) {
		printf(">[G]rass or [C]oncrete, or 0 to cancel\n");
		do {
			c = getchar();
			if (c == EOF)
				return -EIO;
			if (c == '0') {
				putchar('>');
				return 0;
			}

			if (c == 'g' || c == 'G') {
				concrete = false;
				break;
			}
			if (c == 'c' || c == 'C') {
				concrete = true;
				break;
			}
			putchar('?');
		} while (1);
	}
	mts = concrete ? tn->rcs : tn->rgs;
	mtg = concrete ? tn->rcg : tn->rgg;
	mptow = floor(wing_lift(&b->wing, mts / 1.6f));
	mptow = min(mptow, mtg * 1000);
	mptow = min(mptow, b->mtow);
	delta = b->gross - mptow;
	if ((int)b->bay.load < delta) {
		fprintf(stderr, ">Too heavy\n");
		return -ERANGE; /* Too heavy even without load */
	}
	b->bay.load = min(((int)b->bay.load) - delta, b->bay.cap);
	putchar('>');
	dump_bay_fuse(b);
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
	printf("L%cLNZ", b->elec.esl + '1');
	for (i = 0; i < NA_COUNT; i++)
		if (b->elec.navaid[i])
			printf("LN%c", i + 'A');
	printf("E%d", b->engines.number);
	for (i = 0; i < ent->neng; i++)
		if (b->engines.typ == ent->eng[i])
			printf("E%c", i + 'A');
	if (b->engines.typ != b->engines.mou)
		for (i = 0; i < ent->neng; i++)
			if (b->engines.mou == ent->eng[i])
				printf("E@%c", i + 'A');
	printf("E%c", b->engines.egg ? '+' : '-');
	for (i = LXN_NOSE; i < LXN_COUNT; i++) {
		for (j = 0; j < ent->ngun; j++)
			if (b->turrets.typ[i] == ent->gun[j])
				printf("T%c", j + 'A');
		if (b->turrets.mou[i] != b->turrets.typ[i])
			for (j = 0; j < ent->ngun; j++)
				if (b->turrets.mou[i] == ent->gun[j])
					printf("T@%c", j + 'A');
	}
	printf("WA%u\nWR%u\n", b->wing.area, b->wing.art);
	printf("CZ");
	for (i = 0; i < b->crew.n; i++) {
		printf("C%c", crew_to_letter(b->crew.men[i].pos));
		if (b->crew.men[i].gun)
			printf("C*%c", i + '1');
	}
	printf("BB%u\n", b->bay.load);
	switch (b->bay.girth) {
	case BB_SMALL:
		printf("BGS");
		break;
	case BB_MEDIUM:
		printf("BGM");
		break;
	case BB_COOKIE:
		printf("BGC");
		break;
	default:
		return -EINVAL;
	}
	if (b->bay.cap != b->bay.load)
		printf("BP%u\n", b->bay.cap);
	printf("B%c", b->bay.csbs ? 'C' : 'R');
	printf("U%c", b->tanks.sst ? 'S' : 'R');
	printf("UU%u\n", b->tanks.hlb);
	printf("UP%u\n", b->tanks.pct);
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

static int edit_loop(struct bomber *b, struct tech_numbers *tn,
		     const struct entities *ent);

static int edit_refit(const struct bomber *b, const struct tech_numbers *tn,
		      const struct entities *ent)
{
	struct tech_numbers tn2 = *tn;
	struct bomber b2 = *b;
	int c;

	b2.parent = b;
	printf(">Select refit level (mar[K], m[O]d, [D]octrine) or 0 to cancel\n");

	do {
		c = getchar();
		if (c == EOF)
			break;
		if (c == '0')
			return 0;

		switch (c) {
		case 'k':
		case 'K':
			b2.refit = REFIT_MARK;
			calc_bomber(&b2, &tn2);
			putchar('>');
			return edit_loop(&b2, &tn2, ent);
		case 'o':
		case 'O':
			b2.refit = REFIT_MOD;
			calc_bomber(&b2, &tn2);
			putchar('>');
			return edit_loop(&b2, &tn2, ent);
		case 'd':
		case 'D':
			b2.refit = REFIT_DOCTRINE;
			calc_bomber(&b2, &tn2);
			putchar('>');
			return edit_loop(&b2, &tn2, ent);
		default:
			putchar('?');
			break;
		}
	} while (1);

	return -EIO;
}

static int edit_loop(struct bomber *b, struct tech_numbers *tn,
		     const struct entities *ent)
{
	const char *help = "[METWCBFLU] to edit, I to show, D to dump, Return to recalculate, QQ to quit\n";
	bool qc = false;
	int rc, c;

	dump_bomber_info(b);
	dump_bomber_calcs(b);
	printf(help);

	do {
		c = getchar();
		if (c == EOF)
			return -EIO;
		if (c == 'Q') {
			if (qc)
				return 1;
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
		case 'j':
		case 'J':
			/* Exit this refit */
			if (b->parent)
				return 0;
			putchar('?');
			continue;
		case 'k':
		case 'K':
			rc = edit_refit(b, tn, ent);
			if (rc == 1) // QQ
				return 1;
			putchar('>');
			break;
		case 'v':
		case 'V':
			putchar('>');
			putchar('\n');
			dump_limits(b, tn);
			continue;
		case 'a':
		case 'A':
			rc = auto_doctrine(b, tn);
			break;
		case 'x':
		case 'X':
			/* Redisplay errors/warnings */
			putchar('>');
			putchar('\n');
			dump_bomber_ew(b);
			continue;
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
	
	return rc;
}

int editor(struct bomber *b, struct tech_numbers *tn,
	   const struct entities *ent)
{
	struct termios cooked;
	int rc;

	rc = enable_cbreak_mode(&cooked);
	if (rc) {
		fprintf(stderr, "Failed to enable cbreak mode on tty\n");
		return rc;
	}
	rc = edit_loop(b, tn, ent);
	putchar('\n');
	if (disable_cbreak_mode(&cooked))
		fprintf(stderr, "Failed to disable cbreak mode on tty\n");
	return rc;
}
