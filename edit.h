#ifndef _EDIT_H
#define _EDIT_H

#include "calc.h"

const char *describe_bbg(enum bb_girth girth);
const char *describe_esl(enum elec_level esl);
const char *describe_navaid(enum nav_aid na);

int editor(struct bomber *b, struct tech_numbers *tn,
	   const struct entities *ent);

#endif // _EDIT_H
