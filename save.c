#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "save.h"
#include "edit.h"
#include "parse.h"

int save_design(FILE *f, const struct bomber *b)
{
	unsigned int i;

	fprintf(f, "MAN=%s\n", b->manf->ident);
	fprintf(f, "ENG=%u:TYP=%s:MOU=%s:EGG=%d\n", b->engines.number,
		b->engines.typ->ident, b->engines.mou->ident,
		b->engines.egg ? 1 : 0);
	for (i = LXN_NOSE; i < LXN_COUNT; i++)
		fprintf(f, "TUR=%u:TYP=%s:MOU=%s\n", i,
			b->turrets.typ[i] ? b->turrets.typ[i]->ident : "null",
			b->turrets.mou[i] ? b->turrets.mou[i]->ident : "null");
	fprintf(f, "WIN=%u:ART=%u\n", b->wing.area, b->wing.art);
	fprintf(f, "CRW=");
	for (i = 0; i < b->crew.n; i++) {
		const struct crewman *m = b->crew.men + i;

		fputc(crew_to_letter(m->pos), f);
		if (m->gun)
			fputc('*', f);
	}
	fputc('\n', f);
	fprintf(f, "BOM=%u:CAP=%u:GIR=%d:CSB=%d\n", b->bay.load,
		b->bay.cap, (int)b->bay.girth,
		b->bay.csbs ? 1 : 0);
	fprintf(f, "FUS=%d\n", (int)b->fuse.typ);
	fprintf(f, "ESL=%d:NAV=", (int)b->elec.esl);
	for (i = 0; i < NA_COUNT; i++)
		fputc(b->elec.navaid[i] ? '1' : '0', f);
	fputc('\n', f);
	fprintf(f, "TAN=%u:PCT=%u:SST=%d\n", b->tanks.hlb, b->tanks.pct,
		b->tanks.sst ? 1 : 0);
	fprintf(f, "MTW=%u\n", b->mtow);
	fprintf(f, "RFL=%u\n", b->refit);
	fprintf(f, "EOD\n");
	return 0;
}

struct loaddata {
	struct bomber *b;
	const struct entities *ent;
	unsigned int ei, ti;
};

static void load_error(struct loaddata *l, const char *format, ...)
{
	va_list ap;

	/* only one error can occur during load */
	l->b->new = 1;
	va_start(ap, format);
	vsnprintf(l->b->ew[0], EW_LEN, format, ap);
	va_end(ap);
	l->b->error = true;
}

#define LOADER_INT(_name, _field)					\
static int load_##_name(const char *value, struct loaddata *l)		\
{									\
	if (sscanf(value, "%u", &l->b->_field) != 1)			\
		return -EINVAL;						\
	return 0;							\
}

#define LOADER_BOOL(_name, _field)					\
static int load_##_name(const char *value, struct loaddata *l)		\
{									\
	unsigned int v;							\
									\
	if (sscanf(value, "%u", &v) != 1)				\
		return -EINVAL;						\
	l->b->_field = !!v;						\
	return 0;							\
}

static int load_man(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < l->ent->nmanf; i++)
		if (!strcmp(value, l->ent->manf[i]->ident)) {
			l->b->manf = l->ent->manf[i];
			return 0;
		}
	load_error(l, "Manufacturer %s not found!", value);
	return -ENOENT;
}

static int load_eng(const char *value, struct loaddata *l)
{
	if (sscanf(value, "%u", &l->b->engines.number) != 1)
		return -EINVAL;
	l->ei = 1;
	return 0;
}

static int load_ety(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < l->ent->neng; i++)
		if (!strcmp(value, l->ent->eng[i]->ident)) {
			l->b->engines.typ = l->ent->eng[i];
			return 0;
		}
	load_error(l, "Engine %s not found!", value);
	return -ENOENT;
}

static int load_emo(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < l->ent->neng; i++)
		if (!strcmp(value, l->ent->eng[i]->ident)) {
			l->b->engines.mou = l->ent->eng[i];
			return 0;
		}
	load_error(l, "Engine %s not found!", value);
	return -ENOENT;
}

static int load_tty(const char *value, struct loaddata *l)
{
	unsigned int i;

	if (!strcmp(value, "null")) {
		l->b->turrets.typ[l->ti] = NULL;
		return 0;
	}
	for (i = 0; i < l->ent->ngun; i++)
		if (!strcmp(value, l->ent->gun[i]->ident)) {
			l->b->turrets.typ[l->ti] = l->ent->gun[i];
			return 0;
		}
	load_error(l, "Turret %s not found!", value);
	return -ENOENT;
}

static int load_tmo(const char *value, struct loaddata *l)
{
	unsigned int i;

	if (!strcmp(value, "null")) {
		l->b->turrets.mou[l->ti] = NULL;
		return 0;
	}
	for (i = 0; i < l->ent->ngun; i++)
		if (!strcmp(value, l->ent->gun[i]->ident)) {
			l->b->turrets.mou[l->ti] = l->ent->gun[i];
			return 0;
		}
	load_error(l, "Turret %s not found!", value);
	return -ENOENT;
}

