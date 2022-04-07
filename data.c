#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "data.h"

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

static int for_each_line(int fd, int (*cb)(const char *line, void *data),
			 void *data)
{
	size_t len, from = 0, to;
	int count = 0, rc;
	char line[256];
	ssize_t bytes;

	do {
		bytes = read(fd, line + from, sizeof(line) - from - 1);
		if (bytes < 0) {
			rc = -errno;
			goto out;
		}
		if (!bytes) {
			rc = from ? -EIO : count;
			goto out;
		}
		line[from + bytes] = 0;
		to = 0;
		do {
			for (len = to; line[len]; len++)
				if (line[len] == '\n')
					break;
			if (!line[len]) {
				if (to)
					break;
				rc = -EIO;
				goto out;
			}
			line[len] = 0;
			rc = cb(line + to, data);
			if (rc < 0)
				goto out;
			count++;
			to = len + 1;
		} while (1);
		memmove(line, line + to, from + bytes + 1 - to);
		from = from + bytes - to;
	} while (1);
out:
	return rc;
}

static int for_each_word(const char *line,
			 int (*cb)(const char *key, const char *value, void *data),
			 void *data)
{
	char word[256];
	char *equals;
	size_t len;
	bool end;
	int rc;

	do {
		len = strcspn(line, ":");
		if (len >= sizeof(word)) // can't happen
			return -EIO;
		memcpy(word, line, len);
		end = !line[len];
		word[len] = 0;
		equals = strchr(word, '=');
		if (equals)
			*equals++ = 0;
		rc = cb(word, equals, data);
		if (rc)
			return rc;
		line += len + 1;
	} while(!end);
	return 0;
}

#define INT_KEY(obj, kn, vn)						\
	if (!strcmp(key, kn)) {						\
		if (sscanf(value, "%u", &obj->vn) != 1)			\
			return -EINVAL;					\
		return 0;						\
	}

