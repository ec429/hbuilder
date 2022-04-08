#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "calc.h"
#include "edit.h"

#define min(a, b)	((a) < (b) ? (a) : (b))
#define max(a, b)	((a) < (b) ? (b) : (a))

void init_bomber(struct bomber *b, struct manf *m, struct engine *e)
{
	memset(b, 0, sizeof(*b));

	b->manf = m;
	b->engines.number = 1;
	b->engines.typ = e;
	b->wing.area = 200;
	b->wing.art = 70;
	b->crew.n = 2;
	b->crew.men[0] = (struct crewman){.pos = CCLASS_P};
	b->crew.men[1] = (struct crewman){.pos = CCLASS_N};
	b->bay.cap = 1000;
	b->tanks.ht = 60;
}

static int calc_engines(struct bomber *b, struct tech_numbers *tn)
{
	struct engines *e = &b->engines;
	float ees = 1, eet = 1, eec = 1;
	float mounts = 0;

	if (!e->typ->unlocked) {
		fprintf(stderr, "%s not developed yet!\n", e->typ->name);
		b->error = true;
	}
	e->odd = e->number & 1;
	e->manumatch = b->manf->eman && !strcmp(e->typ->manu, b->manf->eman);
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
	e->rely1 = 1.0f - powf(1.0f - e->typ->fai / 1000.0f, e->number);
	if (e->number > 1)
		e->rely2 = e->rely1 - e->number * (e->typ->fai / 1000.0f) *
				      powf(1.0f - (e->typ->fai / 1000.0f),
					   e->number - 1);
	else
		e->rely2 = e->rely1;
	e->serv = 1.0f - powf(1.0f - (e->typ->svc / 1000.0f) * ees,
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
		fprintf(stderr, "Turret in nose position conflicts with engine!\n");
		b->error = true;
	}
	for (j = 0; j < GC_COUNT; j++)
		t->gc[j] = 0;
	for (i = LXN_NOSE; i < LXN_COUNT; i++) {
		struct turret *g = t->typ[i];
		float tare;

		if (!g)
			continue;
		if (!g->unlocked) {
			fprintf(stderr, "%s not developed yet!\n", g->name);
			b->error = true;
		}
		if (g->slb && b->fuse.typ != FT_SLABBY) {
			fprintf(stderr, "%s requires slab-sided fuselage!\n", g->name);
			b->error = true;
		}
		t->need_gunners++;
		t->drag += g->drg * tn->gdf;
		tare = g->twt * tn->gtf / 100.0f;
		t->tare += tare;
		t->ammo += g->gun * tn->gam;
		t->serv *= 1.0f - g->srv / 1000.0f;
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
		fprintf(stderr, "Wing aspect ratio too low!\n");
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
		  (tn->wtf / 100.0f) * arpen;
	arpen = max(w->ar, 6.0f) / 6.0f;
	epen = b->engines.number > 3 ? b->manf->wc4 / 100.0f : 1.0f;
	w->cost = powf(w->tare, b->manf->wcp / 100.0f) *
		  (tn->wcf / 100.0f) * (b->manf->wcf / 100.0f) *
		  arpen * epen / 12.0f;
	/* wl and drag need b->gross, fill in later */
	return 0;
}

const char *crew_name(enum crewpos c)
{
	switch (c) {
	case CCLASS_P:
		return "Pilot";
	case CCLASS_N:
		return "Navigator";
	case CCLASS_B:
		return "Bomb-aimer";
	case CCLASS_W:
		return "Wireless-op";
	case CCLASS_E:
		return "Engineer";
	case CCLASS_G:
		return "Gunner";
	default:
		return "Unknown crew";
	}
}

static int calc_crew(struct bomber *b, struct tech_numbers *tn)
{
	struct crew *c = &b->crew;
	unsigned int i;

	c->gunners = 0;
	c->pilot = c->nav = false;
	c->dc = 0;
	for (i = 0; i < c->n; i++) {
		struct crewman *m = c->men + i;

		if (m->gun && m->pos != CCLASS_B && m->pos != CCLASS_W) {
			fprintf(stderr, "%s cannot dual-role!\n",
				crew_name(m->pos));
			b->error = true;
		}
		if (m->pos == CCLASS_P)
			c->pilot = true;
		if (m->pos == CCLASS_N)
			c->nav = true;
		if (m->pos == CCLASS_G || m->gun)
			c->gunners++;
		if (m->pos == CCLASS_E)
			c->dc += m->gun ? 0.75f : 1.0f;
		if (m->pos == CCLASS_W)
			c->dc += m->gun ? 0.45f : 0.6f;
	}
	if (!c->pilot) {
		fprintf(stderr, "Crew must include a pilot!\n");
		b->error = true;
	}
	if (!c->nav) {
		fprintf(stderr, "Crew must include a navigator!\n");
		b->error = true;
	}
	c->tare = c->n * tn->cmi;
	c->gross = c->n * 168;
	if (c->gunners < b->turrets.need_gunners) {
		fprintf(stderr, "Fewer gunners than turrets, defence will be weakened.\n");
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
		fprintf(stderr, "Nonexistent bombbay girth!\n");
		b->error = true;
		return -EINVAL;
	}
	if (!tn->bt[a->girth]) {
		fprintf(stderr, "%s not developed yet!\n",
			describe_bbg(a->girth));
		b->error = true;
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
		fprintf(stderr, "Nonexistent fuselage type!\n");
		b->error = true;
		return -EINVAL;
	}
	if (f->typ == FT_GEODETIC && !b->manf->geo) {
		fprintf(stderr, "This manufacturer cannot design geodetics!\n");
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
		fprintf(stderr, "Electrics %s not developed yet!\n",
			describe_esl(e->esl));
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
		fprintf(stderr, "Nonexistent electric supply level!\n");
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
	if (t->ratio > (t->sst ? 2.5f : 2.0f)) {
		fprintf(stderr, "Wing is crammed with fuel, vulnerability high.\n");
		b->warning = true;
	}
	t->vuln = t->ratio * tn->fuv / 400.0f;
	if (t->sst) {
		if (!tn->sft || !tn->sfc || !tn->sfv) {
			fprintf(stderr, "Self sealing tanks not developed yet!\n");
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
	float nwd, a, c, m;

	/* drag (other than wing) scales with v², and is normalised
	 * to 200mph.  So we should have:
	 * v * WD + v³ * NWD / 200² = pwr * 375
	 * But this appears to make Mosquitos impossible, and in any
	 * case doesn't allow for AoA dependence of NWD.  So let's
	 * instead scale NWD to v, yielding:
	 * v * WD + v² * NWD / 200 = pwr * 375
	 * This is a quadratic, solve by the well-known formula.
	 */
	nwd = b->drag - b->wing.drag;
	a = nwd / 200.0f;
	c = -pwr * 375.0f;
	m = sqrt(b->wing.drag * b->wing.drag - 4.0f * a * c);
	return (m - b->wing.drag) / (2.0f * a);
}

static int calc_ceiling(struct bomber *b, struct tech_numbers *tn)
{
	unsigned int alt; // units of 500ft
	float tim = 0; // minutes

	for (alt = 0; alt <= 70; alt++) {
		float c = climb_rate(b, alt * 0.5f);

		if (c < 520.0f)
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
	if (b->takeoff_spd > 120.0f) {
		fprintf(stderr, "Take-off speed is dangerously high!\n");
		b->error = true;
	} else if (b->takeoff_spd > 110.0f) {
		fprintf(stderr, "Take-off speed is worryingly high.\n");
		b->warning = true;
	}
	rc = calc_ceiling(b, tn);
	if (rc)
		return rc;
	b->cruise_alt = min(b->ceiling, 10.0f) +
			max(b->ceiling - 10.0f, 0) / 2.0f;
	b->cruise_spd = airspeed(b, b->cruise_alt);
	b->init_climb = climb_rate(b, 0.0f);
	if (b->init_climb < 400.0f) {
		fprintf(stderr, "Design can barely take off!\n");
		b->error = true;
	} else if (b->init_climb < 640.0f) {
		fprintf(stderr, "Climb rate is very slow.\n");
		b->warning = true;
	}
	b->deck_spd = airspeed(b, 0.0f);
	b->ferry = b->tanks.hours * b->cruise_spd;
	b->range = max(b->ferry * 0.6f - 150.0f, 0.0f);
	if (b->range < 300.0f) {
		fprintf(stderr, "Range is far too low!\n");
		b->error = true;
	} else if (b->range < 500.0f) {
		fprintf(stderr, "Range is on the low side.\n");
		b->warning = true;
	}
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
	unsigned int sch;
	float sgf;

	b->roll_pen = powf(b->wing.ar, 0.8f) * 0.7f;
	b->turn_pen = sqrt(max(b->wing.wl - b->manf->tpl, 0.0f));
	b->manu_pen = b->roll_pen + b->turn_pen;
	b->evade_factor = (max(30.0f - b->ceiling, 3.0f) / 10.0f) *
			  sqrt(max(350.0f - b->cruise_spd, 30.0f) / 1.7f) *
			  (1.0f - 0.3f / max(b->manu_pen - 4.5f, 0.5f));
	b->vuln = (b->engines.vuln + b->fuse.vuln) *
		  max(4.0f - b->crew.dc, 1.0f) +
		  b->tanks.vuln;
	b->flak_factor = b->vuln * 3.0f *
			 sqrt(max(30.0f - b->ceiling, 3.0f));
	sgf = (b->turrets.need_gunners + 1.0f) / (b->crew.gunners + 1.0f);
	for (sch = 0; sch < 2; sch++) {
		b->fight_factor[sch] = powf(b->evade_factor, 0.7f) *
				       (b->vuln * 4.0f +
					b->turrets.rate[sch] * sgf) /
				       3.0f;
		b->defn[sch] = b->fight_factor[sch] + b->flak_factor;
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

	/* clear old errors and warnings */
	b->error = false;
	b->warning = false;

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
