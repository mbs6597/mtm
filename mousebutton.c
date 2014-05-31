#include "mtm.h"
#include "mousebutton.h"
#include <xf86_OSproc.h>

struct button_region_info;

struct button_info {
	struct button_region_info *rinfo;
	DeviceIntPtr device;
};

struct button_region_info {
	struct button_info *slotinfo;
	struct mousebutton_config *cfg;
};
static struct button_info buttondata[2];

static int mousebutton_touch_start(struct mtm_touch_slot *slot, int slotid, struct mtm_region *region) {
	struct button_region_info *rinfo = (struct button_region_info *)region->region_private;
	struct button_info *data = &(rinfo->slotinfo[slotid]);
	int i;

	slot->tracker_private = data;

	for (i=0; i<data->rinfo->cfg->mouse_button_qty; i++) {
		xf86PostButtonEvent(data->device, FALSE, data->rinfo->cfg->mouse_buttons[i], TRUE, 0, 0);
	}

	for (i=0; i<data->rinfo->cfg->keycode_qty; i++) {
		xf86PostKeyEvent(data->device, data->rinfo->cfg->keycodes[i], 1, 0, 0, 0);
	}

	return MTM_TRACKFLAGS_TRACK;
}

static void mousebutton_touch_update(struct mtm_touch_slot *slot) {
	(void)slot;
}
static void mousebutton_touch_end(struct mtm_touch_slot *slot) {
	struct button_info *data = slot->tracker_private;
	int i;

	for (i = data->rinfo->cfg->mouse_button_qty - 1; i>=0; i--) {
		xf86PostButtonEvent(data->device, FALSE, data->rinfo->cfg->mouse_buttons[i], FALSE, 0, 0);
	}

	for (i = data->rinfo->cfg->keycode_qty - 1; i>=0; i--) {
		xf86PostKeyEvent(data->device, data->rinfo->cfg->keycodes[i], 0, 0, 0, 0);
	}
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
	rinfo->cfg = (struct mousebutton_config *)config->region_options;

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