static int load_gun_word(const char *key, const char *value, void *data)
{
	struct turret *gun = data;

	INT_KEY(gun, "SRV", srv);
	INT_KEY(gun, "TWT", twt);
	INT_KEY(gun, "DRG", drg);
	INT_KEY(gun, "LXN", lxn);
	INT_KEY(gun, "GUN", gun);
	INT_KEY(gun, "GCF", gc[GC_FRONT]);
	INT_KEY(gun, "GCD", gc[GC_BEAM_HIGH]);
	INT_KEY(gun, "GCV", gc[GC_BEAM_LOW]);
	INT_KEY(gun, "GCH", gc[GC_TAIL_HIGH]);
	INT_KEY(gun, "GCL", gc[GC_TAIL_LOW]);
	INT_KEY(gun, "GCB", gc[GC_BENEATH]);
	INT_KEY(gun, "ESL", esl);
	if (!strcmp(key, "n")) {
		gun->name = strdup(value);
		if (!gun->name)
			return -ENOMEM;
		return 0;
	}

	fprintf(stderr, "load_gun_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

static int load_gun(const char *line, void *data)
{
	struct turret *gun = calloc(1, sizeof(*gun));
	struct list_head *head = data;
	int rc;

	if (strcspn(line, ":") != 4) {
		fprintf(stderr, "load_gun: ident is not 4 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	memcpy(gun->ident, line, 4);
	gun->ident[4] = 0;
	rc = for_each_word(line + 5, load_gun_word, gun);
out:
	if (rc) {
		fprintf(stderr, "load_gun: failed to load %s\n", gun->ident);
		free(gun->name);
		free(gun);
	} else {
		list_add_tail(head, &gun->list);
	}
	return rc;
}

int load_guns(struct list_head *head)
{
	int fd = open("guns", O_RDONLY), rc;

	if (fd < 0)
		return -errno;
	rc = for_each_line(fd, load_gun, head);
	close(fd);
	return rc;
}

int free_guns(struct list_head *head)
{
	struct turret *gun;

	while (!list_empty(head)) {
		gun = list_first_entry(head, struct turret);
		free(gun->name);
		list_del(&gun->list);
		free(gun);
	}
	return 0;
}

static int load_engine_word(const char *key, const char *value, void *data)
{
	struct engine *eng = data;

	INT_KEY(eng, "BHP", bhp);
	INT_KEY(eng, "VUL", vul);
	INT_KEY(eng, "FAI", fai);
	INT_KEY(eng, "SVC", svc);
	INT_KEY(eng, "COS", cos);
	INT_KEY(eng, "SCL", scl);
	INT_KEY(eng, "TWT", twt);
	INT_KEY(eng, "DRG", drg);
	if (!strcmp(key, "m")) {
		eng->manu = strdup(value);
		if (!eng->manu)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "n")) {
		eng->name = strdup(value);
		if (!eng->name)
			return -ENOMEM;
		return 0;
	}

	fprintf(stderr, "load_engine_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

static int load_engine(const char *line, void *data)
{
	struct engine *eng = calloc(1, sizeof(*eng));
	struct list_head *head = data;
	int rc;

	if (strcspn(line, ":") != 4) {
		fprintf(stderr, "load_engine: ident is not 4 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	memcpy(eng->ident, line, 4);
	eng->ident[4] = 0;
	rc = for_each_word(line + 5, load_engine_word, eng);
out:
	if (rc) {
		fprintf(stderr, "load_engine: failed to load %s\n", eng->ident);
		free(eng->manu);
		free(eng->name);
		free(eng);
	} else {
		list_add_tail(head, &eng->list);
	}
	return rc;
}

int load_engines(struct list_head *head)
{
	int fd = open("eng", O_RDONLY), rc;

	if (fd < 0)
		return -errno;
	rc = for_each_line(fd, load_engine, head);
	close(fd);
	return rc;
}

int free_engines(struct list_head *head)
{
	struct engine *eng;

	while (!list_empty(head)) {
		eng = list_first_entry(head, struct engine);
		free(eng->manu);
		free(eng->name);
		list_del(&eng->list);
		free(eng);
	}
	return 0;
}

static int load_manf_word(const char *key, const char *value, void *data)
{
	struct manf *man = data;

	INT_KEY(man, "WAP", wap);
	INT_KEY(man, "WLD", wld);
	INT_KEY(man, "BTC", btc);
	INT_KEY(man, "BBB", bbb);
	INT_KEY(man, "WCF", wcf);
	INT_KEY(man, "WCP", wcp);
	INT_KEY(man, "ACF", acf);
	INT_KEY(man, "ACT", act);
	INT_KEY(man, "GEO", geo);
	INT_KEY(man, "TPL", tpl);
	INT_KEY(man, "FDN", fdn);
	INT_KEY(man, "FTN", ftn);
	INT_KEY(man, "FTS", fts);
	INT_KEY(man, "SVP", svp);
	INT_KEY(man, "FDF", fdf);
	INT_KEY(man, "BOF", bof);
	if (!strcmp(key, "e")) {
		man->eman = strdup(value);
		if (!man->eman)
			return -ENOMEM;
		return 0;
	}
	if (!strcmp(key, "n")) {
		man->name = strdup(value);
		if (!man->name)
			return -ENOMEM;
		return 0;
	}

	fprintf(stderr, "load_manf_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

struct manf_loader {
	struct list_head *head;
	struct manf *starman;
};

static int load_manf(const char *line, void *data)
{
	struct manf *man = calloc(1, sizeof(*man));
	struct manf_loader *loader = data;
	bool star;
	int rc;

	if (strcspn(line, ":") != 2) {
		fprintf(stderr, "load_manf: ident is not 2 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	star = line[0] == '*' && line[1] == '*';
	if (!star) {
		if (!loader->starman) {
			fprintf(stderr, "load_manf: entry ** must be first\n");
			rc = -EINVAL;
			goto out;
		}
		// copy all the stars
		*man = *loader->starman;
		// except pointery things
		INIT_LIST_HEAD(&man->list);
		man->eman = NULL;
		man->name = NULL;
	}
	memcpy(man->ident, line, 2);
	man->ident[2] = 0;
	rc = for_each_word(line + 3, load_manf_word, man);
out:
	if (rc) {
		fprintf(stderr, "load_manf: failed to load %s\n", man->ident);
		free(man->eman);
		free(man->name);
		free(man);
	} else if (star) {
		loader->starman = man;
	} else {
		list_add_tail(loader->head, &man->list);
	}
	return rc;
}

int load_manfs(struct list_head *head)
{
	int fd = open("manu", O_RDONLY), rc;
	struct manf_loader loader;

	if (fd < 0)
		return -errno;
	loader.head = head;
	loader.starman = NULL;
	rc = for_each_line(fd, load_manf, &loader);
	if (loader.starman) {
		free(loader.starman->eman);
		free(loader.starman->name);
	}
	free(loader.starman);
	close(fd);
	if (rc > 0)
		rc--; /* starman doesn't count */
	return rc;
}

int free_manfs(struct list_head *head)
{
	struct manf *man;

	while (!list_empty(head)) {
		man = list_first_entry(head, struct manf);
		free(man->eman);
		free(man->name);
		list_del(&man->list);
		free(man);
	}
	return 0;
}

struct tech_loader {
	struct list_head *head;
	struct tech *tech;
	struct list_head *engines;
	struct list_head *guns;
};

static int load_tech_word(const char *key, const char *value, void *data)
{
	struct tech_loader *loader = data;

	INT_KEY(loader->tech, "y", year);
	INT_KEY(loader->tech, "i", inter);
	INT_KEY(loader->tech, "G4T", g4t);
	INT_KEY(loader->tech, "G4C", g4c);
	INT_KEY(loader->tech, "CMI", cmi);
	INT_KEY(loader->tech, "CES", ces);
	INT_KEY(loader->tech, "CCC", ccc);
	INT_KEY(loader->tech, "CLT", clt);
	INT_KEY(loader->tech, "FTN", ft[FT_NORMAL]);
	INT_KEY(loader->tech, "FTT", ft[FT_SLENDER]);
	INT_KEY(loader->tech, "FTS", ft[FT_SLABBY]);
	INT_KEY(loader->tech, "FTG", ft[FT_GEODETIC]);
	INT_KEY(loader->tech, "FDN", fd[FT_NORMAL]);
	INT_KEY(loader->tech, "FDT", fd[FT_SLENDER]);
	INT_KEY(loader->tech, "FDS", fd[FT_SLABBY]);
	INT_KEY(loader->tech, "FDG", fd[FT_GEODETIC]);
	INT_KEY(loader->tech, "FWT", fwt);
	INT_KEY(loader->tech, "CCN", cc[FT_NORMAL]);
	INT_KEY(loader->tech, "CCT", cc[FT_SLENDER]);
	INT_KEY(loader->tech, "CCS", cc[FT_SLABBY]);
	INT_KEY(loader->tech, "CCG", cc[FT_GEODETIC]);
	INT_KEY(loader->tech, "FCN", fc[FT_NORMAL]);
	INT_KEY(loader->tech, "FCT", fc[FT_SLENDER]);
	INT_KEY(loader->tech, "FCS", fc[FT_SLABBY]);
	INT_KEY(loader->tech, "FCG", fc[FT_GEODETIC]);
	INT_KEY(loader->tech, "WTS", wts);
	INT_KEY(loader->tech, "WTC", wtc);
	INT_KEY(loader->tech, "WTF", wtf);
	INT_KEY(loader->tech, "WLD", wld);
	INT_KEY(loader->tech, "WCF", wcf);
	INT_KEY(loader->tech, "FUT", fut);
	INT_KEY(loader->tech, "FUV", fuv);
	INT_KEY(loader->tech, "SFT", sft);
	INT_KEY(loader->tech, "SFV", sfv);
	INT_KEY(loader->tech, "SFC", sfc);
	INT_KEY(loader->tech, "FUC", fuc);
	INT_KEY(loader->tech, "ETF", etf);
	INT_KEY(loader->tech, "EDF", edf);
	INT_KEY(loader->tech, "EES", ees);
	INT_KEY(loader->tech, "EET", eet);
	INT_KEY(loader->tech, "EEC", eec);
	INT_KEY(loader->tech, "EMC", emc);
	INT_KEY(loader->tech, "GTF", gtf);
	INT_KEY(loader->tech, "GDF", gdf);
	INT_KEY(loader->tech, "GCF", gcf);
	INT_KEY(loader->tech, "GAC", gac);
	INT_KEY(loader->tech, "GAM", gam);
	INT_KEY(loader->tech, "BTS", bt[BB_SMALL]);
	INT_KEY(loader->tech, "BTM", bt[BB_MEDIUM]);
	INT_KEY(loader->tech, "BTC", bt[BB_COOKIE]);
	INT_KEY(loader->tech, "BMC", bmc);
	INT_KEY(loader->tech, "BBB", bbb);
	INT_KEY(loader->tech, "BBF", bbf);
	INT_KEY(loader->tech, "ESL", esl);
	if (!strcmp(key, "e")) {
		struct engine *eng;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(loader->tech->eng); i++)
			if (!loader->tech->eng[i])
				break;
		if (i == ARRAY_SIZE(loader->tech->eng))
			return -ENOBUFS;
		list_for_each_entry(eng, loader->engines) {
			if (!strcmp(value, eng->ident)) {
				loader->tech->eng[i] = eng;
				return 0;
			}
		}
		fprintf(stderr, "load_tech_word: No such engine '%s'\n", value);
		return -ENOENT;
	}
	if (!strcmp(key, "t")) {
		struct turret *gun;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(loader->tech->gun); i++)
			if (!loader->tech->gun[i])
				break;
		if (i == ARRAY_SIZE(loader->tech->gun))
			return -ENOBUFS;
		list_for_each_entry(gun, loader->guns) {
			if (!strcmp(value, gun->ident)) {
				loader->tech->gun[i] = gun;
				return 0;
			}
		}
		fprintf(stderr, "load_tech_word: No such turret '%s'\n", value);
		return -ENOENT;
	}
	if (!strcmp(key, "r")) {
		struct tech *req;
		unsigned int i;

		for (i = 0; i < ARRAY_SIZE(loader->tech->req); i++)
			if (!loader->tech->req[i])
				break;
		if (i == ARRAY_SIZE(loader->tech->req))
			return -ENOBUFS;
		list_for_each_entry(req, loader->head) {
			if (!strcmp(value, req->ident)) {
				loader->tech->req[i] = req;
				return 0;
			}
		}
		fprintf(stderr, "load_tech_word: No such tech '%s'\n", value);
		return -ENOENT;
	}
	if (!strcmp(key, "n")) {
		loader->tech->name = strdup(value);
		if (!loader->tech->name)
			return -ENOMEM;
		return 0;
	}

	fprintf(stderr, "load_tech_word: unrecognised key '%s'\n", key);
	return -EINVAL;
}

static int load_tech(const char *line, void *data)
{
	struct tech *tech = calloc(1, sizeof(*tech));
	struct tech_loader *loader = data;
	bool star;
	int rc;

	if (strcspn(line, ":") != 3) {
		fprintf(stderr, "load_tech: ident is not 3 chars long\n");
		rc = -EINVAL;
		goto out;
	}
	memcpy(tech->ident, line, 3);
	tech->ident[3] = 0;
	loader->tech = tech;
	rc = for_each_word(line + 4, load_tech_word, loader);
out:
	if (rc) {
		fprintf(stderr, "load_tech: failed to load %s\n", tech->ident);
		free(tech->name);
		free(tech);
	} else {
		list_add_tail(loader->head, &tech->list);
	}
	return rc;
}

int load_techs(struct list_head *head, struct list_head *engines,
	       struct list_head *guns)
{
	int fd = open("tech", O_RDONLY), rc;
	struct tech_loader loader;

	loader.head = head;
	loader.engines = engines;
	loader.guns = guns;

	if (fd < 0)
		return -errno;
	rc = for_each_line(fd, load_tech, &loader);
	close(fd);
	return rc;
}

int free_techs(struct list_head *head)
{
	struct tech *tech;

	while (!list_empty(head)) {
		tech = list_first_entry(head, struct tech);
		free(tech->name);
		list_del(&tech->list);
		free(tech);
	}
	return 0;
}
