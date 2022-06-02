#include "save.h"
#include "edit.h"

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
