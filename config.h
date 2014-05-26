#ifndef _MTM_CONFIG_H
#define _MTM_CONFIG_H


struct mtm_region_type joymouse_type;

static struct mtm_region_type *region_types[] = {
	&joymouse_type,
	NULL
};

static struct mtm_region_config default_config[] = {
	{
		.region_type = "joymouse",
		.region_options = NULL,
		.minx = 0,
		.miny = 0,
		.maxx = 500,
		.maxy = 600
	},
};

#endif