static int load_typ(const char *value, struct loaddata *l)
{
	if (l->ei)
		return load_ety(value, l);
	if (l->ti)
		return load_tty(value, l);
	load_error(l, "TYP (%s) not in ENG or TUR line!", value);
	return -EINVAL;
}

static int load_mou(const char *value, struct loaddata *l)
{
	if (l->ei)
		return load_emo(value, l);
	if (l->ti)
		return load_tmo(value, l);
	load_error(l, "MOU (%s) not in ENG or TUR line!", value);
	return -EINVAL;
}

static int load_egg(const char *value, struct loaddata *l)
{
	unsigned int e;

	if (!l->ei) {
		load_error(l, "EGG (%s) not in ENG line!", value);
		return -EINVAL;
	}
	if (sscanf(value, "%u", &e) != 1)
		return -EINVAL;
	l->b->engines.egg = !!e;
	return 0;
}

static int load_tur(const char *value, struct loaddata *l)
{
	if (sscanf(value, "%u", &l->ti) != 1)
		return -EINVAL;
	if (l->ti <= LXN_UNSPEC || l->ti >= LXN_COUNT)
		return -EINVAL;
	return 0;
}

LOADER_INT(win, wing.area);
LOADER_INT(art, wing.art);

static int load_crw(const char *value, struct loaddata *l)
{
	struct crew *c = &l->b->crew;
	enum crewpos p;
	char v;

	while ((v = *value++)) {
		if (v == '*') {
			if (!c->n)
				return -EINVAL;
			c->men[c->n - 1].gun = true;
			continue;
		}
		p = letter_to_crew(v);
		if (p >= CREW_CLASSES)
			return -EINVAL;
		if (c->n >= MAX_CREW)
			return -ENOSPC;
		c->men[c->n++].pos = p;
	}
	return 0;
}

LOADER_INT(bom, bay.load);
LOADER_INT(cap, bay.cap);
LOADER_INT(gir, bay.girth);
LOADER_BOOL(csb, bay.csbs);
LOADER_INT(fus, fuse.typ);
LOADER_INT(esl, elec.esl);

static int load_nav(const char *value, struct loaddata *l)
{
	unsigned int i;

	for (i = 0; i < NA_COUNT; i++) {
		switch (value[i]) {
		case '0':
			break;
		case '1':
			l->b->elec.navaid[i] = true;
			break;
		default:
			return -EINVAL;
		}
	}
	if (value[i])
		return -EINVAL;
	return 0;
}

LOADER_INT(tan, tanks.hlb);
LOADER_INT(pct, tanks.pct);
LOADER_BOOL(sst, tanks.sst);
LOADER_INT(mtw, mtow);
LOADER_INT(rfl, refit);

static int load_eod(const char *value, struct loaddata *l)
{
	if (value)
		return -EINVAL;
	return 1;
}

struct loadkey {
	const char *key;
	int (*fn)(const char *value, struct loaddata *l);
} loaders[] = {
	{"MAN", load_man},
	{"ENG", load_eng},
	{"TYP", load_typ},
	{"MOU", load_mou},
	{"EGG", load_egg},
	{"TUR", load_tur},
	{"WIN", load_win},
	{"ART", load_art},
	{"CRW", load_crw},
	{"BOM", load_bom},
	{"CAP", load_cap},
	{"GIR", load_gir},
	{"CSB", load_csb},
	{"FUS", load_fus},
	{"ESL", load_esl},
	{"NAV", load_nav},
	{"TAN", load_tan},
	{"PCT", load_pct},
	{"SST", load_sst},
	{"MTW", load_mtw},
	{"RFL", load_rfl},
	{"EOD", load_eod},
};

static int load_design_word(const char *key, const char *value, void *data)
{
	struct loaddata *l = data;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(loaders); i++)
		if (!strcmp(key, loaders[i].key))
			return loaders[i].fn(value, l);
	load_error(l, "Key %s not found!", key);
	return -ENOENT;
}

static int load_design_line(const char *line, void *data)
{
	struct loaddata *l = data;

	l->ei = l->ti = 0;
	return for_each_word(line, load_design_word, l);
}

int load_design(FILE *f, struct bomber *b, const struct entities *ent)
{
	struct loaddata l = {b, ent};
	int fd = fileno(f), rc;

	memset(b, 0, sizeof(*b));
	b->manf = ent->manf[0];
	b->engines.typ = b->engines.mou = ent->eng[0];
	b->parent = b;

	if (fd < 0) {
		load_error(&l, "Invalid stream!");
		return -EBADF;
	}
	rc = for_each_line(fd, load_design_line, &l);
	return rc > 0 ? 0 : rc;
}
