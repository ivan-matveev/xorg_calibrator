#include "touch_device.h"
#include "log.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>

bool calibratable(XDeviceInfoPtr info)
{
	XAnyClassPtr any = (XAnyClassPtr) (info->inputclassinfo);
	for (int cnt = 0; cnt < info->num_classes; cnt++,
		any = reinterpret_cast<XAnyClassPtr>(reinterpret_cast<uint8_t*>(any) + any->length))
	{
		if (any->c_class != ValuatorClass)
			continue;

		XValuatorInfoPtr V = (XValuatorInfoPtr) any;
		XAxisInfoPtr ax = (XAxisInfoPtr) V->axes;
		any = (XAnyClassPtr) ((char *) any + any->length);

		if (V->mode != Absolute) {
			LOG("DEBUG: Skipping device '%s' id=%i, does not report Absolute events.",
					info->name, (int)info->id);
			continue;
		}
		if (V->num_axes < 2 ||
			(ax[0].min_value == -1 && ax[0].max_value == -1) ||
			(ax[1].min_value == -1 && ax[1].max_value == -1)) {
			LOG("DEBUG: Skipping device '%s' id=%i, does not have two calibratable axes.",
					info->name, (int)info->id);
			continue;
		} 
		return true;
	}
	return false;
}

device_info_list_t device_info_list_get()
{
	device_info_list_t device_info_list;

    Display* display = XOpenDisplay(NULL);
    if (display == NULL) {
        ERR("Unable to connect to X server");
        return device_info_list;
    }

    int xi_opcode, event, error;
    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &event, &error))
    {
        ERR("X Input extension not available.");
        return device_info_list;
    }

	XExtensionVersion *version = XGetExtensionVersion(display, INAME);
	if (version && (version != (XExtensionVersion*) NoSuchExtension))
	{
		LOG("%s version is %i.%i\n",
			INAME, version->major_version, version->minor_version);
		XFree(version);
	}

    XDeviceInfoPtr list, slist;
    int ndevices;
    slist = list = (XDeviceInfoPtr) XListInputDevices (display, &ndevices);
    for (int i = 0; i < ndevices; i++, list++)
    {
		// skip virtual master device
        if (list->use == IsXKeyboard || list->use == IsXPointer)
            continue;
        device_info_list.emplace_back(device_info_t{list->id, list->name, calibratable(list)});
    }

    XFreeDeviceList(slist);
    XCloseDisplay(display);

	return device_info_list;
}

bool set_matrix(Display *dpy, int deviceid, const transform_matrix_t& matr)
{
	if (!transform_matrix_valid(matr))
	{
		ERR("failed: transform_matrix_valid()");
		return false;
	}

	LOG("deviceid: %d", deviceid);
	
	Atom prop_float = XInternAtom(dpy, "FLOAT", False);
	Atom prop_matrix = XInternAtom(dpy, "Coordinate Transformation Matrix", False);

	if (!prop_float)
	{
		ERR("Float atom not found. This server is too old.\n");
		return false;
	}
	if (!prop_matrix)
	{
		ERR("Coordinate transformation matrix not found. This "
				"server is too old\n");
		return false;
	}

	int format_return;
	Atom type_return;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char* prop_matrix_data = nullptr;
	int rc = XIGetProperty(dpy, deviceid, prop_matrix, 0, 9, False, prop_float,
					   &type_return, &format_return, &nitems, &bytes_after,
					   &prop_matrix_data);
	if (rc != Success || prop_float != type_return || format_return != 32 ||
		nitems != 9 || bytes_after != 0 || prop_matrix_data == nullptr)
	{
		ERR("Failed to retrieve current property values\n");
		return false;
	}

	float* prop_matrix_data_float = reinterpret_cast<float*>(prop_matrix_data);
	for (size_t idx = 0; idx < 9; idx++)
		prop_matrix_data_float[idx] = matr[idx];

	XIChangeProperty(dpy, deviceid, prop_matrix, prop_float,
					 format_return, PropModeReplace, prop_matrix_data, nitems);

	XFree(prop_matrix_data);
	XSync(dpy, False);

	return true;
}

#ifdef TOUCH_DEVICE_TEST

bool verbose = true;

int main()
{
	device_info_list_t dev_info_list = device_info_list_get();
	for (auto info : dev_info_list)
		std::cout << info.xid << " " << info.name << " " << info.calibratable << "\n";

	return 0;
}

#endif // TOUCH_DEVICE_TEST
