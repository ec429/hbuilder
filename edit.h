#ifndef _EDIT_H
#define _EDIT_H

#include "calc.h"

const char *describe_bbg(enum bb_girth girth);

int edit_loop(struct bomber *b, struct tech_numbers *tn,
	      const struct entities *ent);

#endif // _EDIT_H
