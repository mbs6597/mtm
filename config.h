#ifndef _MTM_CONFIG_H
#define _MTM_CONFIG_H

#include "mtm.h"

struct mtm_region_type joymouse_type;
struct mtm_region_type mousebutton_type;

static struct mtm_region_type *region_types[] = {
	&joymouse_type,
	&mousebutton_type,
};

int button1 = 1;
int button2 = 2;
int button3 = 3;

int mouse = MTM_JOYAXIS_MOUSE;
int wheel = MTM_JOYAXIS_WHEEL;

static struct mtm_region_config default_config[] = {
	{
		.region_type = "joymouse",
		.region_options = &mouse,
		.minx = 0,
		.miny = 0,
		.maxx = 500,
		.maxy = 700
	},
	{
		.region_type = "joymouse",
		.region_options = &wheel,
		.minx = 500,
		.miny = 0,
		.maxx = 800,
		.maxy = 700
	},
	{
		.region_type = "mousebutton",
		.region_options = &button1,
		.minx = 0,
		.miny = 700,
		.maxx = 330,
		.maxy = 1000,
	},
	{
		.region_type = "mousebutton",
		.region_options = &button3,
		.minx = 330,
		.miny = 700,
		.maxx = 660,
		.maxy = 1000,
	},
	{
		.region_type = "mousebutton",
		.region_options = &button2,
		.minx = 660,
		.miny = 700,
		.maxx = 1000,
		.maxy = 1000,
	}
};

#endif
