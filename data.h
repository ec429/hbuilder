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
};

int load_engines(struct list_head *head);
int free_engines(struct list_head *head);

struct manf {
	struct list_head list;
	char ident[3];
	unsigned int wap, wld, btc, bbb, wcf, wcp, acf, act, geo;
	unsigned int tpl, fdn, ftn, fts, svp, fdf, bof;
	char *eman; // engine manufacturer
	char *name;
};

int load_manfs(struct list_head *head);
int free_manfs(struct list_head *head);

enum fuse_type {
	FT_NORMAL,
	FT_SLENDER,
	FT_SLABBY,
	FT_GEODETIC,

	FT_COUNT
};

enum bb_girth {
	BB_SMALL,
	BB_MEDIUM,
	BB_COOKIE,

	BB_COUNT
};

struct tech {
	struct list_head list;
	char ident[4];
	unsigned int year, inter;
	unsigned int g4t, g4c; // General Arrangement
	unsigned int cmi, ces, ccc; // Crew Stations
	unsigned int clt; // CLimb Time
	unsigned int ft[FT_COUNT]; // Fuse Tare * 10
	unsigned int fd[FT_COUNT]; // Fuse Drag * 10
	unsigned int fwt; // Fuse Wing Tare * 100
	unsigned int cc[FT_COUNT]; // Core Cost * 10
	unsigned int fc[FT_COUNT]; // Fuse Cost * 10
	unsigned int wts, wtc, wtf, wld, wcf; // Wings
	unsigned int fut, fuv, sft, sfv, sfc, fuc; // Fuel Tanks
	unsigned int etf, edf, ees, eet, eec, emc; // Engine Mounts
	unsigned int gtf, gdf, gcf, gac, gam; // Gun Turrets
	unsigned int bt[BB_COUNT], bmc, bbb, bbf;
	unsigned int esl;
	/* TODO numbers */
	struct tech *req[8];
	struct engine *eng[16];
	struct turret *gun[16];
	char *name;
};

int load_techs(struct list_head *head, struct list_head *engines,
	       struct list_head *guns);
int free_techs(struct list_head *head);
