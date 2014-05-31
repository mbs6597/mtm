#ifndef _MTM_CONFIG_H
#define _MTM_CONFIG_H

#include "mtm.h"
#include "joymouse.h"
#include "mousebutton.h"

struct mtm_region_type joymouse_type;
struct mtm_region_type mousebutton_type;

static struct mtm_region_type *region_types[] = {
	&joymouse_type,
	&mousebutton_type,
};

static int button1 = 1;
static int button2 = 2;
static int button3 = 3;

int mouse_distance[] = {0, 150,  300,  1200,   5000};
int mouse_velocity[] = {0, 100,  800, 10000, 100000};

struct joymouse_config mousecfg = {
	.joyaxis_single = MTM_JOYAXIS_MOUSE,
	.joyaxis_multi = MTM_JOYAXIS_WHEEL,

	.tap_max_ticks = 5,
	.tap_max_distance = 300,
	.tap_button = 1,

	.num_points = 5,
	.distance = mouse_distance,
	.velocity = mouse_velocity,
};

struct mousebutton_config b1_cfg = {
	.mouse_button_qty = 1,
	.mouse_buttons = &button1,
	.keycode_qty = 0,
	.keycodes = NULL,
};

struct mousebutton_config b2_cfg = {
	.mouse_button_qty = 1,
	.mouse_buttons = &button2,
	.keycode_qty = 0,
	.keycodes = NULL,
};

struct mousebutton_config b3_cfg = {
	.mouse_button_qty = 1,
	.mouse_buttons = &button3,
	.keycode_qty = 0,
	.keycodes = NULL,
};

static struct mtm_region_config base_layer[] = {
	{
		.region_type = "joymouse",
		.region_options = &mousecfg,
		.minx = 0,
		.miny = 0,
		.maxx = 800,
		.maxy = 700
	},
	{
		.region_type = "mousebutton",
		.region_options = &b1_cfg,
		.minx = 0,
		.miny = 700,
		.maxx = 330,
		.maxy = 1000,
	},
	{
		.region_type = "mousebutton",
		.region_options = &b3_cfg,
		.minx = 330,
		.miny = 700,
		.maxx = 660,
		.maxy = 1000,
	},
	{
		.region_type = "mousebutton",
		.region_options = &b2_cfg,
		.minx = 660,
		.miny = 700,
		.maxx = 1000,
		.maxy = 1000,
	}
};

static struct mtm_region_config shortcut_layer[] = {
	
};

static struct mtm_region_list default_layers[] = {
	{
		.regions = base_layer,
		.num_regions = ARRAY_SIZE(base_layer),
	},
	{
		.regions = shortcut_layer,
		.num_regions = ARRAY_SIZE(shortcut_layer),
	}
};

#endif
