#include <stdio.h>
#include <string.h>

#include "data.h"
#include "calc.h"
#include "edit.h"

void error(const char *msg, int rc)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(-rc));
}

void empty_guns(struct list_head *guns)
{
	struct turret *gun;

	list_for_each_entry(gun, guns)
		gun->unlocked = false;
}

void empty_engines(struct list_head *engines)
{
	struct engine *eng;

	list_for_each_entry(eng, engines)
		eng->unlocked = false;
}

void empty_techs(struct list_head *techs)
{
	struct tech *tech;

	list_for_each_entry(tech, techs)
		tech->unlocked = !tech->year && !tech->inter;
}

int main(void)
{
	struct list_head guns, engines, manfs, techs;
	struct entities entities;
	struct tech_numbers tn;
	struct bomber b;
	int rc;

	INIT_LIST_HEAD(&guns);
	INIT_LIST_HEAD(&engines);
	INIT_LIST_HEAD(&manfs);
	INIT_LIST_HEAD(&techs);

	rc = load_guns(&guns);
	if (rc < 0) {
		error("Failed to load guns", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d guns\n", rc);

	rc = load_engines(&engines);
	if (rc < 0) {
		error("Failed to load engines", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d engines\n", rc);

	rc = load_manfs(&manfs);
	if (rc < 0) {
		error("Failed to load manfs", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d manfs\n", rc);

	rc = load_techs(&techs, &engines, &guns);
	if (rc < 0) {
		error("Failed to load techs", rc);
		return 1;
	}
	fprintf(stderr, "Loaded %d techs\n", rc);

	empty_guns(&guns);
	empty_engines(&engines);
	empty_techs(&techs);
	rc = apply_techs(&techs, &tn);
	if (rc < 0) {
		error("Failed to init techs", rc);
		return 1;
	}
	fprintf(stderr, "Initialised tech state\n");

	init_bomber(&b, list_first_entry(&manfs, struct manf),
		    list_first_entry(&engines, struct engine));
	rc = calc_bomber(&b, &tn);
	if (rc < 0) {
		error("Failed to update calcs", rc);
		return 1;
	}
	fprintf(stderr, "Prepared blank bomber\n");

	rc = populate_entities(&entities, &guns, &engines, &manfs, &techs);
	if (rc < 0) {
		error("Failed to create entity arrays", rc);
		return 1;
	}
	fprintf(stderr, "Initialised editor\n");

	edit_loop(&b, &tn, &entities);

	fprintf(stderr, "Cleaning up...\n");
	free_techs(&techs);
	free_guns(&guns);
	free_engines(&engines);
	free_manfs(&manfs);
	return 0;
}
