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

static const char *describe_bbg(enum bb_girth girth)
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

#if 0 /* not used yet */
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
#endif

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
	       b->wing.area, b->wing.ar);
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
	printf("; f[U]el: %.1f hours%s\n", b->tanks.hours,
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
		printf("Design contains errors!  Please review.\n");
	else if (b->warning)
		printf("Design contains warnings!  Consider reviewing.\n");
}

static void dump_bomber_calcs(struct bomber *b)
{
	printf("Dimensions: span %.1fft, chord %.1fft\n",
	       b->wing.span, b->wing.chord);
	printf("Weights: tare %.0flb, gross %.0flb; wing loading %.1flb/sq ft\n", b->tare, b->gross, b->wing.wl);
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
	printf("\t(roll %.1f, turn %.1f, evade %.1f, vuln %.2f (fr %.1f))\n",
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

int edit_loop(struct bomber *b, struct tech_numbers *tn,
	      const struct entities *ent)
{
	const char *help = "[METWCBFLU] to edit, I to show, Return to recalculate, QQ to quit\n";
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
