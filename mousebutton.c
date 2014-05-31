#include "mtm.h"
#include <xf86_OSproc.h>

struct button_region_info;

struct button_info {
	struct button_region_info *rinfo;
	DeviceIntPtr device;
};

struct button_region_info {
	struct button_info *slotinfo;
	int buttonid;
};
static struct button_info buttondata[2];

static int mousebutton_touch_start(struct mtm_touch_slot *slot, int slotid, struct mtm_region *region) {
	struct button_region_info *rinfo = (struct button_region_info *)region->region_private;
	struct button_info *data = &(rinfo->slotinfo[slotid]);

	slot->tracker_private = data;

	xf86PostButtonEvent(data->device, FALSE, data->rinfo->buttonid, TRUE, 0, 0);

	return MTM_TRACKFLAGS_TRACK;
}

static int tick_delta(int delta) {
	int sign = delta > 0 ? 1 : -1;
	int mag = delta * sign;

	if (mag < 400) return (mag * 4) * sign;
	else return (400 * 4+ (mag-400) * 8) * sign;

	return delta*4;
}

static void mousebutton_touch_update(struct mtm_touch_slot *slot) {
	(void)slot;
}
static void mousebutton_touch_end(struct mtm_touch_slot *slot) {
	struct button_info *data = slot->tracker_private;
	xf86PostButtonEvent(data->device, FALSE, data->rinfo->buttonid, FALSE, 0, 0);
}

static void mousebutton_touch_timer(struct mtm_touch_slot *slot) {
}
struct mtm_slot_tracker mousebutton_tracker = {
	.touch_start = mousebutton_touch_start,
	.touch_update = mousebutton_touch_update,
	.touch_end = mousebutton_touch_end,
	.touch_timer_fire = mousebutton_touch_timer,
};

static struct mtm_region *mousebutton_init_region(struct mtm_region_config *config, struct mtm_info *mtm, int slot_qty) {
	struct mtm_region *region = NULL;
	struct button_region_info *rinfo = NULL;
	struct button_info *slotinfo = NULL;
	int i;

	region = malloc(sizeof(struct mtm_region));
	if (!region) {
		goto out_err_cleanup_none;
	}

	region->tracker = &mousebutton_tracker;


	
	rinfo = malloc(sizeof(struct button_region_info));
	if (!rinfo) {
		goto out_err_cleanup_region;
	}
	region->region_private = rinfo;
	rinfo->buttonid = *((int *)config->region_options);

	slotinfo = malloc(sizeof(struct button_info) * slot_qty);
	if (!slotinfo) {
		goto out_err_cleanup_rinfo;
	}
	rinfo->slotinfo = slotinfo;

	for (i=0; i<slot_qty; i++) {
		rinfo->slotinfo[i].device = mtm->pinfo->dev;
		rinfo->slotinfo[i].rinfo = rinfo;
	}

	return region;

out_err_cleanup_rinfo:
	free(rinfo);

out_err_cleanup_region:
	free(region);

out_err_cleanup_none:
	return NULL;
}

static void mousebutton_uninit_region(struct mtm_region *region) {
	free(region->region_private);
	free(region);
}

struct mtm_region_type mousebutton_type = {
	.type_name = "mousebutton",
	.init_region = mousebutton_init_region,
	.uninit_region = mousebutton_uninit_region,
};
