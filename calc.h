#include <stdbool.h>
#include "data.h"

#ifndef _CALC_H
#define _CALC_H

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

struct crew {
	/* Inputs */
	unsigned int n;
	struct crewman men[16];
	/* Output cache */
	unsigned int gunners;
	float tare;
	float gross;
	float dc;
	float cct; /* add to core_tare for core_cost */
	float es;
};

struct bombbay {
	/* Inputs */
	unsigned int cap;
	enum bb_girth girth;
	/* Output cache */
	float factor;
	float bigfactor;
	float tare;
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
	bool error, warning;
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
	float deck_spd;
	float range;
	float ferry;
	float roll_pen, turn_pen, manu_pen;
	float evade_factor;
	float vuln;
	float fight_factor[2], flak_factor;
	float defn[2];
	float tproto;
	float tprod;
	float cproto;
	float cprod;
};

void init_bomber(struct bomber *b, struct manf *m, struct engine *e);
int calc_bomber(struct bomber *b, struct tech_numbers *tn);

const char *describe_ftype(enum fuse_type ft);
const char *describe_bbg(enum bb_girth girth);
const char *describe_esl(enum elec_level esl);
char crew_to_letter(enum crewpos c);
enum crewpos letter_to_crew(char c);

void dump_bomber_info(struct bomber *b);
void dump_bomber_calcs(struct bomber *b);

#endif // _CALC_H