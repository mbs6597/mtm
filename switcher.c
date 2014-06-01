#include "mtm.h"
#include <xf86_OSproc.h>

struct switcher_region_info;

struct switcher_info {
	struct mtm_info *mtm;
	struct mtm_layer_selection *handle; 
};

struct switcher_region_info {
	struct switcher_info *slotinfo;
	int target_layer;
};

static int switcher_touch_start(struct mtm_touch_slot *slot, int slotid, struct mtm_region *region) {
	struct switcher_region_info *rinfo = (struct switcher_region_info *)region->region_private;
	struct switcher_info *data = &(rinfo->slotinfo[slotid]);

	slot->tracker_private = data;
	data->handle = force_layer (data->mtm, rinfo->target_layer);

	return MTM_TRACKFLAGS_TRACK;
}

static void switcher_touch_update(struct mtm_touch_slot *slot) {
	(void) slot;
}
static void switcher_touch_end(struct mtm_touch_slot *slot) {
	struct switcher_info *data = slot->tracker_private;

	if (data->handle) {
		release_layer (data->mtm, data->handle);
		data->handle = NULL;
	}

}

static void switcher_touch_timer(struct mtm_touch_slot *slot) {
	(void) slot;
}
struct mtm_slot_tracker switcher_tracker = {
	.touch_start = switcher_touch_start,
	.touch_update = switcher_touch_update,
	.touch_end = switcher_touch_end,
	.touch_timer_fire = switcher_touch_timer,
};

static struct mtm_region *switcher_init_region(struct mtm_region_config *config, struct mtm_info *mtm, int slot_qty) {
	struct mtm_region *region = NULL;
	struct switcher_region_info *rinfo = NULL;
	struct switcher_info *slotinfo = NULL;
	int i;

	region = malloc(sizeof(struct mtm_region));
	if (!region) {
		goto out_err_cleanup_none;
	}

	region->tracker = &switcher_tracker;
	
	rinfo = malloc(sizeof(struct switcher_region_info));
	if (!rinfo) {
		goto out_err_cleanup_region;
	}
	region->region_private = rinfo;
	rinfo->target_layer = *((int*)config->region_options);

	slotinfo = malloc(sizeof(struct switcher_info) * slot_qty);
	if (!slotinfo) {
		goto out_err_cleanup_rinfo;
	}
	rinfo->slotinfo = slotinfo;

	for (i=0; i<slot_qty; i++) {
		rinfo->slotinfo[i].mtm = mtm;
		rinfo->slotinfo[i].handle = NULL;
	}

	return region;

out_err_cleanup_rinfo:
	free(rinfo);

out_err_cleanup_region:
	free(region);

out_err_cleanup_none:
	return NULL;
}

static void switcher_uninit_region(struct mtm_region *region) {
	struct switcher_region_info *rinfo = region->region_private;

	free(rinfo->slotinfo);
	free(region->region_private);
	free(region);
}

struct mtm_region_type switcher_type = {
	.type_name = "switcher",
	.init_region = switcher_init_region,
	.uninit_region = switcher_uninit_region,
};
