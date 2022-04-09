#ifndef _CALC_H
#define _CALC_H

#include <stdbool.h>
#include "data.h"

struct engines {
	/* Inputs */
	unsigned int number;
	struct engine *typ;
	bool egg;
	/* Output cache */
	bool odd, manumatch;
	float power_factor;
	float vuln;
	float rely1, rely2;
	float serv;
	float cost;
	unsigned int scl;
	float fuelrate;
	float tare;
	float drag;
};

struct turrets {
	/* Inputs */
	struct turret *typ[LXN_COUNT];
	/* Output cache */
	unsigned int need_gunners;
	bool gas[LXN_COUNT];
	float drag;
	float tare;
	float ammo;
	float serv;
	float cost;
	float gc[GC_COUNT];
	float rate[2];
};

struct wing {
	/* Inputs */
	unsigned int area;
	unsigned int art; // AR in tenths
	/* Output cache */
	float ar;
	float span;
	float chord;
	float cl, ld;
	float tare;
	float cost;
	float wl;
	float drag;
};

enum crewpos {
	CCLASS_P,
	CCLASS_N,
	CCLASS_B,
	CCLASS_W,
	CCLASS_E,
	CCLASS_G,

	CREW_CLASSES
};

struct crewman {
	enum crewpos pos;
	bool gun;
};

#define MAX_CREW	16

struct crew {
	/* Inputs */
	unsigned int n;
	struct crewman men[MAX_CREW];
	/* Output cache */
	unsigned int gunners, engineers;
	bool pilot, nav;
	float tare;
	float gross;
	float dc;
	float bn;
	float cct; /* add to core_tare for core_cost */
	float es;
};

struct bombbay {
	/* Inputs */
	unsigned int cap;
	enum bb_girth girth;
	bool csbs; /* not really bomb "bay" but whatever */
	/* Output cache */
	float factor;
	float bigfactor;
	float tare;
	float cost; /* csbs cost, rest is paid through core_tare */
	bool cookie;
};

struct fuselage {
	/* Inputs */
	enum fuse_type typ;
	/* Output cache */
	float serv, fail;
	float tare;
	float cost;
	float vuln;
	float drag;
};

enum elec_level {
	ESL_LOW,
	ESL_HIGH,
	ESL_STABLE,

	ESL_COUNT
};

struct electrics {
	/* Inputs */
	enum elec_level esl;
	/* Output cache */
	float cost;
};

struct tanks {
	/* Inputs */
	unsigned int ht; // hours in tenths
	bool sst;
	/* Output cache */
	float hours;
	float mass;
	float tare;
	float cost;
	float ratio;
	float vuln;
};

#define MAX_EW	16
#define EW_LEN	80
struct bomber {
	/* Inputs */
	struct manf *manf;
	struct engines engines;
	struct turrets turrets;
	struct wing wing;
	struct crew crew;
	struct bombbay bay;
	struct fuselage fuse;
	struct electrics elec;
	struct tanks tanks;
	/* Output cache */
	bool error;
	unsigned int new;
	char ew[MAX_EW][EW_LEN];
	float serv;
	float fail;
	float core_tare;
	float tare;
	float gross;
	float drag;
	float core_cost;
	float cost;
	float takeoff_spd;
	float ceiling; /* thousands ft */
	float cruise_alt, cruise_spd;
	float init_climb, deck_spd;
	float range;
	float ferry;
	float roll_pen, turn_pen, manu_pen;
	float evade_factor;
	float vuln;
	float fight_factor[2], flak_factor;
	float defn[2];
	float accu;
	float tproto;
	float tprod;
	float cproto;
	float cprod;
};

void init_bomber(struct bomber *b, struct manf *m, struct engine *e);
int calc_bomber(struct bomber *b, struct tech_numbers *tn);

#endif // _CALC_H
