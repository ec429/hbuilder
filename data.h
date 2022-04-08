#ifndef _DATA_H
#define _DATA_H

#include <stdbool.h>
#include "list.h"

enum turret_location {
	LXN_UNSPEC,
	LXN_NOSE,
	LXN_DORSAL,
	LXN_TAIL,
	LXN_WAIST,
	LXN_VENTRAL,
	LXN_CHIN,

	LXN_COUNT
};

enum coverage_direction {
	GC_FRONT, // GCF
	GC_BEAM_HIGH, // GCD
	GC_BEAM_LOW, // GCV
	GC_TAIL_HIGH, // GCH
	GC_TAIL_LOW, // GCL
	GC_BENEATH, // GCB

	GC_COUNT
};

struct turret {
	struct list_head list;
	char ident[5];
	unsigned int srv;
	unsigned int twt;
	unsigned int drg;
	enum turret_location lxn;
	unsigned int gun;
	unsigned int gc[GC_COUNT];
	unsigned int esl;
	char *name;
	bool unlocked;
};

int load_guns(struct list_head *head);
int free_guns(struct list_head *head);

struct engine {
	struct list_head list;
	char ident[5];
	unsigned int bhp;
	unsigned int vul;
	unsigned int fai;
	unsigned int svc;
	unsigned int cos;
	unsigned int scl;
	unsigned int twt;
	unsigned int drg;
	char *manu;
	char *name;
	bool unlocked;
};

int load_engines(struct list_head *head);
int free_engines(struct list_head *head);

enum bb_girth {
	BB_SMALL,
	BB_MEDIUM,
	BB_COOKIE,

	BB_COUNT
};

enum fuse_type {
	FT_NORMAL,
	FT_SLENDER,
	FT_SLABBY,
	FT_GEODETIC,

	FT_COUNT
};

struct manf {
	struct list_head list;
	char ident[3];
	unsigned int wap, wld, bt[BB_COUNT], bbb, wcf, wcp, wc4, acc, act, geo;
	unsigned int tpl, fd[FT_COUNT], ft[FT_COUNT], svp, bof;
	char *eman; // engine manufacturer
	char *name;
};

int load_manfs(struct list_head *head);
int free_manfs(struct list_head *head);

struct tech_numbers {
	/* These MUST all be `unsigned int`!  Copying code assumes this. */
	unsigned int g4t, g4c; // General Arrangement
	unsigned int cmi, ces, ccc; // Crew Stations
	unsigned int clt; // CLimb Time
	unsigned int ft[FT_COUNT]; // Fuse Tare * 100
	unsigned int fd[FT_COUNT]; // Fuse Drag * 10
	unsigned int fs[FT_COUNT]; // Fuse Serv * 10
	unsigned int ff[FT_COUNT]; // Fuse Fail * 10
	unsigned int fv[FT_COUNT]; // Fuse Vuln * 100
	unsigned int fwt; // Fuse Wing Tare * 100
	unsigned int cc[FT_COUNT]; // Core Cost * 100
	unsigned int fc[FT_COUNT]; // Fuse Cost * 100
	unsigned int wts, wtc, wtf, wld, wcf; // Wings
	unsigned int fut, fuv, fgv, sft, sfv, sfc, fuc; // Fuel Tanks
	unsigned int etf, edf, ees, eet, eec, emc; // Engine Mounts
	unsigned int gtf, gdf, gcf, gac, gam; // Gun Turrets
	unsigned int bt[BB_COUNT], bmc, bbb, bbf;
	unsigned int esl;
};

struct tech {
	struct list_head list;
	char ident[4];
	unsigned int year;
	struct tech_numbers num;
	struct tech *req[8];
	struct engine *eng[16];
	struct turret *gun[16];
	char *name;
	bool unlocked, have_reqs;
};

int load_techs(struct list_head *head, struct list_head *engines,
	       struct list_head *guns);
int free_techs(struct list_head *head);

struct entities {
	unsigned int ngun, neng, nmanf, ntech;
	struct turret **gun;
	struct engine **eng;
	struct manf **manf;
	struct tech **tech;
};

int populate_entities(struct entities *ent, struct list_head *guns,
		      struct list_head *engines, struct list_head *manfs,
		      struct list_head *techs);

int apply_techs(const struct entities *ent, struct tech_numbers *tn);

#endif // _DATA_H
