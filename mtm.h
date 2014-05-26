#ifndef _MTM_H
#define _MTM_H

#include <mtdev.h>

#define MIN_TOUCH_SLOTS 2
#define TIMER_HZ 120

struct mtm_info;
struct mtm_touch_slot;
struct mtm_slot_tracker;
struct mtm_region;

struct mtm_region {
	struct mtm_info *mtm;
	struct mtm_slot_tracker *tracker;
	void *regiondata;

	//TODO linked list of regions queryable by initial x/y
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
