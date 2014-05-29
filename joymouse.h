#ifndef _MTM_JOYMOUSE_H
#define _MTM_JOYMOUSE_H

#define MTM_JOYAXIS_MOUSE 0
#define MTM_JOYAXIS_WHEEL 2

struct joymouse_config {
	int joyaxis;

	int num_points;
	int *distance;
	int *velocity;
};

#endif
