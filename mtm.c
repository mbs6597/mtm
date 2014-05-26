#include <errno.h>
#include <fcntl.h>
#include <mtdev.h>
#include <stdio.h>
#include <xf86Xinput.h>
#include <xf86_OSproc.h>
#include <xorg-server.h>
#include <exevents.h>
#include <xserver-properties.h>
#include <string.h>

#include "mtm.h"
#include "config.h"

static int mt_abs_slot_types[] = MT_SLOT_ABS_EVENTS;
extern struct mtm_slot_tracker joymouse_tracker;

static struct mtm_region_type *find_region_type(const char *name) {
	struct mtm_region_type *type = NULL;
	unsigned int i;

	for (i=0; i<ARRAY_SIZE(region_types); i++) {
		if (!strcmp(name, region_types[i]->type_name)) {
			return region_types[i];
		}
	}

	return type;
}

static int mtm_init_regions(struct mtm_info *mtm) {
	unsigned int i;
	int ret = Success;
	struct mtm_region **nextptr = &(mtm->region);

	for (i=0; i<ARRAY_SIZE(default_config); i++) {
		struct mtm_region_type *type;
		struct mtm_region *new;

		type = find_region_type(default_config[i].region_type);
		if (!type) {
			ret = BadRequest;
			break;
		}

		new = type->init_region(&default_config[i], mtm->num_slots);
		if (!new) {
			ret = BadAlloc;
			break;
		}

		new->next = NULL;
		new->mtm = mtm;
		new->type = type;
		{
			int dx = mtm->maxx-mtm->minx;
			int dy = mtm->maxy-mtm->miny;

			new->minx = ((dx * default_config[i].minx)/1000) + mtm->minx;
			new->miny = ((dy * default_config[i].miny)/1000) + mtm->miny;
			new->maxx = ((dx * default_config[i].maxx)/1000) + mtm->minx;
			new->maxy = ((dy * default_config[i].maxy)/1000) + mtm->miny;

			
			xf86IDrvMsg(mtm->pinfo, X_INFO, "%d %d %d %d x %d %d %d %d = %d %d %d %d\n",
				mtm->minx, mtm->maxx, mtm->miny, mtm->maxy,
				default_config[i].minx, default_config[i].maxx, default_config[i].miny, default_config[i].maxy,
				new->minx, new->maxx, new->miny, new->maxy);
		}

		*nextptr = new;
		nextptr = &(new->next);
	}

	if (ret != Success) {
		struct mtm_region *region, *next;
		for (region = mtm->region; region; region = next) {
			next = region->next;
			region->type->uninit_region(region);
		}
	}

	xf86IDrvMsg(mtm->pinfo, X_INFO, "region ptr of mtm: %p\n", mtm->region);

	return ret;
}

static void mtm_uninit_regions(struct mtm_info *mtm) {
	struct mtm_region *region, *next;
	for (region = mtm->region; region; region = next) {
		next = region->next;
		region->type->uninit_region(region);
	}
}

static Bool in_region(struct mtm_region *region, int x, int y) {
	return x >= region->minx && 
	       x < region->maxx &&
	       y >= region->miny &&
	       y < region->maxy;
}

static struct mtm_region *get_region(struct mtm_info *mtm, int x, int y) {
	struct mtm_region *region;
	for (region = mtm->region; region; region=region->next) {
		if (in_region(region, x, y)) {
			return region;
		}
	}
	return NULL;
}

static void mtm_setup_slots(struct mtm_info *mtm) {
	struct mtm_touch_slot *slot = mtm->slots;
	int i;
	
	for (i=0; i<mtm->num_slots; i++) {
		slot->id = -1;
		slot->parent = mtm;
		slot->track_flags = 0;
		slot->change_flags = 0;
		slot->tracker = NULL;
		slot->tracker_private = NULL;
		slot->xpos = -1;
		slot->ypos = -1;
		slot++;
	}
	mtm->active_slot = 0;
}

