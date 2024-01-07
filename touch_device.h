#ifndef TOUCH_DEVICE_H
#define TOUCH_DEVICE_H

#include "transform_matrix.h"

#include <X11/extensions/XInput.h>

#include <string>
#include <vector>

struct device_info_t
{
	XID xid;
	std::string name;
	bool calibratable;
};

using device_info_list_t = std::vector<device_info_t>;

device_info_list_t device_info_list_get();
bool set_matrix(Display *dpy, int deviceid, const transform_matrix_t& matr);

#endif  // TOUCH_DEVICE_H
