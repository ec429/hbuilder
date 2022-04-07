#include <stdio.h>
#include <string.h>

#include "data.h"

void error(const char *msg, int rc)
{
	fprintf(stderr, "%s: %s\n", msg, strerror(-rc));
}

int main(void)
{
	struct list_head guns, engines, manfs, techs;
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

	fprintf(stderr, "Cleaning up...\n");
	free_techs(&techs);
	free_guns(&guns);
	free_engines(&engines);
	free_manfs(&manfs);
	return 0;
}