static int mtm_device_control_on(InputInfoPtr pinfo) {
	int fd, i, ret = Success;
	struct mtm_info *mtm = pinfo->private;
	struct mtdev *mt;
	Bool xset=FALSE, yset=FALSE;

	xf86IDrvMsg(pinfo, X_ERROR, "mskiba pinfo: %p\n", pinfo);

	fd = xf86OpenSerial(pinfo->options);
	if (fd == -1) {
		xf86IDrvMsg(pinfo, X_ERROR, "could not open serial device.\n");
		ret = BadAlloc;
		goto out_err_cleanup_none;
	}
	mtm->fd = fd;
	pinfo->fd = fd;

	mt = mtdev_new_open(fd);
	if (!mt) {
		xf86IDrvMsg(pinfo, X_ERROR, "failed to open multitouch device.\n");
		ret = BadAlloc;
		goto out_err_cleanup_serial;
	}
	mtm->mt = mt;

	for (i=0; i<MT_ABS_SIZE; i++) {
		if (mt->caps.has_abs[i]) {
			if (mt_abs_slot_types[i]==ABS_MT_POSITION_X) {
				mtm->minx = mt->caps.abs[i].minimum;
				mtm->maxx = mt->caps.abs[i].maximum;
				xset = TRUE;
			} else if (mt_abs_slot_types[i]==ABS_MT_POSITION_Y) {
				mtm->miny = mt->caps.abs[i].minimum;
				mtm->maxy = mt->caps.abs[i].maximum;
				yset = TRUE;
			}
		}
	}
	if (!(xset && yset)) {
		xf86IDrvMsg(pinfo, X_ERROR, "failed to find axis limits.\n");
		ret = BadValue;
		goto out_err_cleanup_mt;

	}

	mtm_setup_slots(mtm);

	ret = mtm_init_regions(mtm);
	if (ret != Success) {
		xf86IDrvMsg(pinfo, X_ERROR, "failed to initialize regions.\n");
		goto out_err_cleanup_mt;
	}


	mtm->timer_count = 0;
	mtm->timer = TimerSet(NULL, 0, 0, NULL, NULL);
	if (!mtm->timer) {
		ret = BadAlloc;
		goto out_err_cleanup_mt;
	}

	xf86AddEnabledDevice(pinfo);

	return ret;

out_err_cleanup_mt:
	mtdev_close_delete(mt);
	mtm->mt = NULL;

out_err_cleanup_serial:
	xf86CloseSerial(fd);
	mtm->fd = -1;
	pinfo->fd = -1;

out_err_cleanup_none:
	return ret;
}

static int mtm_device_control_off(InputInfoPtr pinfo) {
	struct mtm_info *mtm = pinfo->private;

	TimerFree(mtm->timer);


	xf86RemoveEnabledDevice(pinfo);

	mtm_uninit_regions(mtm);

	mtdev_close_delete(mtm->mt);
	mtm->mt = NULL;

	xf86CloseSerial(mtm->fd);
	xf86IDrvMsg(pinfo, X_ERROR, "fd set tot -1 \n");
	mtm->fd = -1;
	pinfo->fd = -1;

	return Success;
}

static Bool mtm_device_control(DeviceIntPtr device, int mode) {
	InputInfoPtr pinfo = device->public.devicePrivate;
	Atom labels[2];
	switch(mode) {
	case DEVICE_INIT:
		labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
		labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

		InitValuatorClassDeviceStruct(device, 2, labels, GetMotionHistorySize(), Relative);

		xf86IDrvMsg(pinfo, X_INFO, "device in cfg: %p\n", device);
		
		xf86InitValuatorAxisStruct(device, 0, labels[0], 0,
                           -1, 1000, 0, 1000,
                           Relative);
		//xf86InitValuatorDefaults(device, 0);

		xf86InitValuatorAxisStruct(device, 1, labels[1], 0,
                           -1, 1000, 0, 1000,
                           Relative);
		//xf86InitValuatorDefaults(device, 1);

		xf86IDrvMsg(pinfo, X_ERROR, "mode init!\n");
		break;
	case DEVICE_ON:
		xf86IDrvMsg(pinfo, X_ERROR, "mode dev on!\n");
		return mtm_device_control_on(pinfo);
	case DEVICE_OFF:
		xf86IDrvMsg(pinfo, X_ERROR, "mode dev off!\n");
		return mtm_device_control_off(pinfo);
	case DEVICE_CLOSE:
		xf86IDrvMsg(pinfo, X_ERROR, "mode dev close!\n");
		break;
		
	}
	return 0;
}

