#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "calc.h"

#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) < (b) ? (b) : (a))

void init_bomber(struct bomber *b, struct manf *m, struct engine *e)
{
	memset(b, 0, sizeof(*b));

	b->manf = m;
	b->engines.number = 1;
	b->engines.typ = e;
	b->wing.area = 100;
	b->wing.art = 70;
	b->crew.n = 2;
	b->crew.men[0] = (struct crewman){.pos = CCLASS_P};
	b->crew.men[1] = (struct crewman){.pos = CCLASS_N};
	b->bay.cap = 1000;
	b->tanks.ht = 50;
}

static int calc_engines(struct bomber *b, struct tech_numbers *tn)
{
	struct engines *e = &b->engines;
	float ees = 1, eet = 1, eec = 1;
	float mounts = 0;

	e->odd = e->number & 1;
	e->manumatch = !strcmp(e->typ->manu, b->manf->eman);
	if (e->egg) {
		if (!tn->ees || !tn->eet || !tn->eec) {
			fprintf(stderr, "Power Egg mounts not developed yet!\n");
			b->error = true;
		}
		ees = tn->ees / 100.0f;
		eet = tn->eet / 100.0f;
		eec = tn->eec / 100.0f;
	}
	e->power_factor = e->number - (e->odd ? 0.1f : 0.0f);
	e->vuln = e->typ->vul / 100.0f;
	e->rely1 = 1.0f - powf(1.0f - e->typ->fai / 100.0f, e->number);
	if (e->number > 1)
		e->rely2 = e->rely1 - e->number * (e->typ->fai / 100.0f) *
				      powf(1.0f - (e->typ->fai / 100.0f),
					   e->number - 1);
	else
		e->rely2 = e->rely1 / 10.0f;
	e->serv = 1.0f - powf(1.0f - (e->typ->svc / 100.0f) * ees,
			      e->number);
	e->cost = e->number * e->typ->cos * eec * (1.0f + 0.5f * tn->emc / 100.0f);
	if (e->number > 3) {
		e->cost *= tn->g4c / 100.0f;
		if (!tn->g4c || !tn->g4t) {
			fprintf(stderr, "Four-engined bombers not developed yet!\n");
			b->error = true;
		}
	}
	e->scl = e->typ->scl;
	e->fuelrate = e->number * e->typ->bhp * 0.36f;
	mounts = 220 * e->number;
	if (e->odd)
		mounts -= 55;
	if (e->number > 3)
		mounts += tn->g4t * (e->number / 2 - 1);
	if (e->manumatch)
		mounts *= 0.8f;
	e->tare = e->number * e->typ->twt * eet + mounts;
	e->drag = e->number * e->typ->drg * tn->edf / 100.0f;
	return 0;
}

static int gcr[GC_COUNT] = {
	[GC_FRONT] = 1,
	[GC_BEAM_HIGH] = 2,
	[GC_BEAM_LOW] = 1,
	[GC_TAIL_HIGH] = 2,
	[GC_TAIL_LOW] = 3,
	[GC_BENEATH] = 3,
};

static int calc_turrets(struct bomber *b, struct tech_numbers *tn)
{
	struct turrets *t = &b->turrets;
	unsigned int i, j;

	t->need_gunners = 0;
	t->drag = 0;
	t->tare = 0;
	t->ammo = 0;
	t->serv = 1.0f;
	t->cost = 0;
	if (t->typ[LXN_NOSE] && b->engines.odd) {
		fprintf(stderr, "Turret in nose position conflicts with engine\n");
		b->error = true;
	}
	for (j = 0; j < GC_COUNT; j++)
		t->gc[j] = 0;
	for (i = LXN_NOSE; i < LXN_COUNT; i++) {
		struct turret *g = t->typ[i];
		float tare;

		if (!g)
			continue;
		t->need_gunners++;
		t->drag += g->drg * tn->gdf;
		tare = g->twt * tn->gtf / 100.0f;
		t->tare += tare;
		t->ammo += g->gun * tn->gam;
		t->serv *= 1.0f - g->srv / 100.0f;
		t->cost += 3.0f * tare + g->gun * tn->gcf / 10.0f + g->gun * tn->gac / 10.0f;
		for (j = 0; j < GC_COUNT; j++)
			t->gc[j] += g->gc[j] / 10.0f;
	}
	t->serv = 1.0f - t->serv;
	t->rate[0] = t->rate[1] = 0;
	for (j = 0; j < GC_COUNT; j++) {
		float x = gcr[j] / (1.0f + t->gc[j]);

		if (j != GC_BENEATH)
			t->rate[0] += x;
		t->rate[1] += x;
	}
	return 0;
}

