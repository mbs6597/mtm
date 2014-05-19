
#include <xf86Xinput.h>
#include <stdio.h>
#include <fcntl.h>
#include <xorg-server.h>
#include <errno.h>

/*
    Bool (*device_control) (DeviceIntPtr device, int what);
    void (*read_input) (struct _InputInfoRec * local);
    int (*control_proc) (struct _InputInfoRec * local, xDeviceCtl * control);
    int (*switch_mode) (ClientPtr client, DeviceIntPtr dev, int mode);
    int (*set_device_valuators)
     (struct _InputInfoRec * local,
      int *valuators, int first_valuator, int num_valuators);



    proto = xf86SetStrOption(pInfo->options, "Protocol", NULL)
    device = xf86SetStrOption(pInfo->options, "Device", NULL);
    char *proto, *device;
*/
Bool mtm_device_control(DeviceIntPtr device, int what) {
	return 0;
}
void mtm_read_input(InputInfoPtr pinfo) {

}
int mtm_control_proc(InputInfoPtr pinfo, xDeviceCtl * control) {
	return 0;
}

int mtm_switch_mode(ClientPtr client, DeviceIntPtr dev, int mode) {
	return XI_BadMode;
}

void mtm_un_init(InputDriverPtr drv, InputInfoPtr pinfo, int flags) {
}

int fd;
int mtm_pre_init(InputDriverPtr drv, InputInfoPtr pinfo, int flags) {
	char *proto, *device;
	xf86IDrvMsg(pinfo, X_ERROR, "mskiba pre init!\n");

	pinfo->type_name = XI_TOUCHPAD;
	pinfo->device_control = mtm_device_control;
	pinfo->read_input = mtm_read_input;
	pinfo->control_proc = mtm_control_proc;
	pinfo->switch_mode = mtm_switch_mode;

	
	fd = xf86OpenSerial(pinfo->options);
	xf86IDrvMsg(pinfo, X_ERROR, "mskiba fd: %d errno: %d\n", fd, errno);

	return 0;
}

InputDriverRec MTM = {
    1,
    "mtm",
    NULL,
    mtm_pre_init,
    mtm_un_init,
    NULL,
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
    xf86AddInputDriver(&MTM, module, 0);
    return module;
}

_X_EXPORT XF86ModuleData mtmModuleData = {
    &VersionRec,
    &SetupProc,
    NULL
};