static CARD32 mtm_timer_fire(OsTimerPtr timer, CARD32 now, pointer arg) {
	struct mtm_info *mtm = (struct mtm_info*)arg;
	int sigstate, i;

	(void) timer;
	(void) now;

	sigstate = xf86BlockSIGIO();

	for (i=0; i<mtm->num_slots; i++) {
		if (mtm->slots[i].track_flags & MTM_TRACKFLAGS_WANTS_TIMER && mtm->slots[i].tracker) {
			mtm->slots[i].tracker->touch_timer_fire(mtm->slots+i);
		}
	}

	TimerSet(mtm->timer, 0, 1000/TIMER_HZ, mtm_timer_fire, (void *)mtm);

	xf86UnblockSIGIO(sigstate);
	return 0;
}

static void mtm_read_input(InputInfoPtr pinfo) {
	struct mtm_info *mtm = pinfo->private;
	int msg;
	struct input_event ev;

	while (1) {
		msg = mtdev_get(mtm->mt, mtm->fd, &ev, 1);
		if (msg != 1) {
			break;
		}

		if (ev.type == EV_ABS) {
			if (ev.code == ABS_MT_SLOT) {
				mtm->active_slot = ev.value;

			} else if (mtm->active_slot < mtm->num_slots) {
				if (ev.code == ABS_MT_POSITION_X) {
					mtm->slots[mtm->active_slot].xpos = ev.value;
					mtm->slots[mtm->active_slot].change_flags |= MTM_CHANGE_X;

				} else if (ev.code == ABS_MT_POSITION_Y) {
					mtm->slots[mtm->active_slot].ypos = ev.value;
					mtm->slots[mtm->active_slot].change_flags |= MTM_CHANGE_Y;

				} else if (ev.code == ABS_MT_TRACKING_ID) {

					if (mtm->slots[mtm->active_slot].id != -1)  {
						mtm->slots[mtm->active_slot].change_flags |= MTM_CHANGE_END;
					}

					mtm->slots[mtm->active_slot].id = ev.value;

					if (ev.value != -1) {
						mtm->slots[mtm->active_slot].change_flags |= MTM_CHANGE_START;
					}
				}
			}
		} else if (ev.type == EV_SYN) {
			int i;
			for (i=0; i<mtm->num_slots; i++) {
				if (mtm->slots[i].change_flags & MTM_CHANGE_END) {
					if (mtm->slots[i].tracker) {
						mtm->slots[i].tracker->touch_end(mtm->slots+i);
						mtm->slots[i].tracker = NULL;
						mtm->slots[i].tracker_private = NULL;
						if (mtm->slots[i].track_flags & MTM_TRACKFLAGS_WANTS_TIMER && 
						    mtm->slots[i].track_flags & MTM_TRACKFLAGS_TRACK) {
							mtm->timer_count -= 1;
							if (!mtm->timer_count) {
								TimerCancel(mtm->timer);
							}
						}
						mtm->slots[i].track_flags = 0;
					}
				}
				if ( (mtm->slots[i].change_flags & MTM_CHANGE_START) || 
				     (mtm->slots[i].tracker==NULL && mtm->slots[i].id != -1)) {
					struct mtm_region *region = get_region(mtm, mtm->slots[i].xpos, mtm->slots[i].ypos);

					if (region) {
						int flags = region->tracker->touch_start(mtm->slots+i, i, region);

						mtm->slots[i].track_flags = flags;

						if (flags & MTM_TRACKFLAGS_TRACK) {
							mtm->slots[i].tracker = region->tracker;
							if (flags & MTM_TRACKFLAGS_WANTS_TIMER) {
								if (!mtm->timer_count) {
									TimerSet(mtm->timer, 0, 1000/TIMER_HZ, mtm_timer_fire, (void *)mtm);
								}
								mtm->timer_count+=1;
							}
						}
					}
				} else if (mtm->slots[i].change_flags && mtm->slots[i].tracker) {
					mtm->slots[i].tracker->touch_update(mtm->slots+i);
				}
				mtm->slots[i].change_flags = 0;
			}
		}
	} 
}

