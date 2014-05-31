#ifndef _MTM_JOYMOUSE_H
#define _MTM_JOYMOUSE_H

#define MTM_JOYAXIS_MOUSE 0
#define MTM_JOYAXIS_WHEEL 2

struct joymouse_config {
	int joyaxis_single;
	int joyaxis_multi;

	int tap_max_ticks;
	int tap_max_distance;
	int tap_button;

	int num_points;
	int *distance;
	int *velocity;
};

#endif