static int calc_wing(struct bomber *b, struct tech_numbers *tn)
{
	struct wing *w = &b->wing;
	float arpen, epen;

	w->ar = w->art / 10.0f;
	/* Make sure there's no risk of calculations blowing up */
	if (w->ar < 1.0) {
		fprintf(stderr, "Wing aspect ratio too low\n");
		b->error = true;
		return -EINVAL;
	}
	w->span = sqrt(w->area * w->ar);
	w->chord = sqrt(w->area / w->ar);
	w->cl = M_PI * M_PI / 6.0f / (1.0f + 2.0f / w->ar);
	w->ld = M_PI * sqrt(w->ar) * (tn->wld / 100.0f) *
				     (b->manf->wld / 100.0f);
	arpen = sqrt(max(w->ar, b->manf->wap)) / 6.0f;
	w->tare = powf(w->span, tn->wts / 100.0f) *
		  powf(w->chord, tn->wtc / 100.0f) *
		  (tn->wtf / 100.0f);
	arpen = max(w->ar, 6.0f) / 6.0f;
	epen = b->engines.number > 3 ? b->manf->wc4 / 100.0f : 1.0f;
	w->cost = powf(w->tare, b->manf->wcp / 100.0f) *
		  (tn->wcf / 100.0f) * (b->manf->wcf / 100.0f) *
		  arpen * epen / 12.0f;
	/* wl and drag need b->gross, fill in later */
	return 0;
}

static int calc_crew(struct bomber *b, struct tech_numbers *tn)
{
	struct crew *c = &b->crew;
	unsigned int i;

	c->gunners = 0;
	c->dc = 0;
	for (i = 0; i < c->n; i++) {
		struct crewman *m = c->men + i;

		if (m->pos == CCLASS_G || m->gun)
			c->gunners++;
		if (m->pos == CCLASS_E)
			c->dc += m->gun ? 0.75f : 1.0f;
		if (m->pos == CCLASS_W)
			c->dc += m->gun ? 0.45f : 0.6f;
	}
	c->tare = c->n * tn->cmi;
	c->gross = c->n * 168;
	if (c->gunners < b->turrets.need_gunners) {
		fprintf(stderr, "Fewer gunners than turrets, defence will be weakened\n");
		b->warning = true;
	}
	c->cct = c->tare * (tn->ccc / 100.0f - 1.0f);
	c->es = tn->ces / 100.0f;
	return 0;
}

static int calc_bombbay(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int bbb = (tn->bbb + b->manf->bbb) * 1000;
	struct bombbay *a = &b->bay;

	if (a->girth < 0 || a->girth >= BB_COUNT) { /* can't happen */
		fprintf(stderr, "Nonexistent bombbay girth\n");
		b->error = true;
		return -EINVAL;
	}
	a->factor = (tn->bt[a->girth] / 1000.0f) *
		    (b->manf->bt[a->girth] / 100.0f);
	if (a->cap > bbb)
		a->bigfactor = (a->cap - bbb) / (tn->bbf * 1e5f);
	else
		a->bigfactor = 0.0f;
	a->tare = a->cap * (a->factor + a->bigfactor);
	a->cookie = a->girth == BB_COOKIE ||
		    (a->girth == BB_MEDIUM && tn->bmc);
	return 0;
}

