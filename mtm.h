#ifndef _MTM_H
#define _MTM_H

#include <mtdev.h>
#include <xf86Xinput.h>

#define MIN_TOUCH_SLOTS 2
#define MTM_MOUSE_BUTTONS 10
#define TIMER_HZ 60

struct mtm_info;
struct mtm_touch_slot;
struct mtm_slot_tracker;
struct mtm_region;

/* Config Use */
struct mtm_region_config {
	const char *region_type;
	void *region_options; // I'll fix this later, I swear

	int minx, maxx, miny, maxy; //Normalized coords 0,0 to 1000,1000 for whole trackpad
};

struct mtm_region_list {
	unsigned int num_regions;
	struct mtm_region_config *regions;
};

/* Internal Use */
struct mtm_region_type {
	const char *type_name;
	struct mtm_region *(*init_region)(struct mtm_region_config *config, struct mtm_info *mtm, int slot_qty);
	void (*uninit_region)(struct mtm_region *region);
};

struct mtm_region {
	struct mtm_slot_tracker *tracker;
	void *region_private;

	struct mtm_info *mtm;
	int minx, maxx, miny, maxy;
	struct mtm_region *next;
	struct mtm_region_type *type;
};


struct mtm_slot_tracker {
	int (*touch_start)(struct mtm_touch_slot *slot, int slot_index, struct mtm_region *region);
/* touch_start is called when an untracked touch happens inside this tracker's
 * area. It should return a bitfield of MTM_TRACKFLAGS_* bits. */

	void (*touch_update)(struct mtm_touch_slot *slot);
/* touch_update is called when an already tracked touch changes positions. */

	void (*touch_end)(struct mtm_touch_slot *slot);
/* touch_end is called when a tracked touch is lifted. Note that as this is 
 * called slot->id is different than previous callbacks */

	void (*touch_timer_fire)(struct mtm_touch_slot *slot);
/* touch_timer_fire is called TIMER_HZ times per second during the touch if
 * MTM_TRACKFLAGS_WANTS_TIMER is set in the return from touch_start. */
};

struct mtm_touch_slot {
	struct mtm_info *parent;
	int xpos, ypos;
	int id;

	int track_flags;
	int change_flags;


	struct mtm_slot_tracker *tracker;
	void *tracker_private;
};

struct mtm_info {
	InputInfoPtr pinfo;
	struct mtdev *mt;
	int fd;

	struct mtm_region *region;

	int minx, maxx;
	int miny, maxy;

	int active_slot;
	int num_slots;
	struct mtm_touch_slot *slots;

	int timer_count;
	OsTimerPtr timer;
};


/* Set MTM_TRACKFLAGS_TRACK if you want to "lock" the touch to your tracker. If
 * this isn't set, the touch_start callback will be called for every frame the
 * touch is in the tracker's area. If it is set, touch_update will be called
 * for every frame after until touch_end is called. */

#define MTM_TRACKFLAGS_TRACK        0x00000001 
#define MTM_TRACKFLAGS_WANTS_TIMER  0x00000002


#define MTM_CHANGE_X                0x00000001
#define MTM_CHANGE_Y                0x00000002
#define MTM_CHANGE_START            0x00000004
#define MTM_CHANGE_END              0x00000008

#endif
