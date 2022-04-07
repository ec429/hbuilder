#include "calc.h"

#ifndef _EDIT_H
#define _EDIT_H

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

int edit_loop(struct bomber *b, struct tech_numbers *tn,
	      const struct entities *ent);

#endif // _EDIT_H