static int calc_fuselage(struct bomber *b, struct tech_numbers *tn)
{
	struct fuselage *f = &b->fuse;

	/* First need core_tare, as input to fuse_tare */
	b->core_tare = (b->turrets.tare + b->crew.tare + b->bay.tare) *
		       b->manf->act / 100.0f;
	if (f->typ < 0 || f->typ >= FT_COUNT) { /* can't happen */
		fprintf(stderr, "Nonexistent fuselage type\n");
		b->error = true;
		return -EINVAL;
	}
	if (f->typ == FT_GEODETIC && !b->manf->geo) {
		fprintf(stderr, "This manufacturer cannot design geodetics\n");
		b->error = true;
	}
	f->tare = b->core_tare * (tn->ft[f->typ] / 100.0f) *
		  b->manf->ft[f->typ] / 100.0f;
	f->serv = tn->fs[f->typ] / 1000.0f;
	f->fail = tn->ff[f->typ] / 1000.0f;
	f->cost = f->tare * 1.2f * (b->manf->acc / 100.0f) *
		  (tn->fc[f->typ] / 100.0f);
	f->vuln = tn->fv[f->typ] / 100.0f;
	/* drag needs b->tare, fill in later */
	return 0;
}

static int calc_electrics(struct bomber *b, struct tech_numbers *tn)
{
	struct electrics *e = &b->elec;

	if (e->esl > tn->esl) {
		fprintf(stderr, "Level %d electrics not developed yet\n", e->esl);
		b->error = true;
	}
	/* This is all hard-coded; datafiles / techlevels don't get to
	 * change these coefficients.
	 */
	switch (e->esl) {
	case ESL_LOW:
		e->cost = 90.0f;
		break;
	case ESL_HIGH:
		e->cost = 600.0f / max(b->engines.number, 1);
		break;
	case ESL_STABLE:
		e->cost = (b->fuse.typ == FT_SLENDER ? 1200.0f : 900.0f) +
			  500.0f / max(b->engines.number, 1);
		break;
	default: /* can't happen */
		fprintf(stderr, "Nonexistent electric supply level\n");
		b->error = true;
		return -EINVAL;
	}
	return 0;
}

static int calc_tanks(struct bomber *b, struct tech_numbers *tn)
{
	struct tanks *t = &b->tanks;

	t->hours = t->ht / 10.0f;
	t->mass = b->engines.fuelrate * t->hours;
	t->tare = t->mass * (tn->fut / 1000.0f);
	t->cost = t->tare * (tn->fuc / 100.0f);
	t->ratio = t->mass / max(b->wing.tare, 1.0f);
	if (t->ratio > 2.5f) {
		fprintf(stderr, "Wing is crammed with fuel, vulnerability high!\n");
		b->warning = true;
	}
	t->vuln = t->ratio * tn->fuv / 400.0f;
	if (t->sst) {
		if (!tn->sft || !tn->sfc || !tn->sfv) {
			fprintf(stderr, "Self sealing tanks not developed yet\n");
			b->error = true;
		}
		t->tare *= tn->sft / 100.0f;
		t->cost *= tn->sfc / 100.0f; /* note ignores SFT */
		t->vuln *= tn->sfv / 100.0f;
	}
	if (b->fuse.typ == FT_GEODETIC)
		t->vuln *= tn->fgv / 100.0f;
	return 0;
}

/* altitude in thousands ft */
static float engine_power(const struct engines *e, float alt)
{
	float ffth = 16.0f, mfth = 10.25, scp = 0.02f;
	float msp, fsp = 0;

	switch (e->scl) {
	case 3:
		ffth = 21.0f;
		mfth = 12.0f;
		scp = 0.06f;
		/* fallthrough */
	case 2:
		fsp = expf(min(ffth - alt, 0.0f) / 25.1f) - scp;
		/* fallthrough */
	case 1:
		msp = expf(min(mfth - alt, 0.0f) / 25.1f);
		break;
	default: /* bad data */
		return 0.0f;
	}

	return e->power_factor * e->typ->bhp * max(msp, fsp);
}

/* lift in lbf, altitude in thousands ft */
static float wing_minv(const struct wing *w, float lift, float alt)
{
	float rho = 0.0075f * expf(-alt / 25.1f);

	return sqrt(lift * 2.0f / (w->cl * rho * w->area)) * 15.0f / 22.0f;
}