static int mtm_control_proc(InputInfoPtr pinfo, xDeviceCtl * control) {
	(void)(pinfo);
	(void)(control);
	return 0;
}

static int mtm_switch_mode(ClientPtr client, DeviceIntPtr dev, int mode) {
	(void)(client);
	(void)(dev);
	(void)(mode);

	return XI_BadMode;
}

static void mtm_un_init(InputDriverPtr drv, InputInfoPtr pinfo, int flags) {
	(void)(drv);
	(void)(pinfo);
	(void)(flags);
}

static int mtm_pre_init(InputDriverPtr drv, InputInfoPtr pinfo, int flags) {
	int fd, ret=Success, num_slots;
	struct mtdev *mt;
	struct mtm_info *mtm;

	(void)(flags);
	(void)(drv);

	pinfo->type_name = XI_TOUCHPAD;
	pinfo->device_control = mtm_device_control;
	pinfo->read_input = mtm_read_input;
	pinfo->control_proc = mtm_control_proc;
	pinfo->switch_mode = mtm_switch_mode;

	mtm = malloc(sizeof(struct mtm_info));
	if (!mtm) {
		xf86IDrvMsg(pinfo, X_ERROR, "could not allocate info structure.\n");
		ret = BadAlloc;
		goto out_err_cleanup_none;
	}
	mtm->pinfo = pinfo;
	mtm->mt = NULL;
	mtm->fd = -1;
	mtm->timer_count = 0;
	mtm->region = NULL;
	xf86IDrvMsg(pinfo, X_ERROR, "fd set to -1 place 2\n");
	
	pinfo->private = mtm;
	
	fd = xf86OpenSerial(pinfo->options);
	if (fd == -1) {
		xf86IDrvMsg(pinfo, X_ERROR, "could not open serial device.\n");
		ret = BadAlloc;
		goto out_err_cleanup_private;
	}

	mt = mtdev_new_open(fd);
	if (!mt) {
		xf86IDrvMsg(pinfo, X_ERROR, "Failed to open multitouch device. (Is it not a multitouch device?)\n");
		ret = BadAlloc;
		goto out_err_cleanup_serial;
	}

	num_slots = mt->caps.slot.maximum - mt->caps.slot.minimum + 1;
	if (num_slots < MIN_TOUCH_SLOTS) {
		xf86IDrvMsg(pinfo, X_ERROR, "mutitouch device cannot track enough fingers.\n");
		ret = BadValue;
		goto out_err_cleanup_mt;
	}
	mtm->num_slots = num_slots;
	mtm->slots = malloc(num_slots * (sizeof(struct mtm_touch_slot)));
	if (!mtm->slots) {
		xf86IDrvMsg(pinfo, X_ERROR, "could not allocate slot structure.\n");
		ret = BadAlloc;
		goto out_err_cleanup_mt;
	}

	mtdev_close_delete(mt);

	xf86CloseSerial(fd);

	return ret;

out_err_cleanup_mt:
	mtdev_close_delete(mt);

out_err_cleanup_serial:
	xf86CloseSerial(fd);

out_err_cleanup_private:
	free(pinfo->private);
	pinfo->private = NULL;

out_err_cleanup_none:
	return ret;
}

static InputDriverRec MTM = {
	1,
	"mtm",
	NULL,
	mtm_pre_init,
	mtm_un_init,
	NULL,
	NULL
};

static XF86ModuleVersionInfo VersionRec = {
	"mtm",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XORG_VERSION_CURRENT,
	1, 0, 0,
	ABI_CLASS_XINPUT,
	ABI_XINPUT_VERSION,
	MOD_CLASS_XINPUT,
	{0, 0, 0, 0}
};

static pointer
SetupProc(pointer module, pointer options, int *errmaj, int *errmin)
{
	(void)options;
	(void)errmaj;
	(void)errmin;
	xf86AddInputDriver(&MTM, module, 0);
	return module;
}

_X_EXPORT XF86ModuleData mtmModuleData = {
	&VersionRec,
	&SetupProc,
	NULL
};
