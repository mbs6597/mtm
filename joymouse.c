#include "mtm.h"
#include "joymouse.h"
#include <xf86_OSproc.h>

struct joy_region_info;

struct joy_info {
	int startx, starty;
	int progressx;
	int progressy;
	struct joy_region_info *rinfo;
	DeviceIntPtr device;
};

struct joy_region_info {
	struct joy_info *slotinfo;
	struct joymouse_config cfg;
};
static struct joy_info joydata[2];

static int joymouse_touch_start(struct mtm_touch_slot *slot, int slotid, struct mtm_region *region) {
	struct joy_region_info *rinfo = (struct joy_region_info *)region->region_private;
	struct joy_info *data = &(rinfo->slotinfo[slotid]);

	slot->tracker_private = data;

	data->startx = slot->xpos;
	data->starty = slot->ypos;
	data->progressx = 0;
	data->progressy = 0;

	return MTM_TRACKFLAGS_TRACK | MTM_TRACKFLAGS_WANTS_TIMER;
}

static void velocities(int dx, int dy, int *vxvy, struct joy_region_info *info) {
	int distsq;
	int dist;
	int up, down;
	int ud, dd, uv, dv;
	int velocity = 0;

	if (!info->cfg.num_points) {
		vxvy[0] = 0;
		vxvy[1] = 0;
		return;
	}

	distsq = dx*dx + dy*dy;
	dist = (int) sqrtf((float)distsq);

	if (!dist) {
		vxvy[0] = 0;
		vxvy[1] = 0;
		return;
	}

	down = 0;
	up = info->cfg.num_points-1;
	while (abs(up - down) > 1) {
		int mid = (up + down) / 2;
		if (info->cfg.distance[mid] > dist) {
			up = mid;
		} else {
			down = mid;
		}
	}

	ud = info->cfg.distance[up];
	dd = info->cfg.distance[down];
	uv = info->cfg.velocity[up];
	dv = info->cfg.velocity[down];

	if (dist-dd) {
		velocity = dv+(((uv-dv)*(dist-dd))/(ud-dd));
	} else {
		velocity = dv;
	}

	vxvy[0] = velocity * dx / dist;
	vxvy[1] = velocity * dy / dist;
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
	int vxvy[] = {0,0};

	velocities(slot->xpos - data->startx, slot->ypos - data->starty, vxvy, data->rinfo);

	data->progressx += vxvy[0];
	data->progressy += vxvy[1];

	dx = data->progressx/1000;
	dy = data->progressy/1000;
	data->progressx %= 1000;
	data->progressy %= 1000;

        xf86PostMotionEvent(data->device, 0, data->rinfo->cfg.joyaxis, 2, dx, dy);
}
struct mtm_slot_tracker joymouse_tracker = {
	.touch_start = joymouse_touch_start,
	.touch_update = joymouse_touch_update,
	.touch_end = joymouse_touch_end,
	.touch_timer_fire = joymouse_touch_timer,
};

static struct mtm_region *joymouse_init_region(struct mtm_region_config *config, struct mtm_info *mtm, int slot_qty) {
	struct mtm_region *region = NULL;
	struct joy_region_info *rinfo = NULL;
	struct joy_info *slotinfo = NULL;
	struct joymouse_config *jcfg = (struct joymouse_config *)(config->region_options);
	int i;

	region = malloc(sizeof(struct mtm_region));
	if (!region) {
		goto out_err_cleanup_none;
	}

	region->tracker = &joymouse_tracker;


	
	rinfo = malloc(sizeof(struct joy_region_info));
	if (!rinfo) {
		goto out_err_cleanup_region;
	}
	region->region_private = rinfo;


	slotinfo = malloc(sizeof(struct joy_info) * slot_qty);
	if (!slotinfo) {
		goto out_err_cleanup_rinfo;
	}
	rinfo->slotinfo = slotinfo;

	rinfo->cfg= *jcfg;

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

static void joymouse_uninit_region(struct mtm_region *region) {
	free(region->region_private);
	free(region);
}

struct mtm_region_type joymouse_type = {
	.type_name = "joymouse",
	.init_region = joymouse_init_region,
	.uninit_region = joymouse_uninit_region,
};