/* altitude in thousands ft */
static float climb_rate(const struct bomber *b, float alt)
{
	float v = wing_minv(&b->wing, b->gross, alt);
	float lpwr = b->drag * v / 375.0f, pwr, cpwr;

	pwr = engine_power(&b->engines, alt);
	cpwr = (pwr - lpwr) * 0.52f;
	return cpwr * 33e3f / b->gross;
}

/* altitude in thousands ft */
static float airspeed(const struct bomber *b, float alt)
{
	float pwr = engine_power(&b->engines, alt);

	return pwr * 375.0f / b->drag;
}

static int calc_ceiling(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int alt; // units of 500ft
	float tim = 0; // minutes

	for (alt = 0; alt <= 70; alt++) {
		float c = climb_rate(b, alt * 0.5f);

		if (c < 400.0f)
			break;
		if (tim > tn->clt)
			break;
		tim += 500.0f / c;
	}
	b->ceiling = alt * 0.5f;
	return 0;
}

static int calc_perf(struct bomber *b, struct tech_numbers *tn)
{
	int rc;

	b->tare = b->core_tare + b->fuse.tare + b->tanks.tare +
		  b->wing.tare * tn->fwt / 100.0f +
		  b->engines.tare * tn->etf / 100.0f;
	b->gross = b->tare + b->tanks.mass * 0.6f + b->turrets.ammo +
		   b->crew.gross + b->bay.cap;
	b->wing.wl = b->gross / max(b->wing.area, 1.0f);
	b->wing.drag = b->gross / max(b->wing.ld, 1.0f);
	b->fuse.drag = sqrt(b->tare - b->wing.tare) *
		       (b->manf->fd[b->fuse.typ] / 100.0f) *
		       (tn->fd[b->fuse.typ] / 10.0f);
	b->drag = b->wing.drag + b->fuse.drag + b->engines.drag +
		  b->turrets.drag;
	b->takeoff_spd = wing_minv(&b->wing, b->gross, 0.0f) * 1.6f;
	rc = calc_ceiling(b, tn);
	if (rc)
		return rc;
	b->cruise_alt = min(b->ceiling, 10.0f) +
			max(b->ceiling - 10.0f, 0) / 2.0f;
	b->cruise_spd = airspeed(b, b->cruise_alt);
	b->deck_spd = airspeed(b, 0.0f);
	b->ferry = b->tanks.hours * b->cruise_spd;
	b->range = b->ferry * 0.6f - 150.0f;
	return 0;
}

static int calc_rely(struct bomber *b, struct tech_numbers *tn)
{
	b->serv = 1.0f - (b->engines.serv * 6.0f + b->turrets.serv +
			  b->fuse.serv + b->manf->svp / 100.0f);
	b->fail = b->engines.rely1 * 2.0f + b->engines.rely2 * 30.0f +
		  b->fuse.fail;
	return 0;
}

static int calc_combat(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int schrage;

	b->roll_pen = sqrt(b->wing.ar);
	b->turn_pen = sqrt(max(b->wing.wl - b->manf->tpl, 0.0f));
	b->manu_pen = b->roll_pen + b->turn_pen;
	b->evade_factor = (max(30.0f - b->ceiling, 3.0f) / 10.0f) *
			  sqrt(max(300.0f - b->cruise_spd, 10.0f)) *
			  (1.0f - 0.3f / max(b->manu_pen - 4.5f, 0.5f));
	b->vuln = (b->engines.vuln + b->fuse.vuln) *
		  max(4.0f - b->crew.dc, 1.0f) +
		  b->tanks.vuln;
	b->flak_factor = b->vuln * 3.0f *
			 sqrt(max(30.0f - b->ceiling, 3.0f));
	for (schrage = 0; schrage < 2; schrage++) {
		b->fight_factor[schrage] = powf(b->evade_factor, 0.7f) *
					   (b->vuln * 4.0f +
					    b->turrets.rate[schrage]) /
					   3.0f;
		b->defn[schrage] = b->fight_factor[schrage] +
				   b->flak_factor;
	}
	return 0;
}

