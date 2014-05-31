#ifndef _MTM_MOUSEBUTTON_H
#define _MTM_MOUSEBUTTON_H

struct mousebutton_config {
	int mouse_button_qty;
	int *mouse_buttons;

	int keycode_qty;
	int *keycodes;
};

#endif
