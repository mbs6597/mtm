#include "mtm.h"
#include <xf86_OSproc.h>

struct joy_info {
	struct mtm_region *region;
	int startx, starty, slot_id;
	int progressx;
	int progressy;
};
static struct joy_info joydata[2];

static int joymouse_touch_start(struct mtm_touch_slot *slot, int slotid, struct mtm_region *region) {
	struct joy_info *data = ((struct joy_info *)region->region_private)+slotid;

	slot->tracker_private = data;

	data->region = region;
	data->startx = slot->xpos;
	data->starty = slot->ypos;
	data->progressx = 0;
	data->progressy = 0;
	data->slot_id = slotid;

	return MTM_TRACKFLAGS_TRACK | MTM_TRACKFLAGS_WANTS_TIMER;
}

static int tick_delta(int delta) {
	int sign = delta > 0 ? 1 : -1;
	int mag = delta * sign;

	if (mag < 400) return (mag * 4) * sign;
	else return (400 * 4+ (mag-400) * 8) * sign;

	return delta*4;
}

static void joymouse_touch_update(struct mtm_touch_slot *slot) {
	(void)slot;
	//The slot tracks the absolute position, and timer fire uses it to move
}
static void joymouse_touch_end(struct mtm_touch_slot *slot) {
	(void)slot;
}

static void joymouse_touch_timer(struct mtm_touch_slot *slot) {
	struct joy_info *data = slot->tracker_private;
	int dx, dy;

	data->progressx += tick_delta(slot->xpos - data->startx);
	data->progressy += tick_delta(slot->ypos - data->starty);

	dx = data->progressx/1000;
	dy = data->progressy/1000;
	data->progressx %= 1000;
	data->progressy %= 1000;

        xf86PostMotionEvent(data->region->mtm->pinfo->dev, 0, 0, 2, dx, dy);
}
struct mtm_slot_tracker joymouse_tracker = {
	.touch_start = joymouse_touch_start,
	.touch_update = joymouse_touch_update,
	.touch_end = joymouse_touch_end,
	.touch_timer_fire = joymouse_touch_timer,
};

static struct mtm_region *joymouse_init_region(struct mtm_region_config *config, int slot_qty) {
	struct mtm_region *region = NULL;
	int i;

	region = malloc(sizeof(struct mtm_region));
	if (!region) {
		goto out_err_cleanup_none;
	}

	region->tracker = &joymouse_tracker;


	
	region->region_private = malloc(sizeof(struct joy_info) * slot_qty);
	if (!region->region_private) {
		goto out_err_cleanup_region;
	}
	
	for (i=0; i<slot_qty; i++) {
		struct joy_info *joy = ((struct joy_info *)region->region_private)+i;
		joy->region = region;
	}

	return region;

out_err_cleanup_region:
	free(region);

out_err_cleanup_none:
	return NULL;
}

static void joymouse_uninit_region(struct mtm_region *region) {
	free(region->region_private);
	free(region);
}

struct mtm_region_type joymouse_type = {
	.type_name = "joymouse",
	.init_region = joymouse_init_region,
	.uninit_region = joymouse_uninit_region,
};