static int calc_cost(struct bomber *b, struct tech_numbers *tn)
{
	b->core_cost = (b->core_tare + b->crew.cct) *
		       (b->manf->acc / 100.0f) *
		       (tn->cc[b->fuse.typ] / 100.0f) *
		       (b->engines.number > 2 ? 2.0f : 1.0f);
	b->cost = b->engines.cost + b->turrets.cost + b->core_cost +
		  b->fuse.cost + b->wing.cost + b->elec.cost;
	return 0;
}

static int calc_dev(struct bomber *b, struct tech_numbers *tn)
{
	b->tproto = powf(b->gross, 0.3) * powf(b->cost, 0.2) *
		    100.0f / max(b->manf->bof, 1);
	b->tprod = powf(b->gross, 0.4) * powf(b->cost, 0.2) *
		   60.0f / max(b->manf->bof, 1);
	b->cproto = b->cost * 15.0f;
	b->cprod = b->cost * 30.0f;
	return 0;
}

int calc_bomber(struct bomber *b, struct tech_numbers *tn)
{
	int rc;

	rc = calc_engines(b, tn);
	if (rc)
		return rc;

	rc = calc_turrets(b, tn);
	if (rc)
		return rc;

	rc = calc_wing(b, tn);
	if (rc)
		return rc;

	rc = calc_crew(b, tn);
	if (rc)
		return rc;

	rc = calc_bombbay(b, tn);
	if (rc)
		return rc;

	rc = calc_fuselage(b, tn);
	if (rc)
		return rc;

	rc = calc_electrics(b, tn);
	if (rc)
		return rc;

	rc = calc_tanks(b, tn);
	if (rc)
		return rc;

	rc = calc_perf(b, tn);
	if (rc)
		return rc;

	rc = calc_rely(b, tn);
	if (rc)
		return rc;

	rc = calc_combat(b, tn);
	if (rc)
		return rc;

	rc = calc_cost(b, tn);
	if (rc)
		return rc;

	rc = calc_dev(b, tn);
	if (rc)
		return rc;

	return 0;
}

const char *describe_ftype(enum fuse_type ft)
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

char crew_to_letter(enum crewpos c)
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

enum crewpos letter_to_crew(char c)
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

void dump_bomber_info(struct bomber *b)
{
	struct engine *e = b->engines.typ;
	bool comma = false;
	unsigned int i, j;
	char crew[33];

	printf("[M]anufacturer: %s\n", b->manf->name);
	printf("[E]ngines: %u × %s %s (%u hp, SCL %u%s)\n",
	       b->engines.number, e->manu, e->name, e->bhp, e->scl,
	       b->engines.egg ? ", Power Egg" : "");
	printf("[T]urrets:");
	for (i = LXN_NOSE; i < LXN_COUNT; i++) {
		struct turret *t = b->turrets.typ[i];

		if (t) {
			printf("%s %s", comma ? "," : "", t->name);
			comma = true;
		}
	}
	putchar('\n');
	printf("[W]ing: %u sq ft; aspect ratio %.1f\n",
	       b->wing.area, b->wing.ar);
	j = 0;
	for (i = 0; i < b->crew.n; i++) {
		struct crewman *m = b->crew.men + i;

		crew[j++] = crew_to_letter(m->pos);
		if (m->gun)
			crew[j++] = '*';
	}
	crew[j] = 0;
	printf("[C]rew: %s\n", crew);
	printf("[B]omb bay: %ulb, %s\n", b->bay.cap,
	       describe_bbg(b->bay.girth));
	printf("[F]uselage: %s\n", describe_ftype(b->fuse.typ));
	printf("E[l]ectrics: %s\n", describe_esl(b->elec.esl));
	printf("F[u]el: %.1f hours%s\n", b->tanks.hours,
	       b->tanks.sst ? ", self-sealing" : "");
	if (b->error)
		printf("Design contains errors!  Please review.\n");
	else if (b->warning)
		printf("Design contains warnings!  Consider reviewing.\n");
}

void dump_bomber_calcs(struct bomber *b)
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
	       b->ceiling * 1000.0f, b->range, climb_rate(b, 0.0f));
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