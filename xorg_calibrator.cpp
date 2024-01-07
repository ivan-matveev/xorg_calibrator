#include "common.h"
#include "screen_x11.h"
#include "touch_device.h"
#include "transform_matrix.h"
#include "log.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <cerrno>
#include <cstdio>
#include <cstring>

bool verbose = false;

struct config_t
{
	bool list;
	bool fake;
	bool reset;
	bool help;
	bool verbose;
	std::string device_name;
	int device_id = -1;
	int screen_num = invalid_screen_num;
	std::vector<std::string> message;
	std::string output_filename;
	int timeout = 0; // seconds 0 - forever
};

struct key_val_t
{
	std::string key;
	std::string val;
};

key_val_t key_val_split(const std::string& str, const std::string& delimiter)
{
	char key[256]{};
	char val[256]{};
	const char* delimiter_ptr = strstr(str.c_str(), delimiter.c_str());
	if (delimiter_ptr != nullptr)
	{
		int key_len = std::min(static_cast<size_t>(delimiter_ptr - str.c_str()), sizeof(key));
		strncpy(key, str.c_str(), key_len);
		strncpy(val, delimiter_ptr + delimiter.size(), sizeof(val));
	}
	else
		strncpy(key, str.c_str(), sizeof(key));

	return key_val_t{key, val};
}

std::vector<std::string> parse_message(const std::string& str)
{
	std::vector<std::string> message;
	auto key_val = key_val_split(str, "\\n");
	message.push_back(key_val.key);
	while(!key_val.val.empty())
	{
		key_val = key_val_split(key_val.val, "\\n");
		message.push_back(key_val.key);
	}
	
	return message;
}

config_t parse_opts(int argc, const char* argv[])
{
	config_t config{};
	for (ssize_t idx = 1; idx < argc; ++idx)
	{
		std::string opt = argv[idx];
		while(opt.size() > 0 && opt[0] == '-')
			opt.erase(0, 1);
		key_val_t key_val = key_val_split(opt, "=");
		if (key_val.key == "list")
			config.list = true;
		else if (key_val.key == "fake")
			config.fake = true;
		else if (key_val.key == "reset")
			config.reset = true;
		else if (key_val.key == "h")
			config.help = true;
		else if (key_val.key == "help")
			config.help = true;
		else if (key_val.key == "device_name")
			config.device_name = key_val.val;
		else if (key_val.key == "device_id")
			config.device_id = strtol(key_val.val.c_str(), NULL, 10);
		else if (key_val.key == "screen_num")
			config.screen_num = strtol(key_val.val.c_str(), NULL, 10);
		else if (key_val.key == "message")
			config.message = parse_message(key_val.val);
		else if (key_val.key == "output_filename")
			config.output_filename = key_val.val;
		else if (key_val.key == "verbose")
			config.verbose = true;
		else if (key_val.key == "timeout")
			config.timeout = strtol(key_val.val.c_str(), NULL, 10);

	}
	return config;
}

constexpr int cross_size = 100;

void draw_touch_point(screen_x11_t& scr, xy_t xy, color_index_t color_index)
{
	scr.cross(xy, cross_size, color_index);
	scr.circle(xy, cross_size / 5, color_index);
}

void draw_message(screen_x11_t& scr, const std::vector<std::string>& str_list)
{
	int max_width = -1;
	for (const auto str : str_list)
		max_width = std::max(max_width, scr.text_width(str.c_str()));
	int line_spacing = scr.text_height() / 2;
	int height = str_list.size() * (scr.text_height() + line_spacing);
	int rect_offset = scr.text_height();
	xy_t xy{};
	xy.x = (scr.width_ / 2) - (max_width / 2);
	xy.y = (scr.height_ / 2) - (height / 2);
	rect_t rect{
		{
			(scr.width_ / 2) - ((max_width + rect_offset) / 2),
			(scr.height_ / 2) - ((height + rect_offset) / 2)
		},
		{xy.x + max_width + rect_offset, xy.y + height + rect_offset}
	};
	scr.rect(rect, BLACK);

	for(int idx = 0; idx < str_list.size(); ++idx)
	{
		scr.text(
			{
				(scr.width_ / 2) - (scr.text_width(str_list[idx].c_str()) / 2),
				xy.y + ((idx + 1) * (scr.text_height() + line_spacing))
			},
			str_list[idx].c_str());
	}
}

bool wait_touch_or_key(screen_x11_t& scr, xy_t& xy, size_t timeout_s)
{
	auto start = std::chrono::system_clock::now();
	while(timeout_s == 0 ? true : std::chrono::system_clock::now() - start < std::chrono::seconds(timeout_s))
	{
		unsigned int keycode;
		if (scr.get_touch_button_event(xy, keycode))
		{
			LOG("%d:%d", xy.x, xy.y);
			return true;
		}
		else
			if (keycode != 0)
				return false;
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	ERR("timeout %zu sec. is over", timeout_s);
	return false;
}

bool get_touch_point_list(screen_x11_t& scr, touch_point_list_t& touch_point_list, size_t timeout_s)
{
	for (size_t idx = 0; idx < touch_point_list.size(); ++idx)
	{
		draw_touch_point(scr, touch_point_list[idx].point, RED);
		if (idx > 0)
			draw_touch_point(scr, touch_point_list[idx - 1].point, WHITE);

		xy_t xy;
		if (wait_touch_or_key(scr, xy, timeout_s))
			touch_point_list[idx].touch = xy;
		else
			return false;
	}
	return true;
}

using matr33_row_t = std::array<float, 3>;
using matr33_t = std::array<matr33_row_t, 3>;

std::string xorg_str(const transform_matrix_t& transform_matrix, const char* device_name)
{
    char line[1024];
    std::string outstr;
    outstr += "Section \"InputClass\"\n";
    outstr += "	Identifier	\"calibration\"\n";
    sprintf(line, "	MatchProduct	\"%s\"\n", device_name);
    outstr += line;
    sprintf(line, "	Option	\"TransformationMatrix\"	\"%f %f %f %f %f %f %f %f %f\"\n",
		transform_matrix[0], transform_matrix[1], transform_matrix[2],
		transform_matrix[3], transform_matrix[4], transform_matrix[5],
		transform_matrix[6], transform_matrix[7], transform_matrix[8]
		);
    outstr += line;
    outstr += "EndSection\n";
    return outstr;
}

std::string xinput_str(const transform_matrix_t& transform_matrix, const char* device_name)
{
    char line[1024];
    std::string outstr;
    sprintf(line, "xinput set-prop \"%s\" \"Coordinate Transformation Matrix\" %f %f %f %f %f %f %f %f %f \n",
		device_name,
		transform_matrix[0], transform_matrix[1], transform_matrix[2],
		transform_matrix[3], transform_matrix[4], transform_matrix[5],
		transform_matrix[6], transform_matrix[7], transform_matrix[8]
		);
    outstr += line;
	return outstr;
}

device_info_t select_device(const device_info_list_t& dev_info_list, const config_t& config)
{
	device_info_t device_info{.calibratable = false};
	bool multiple = false;
	for (auto info : dev_info_list)
	{
		if (config.device_id == info.xid)
			return info;
		if (config.device_name == info.name)
			return info;
		if (info.calibratable)
		{
			if (device_info.calibratable)
				multiple = true;
			device_info = info;
		}
	}
	if (multiple)
		ERR("Warning: multiple calibratable devices found, calibrating last one (%s)\n\t"
			"use --device to select another one.\n", device_info.name.c_str());
	if (!device_info.calibratable && !config.device_name.empty())
		ERR("Error: Device \"%s\" not found; use --list to list the calibratable input devices.\n", config.device_name.c_str());
	if (!device_info.calibratable && config.device_id != -1)
		ERR("Error: Device id: %d not found; use --list to list the calibratable input devices.\n", config.device_id);
	return device_info;
}

bool write_file(const std::string& file_name, const char *data, size_t size)
{
    FILE* fh = fopen(file_name.c_str(), "w");
    if (fh == nullptr)
    {
        ERR("failed: fopen(): %s : %s", file_name.c_str(), strerror(errno));
        return false;
    }
    ssize_t written = fwrite(data, 1, size, fh);
    if (written != static_cast<ssize_t>(size))
    {
        ERR("failed: fwrite(): %s : %s", file_name.c_str(), strerror(errno));
        fclose(fh);
        return false;
    }
    fclose(fh);
    return true;
}

bool reset_calibration(Display *display, int deviceid)
{
	transform_matrix_t transform_matrix{
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0
		};
	return set_matrix(display, deviceid, transform_matrix);
}

void usage()
{
	std::cout
		<< "\n"
		<< "Usage: ./xorg_calibrator [options]\n"
		<< "\n"
		<< "options:\n"
		<< "list - list calibratable devices \n"
		<< "device_name - name of the device to calibrate\n"
		<< "device_id - id of the device to calibrate\n"
		<< "fake - use fake device for test\n"
		<< "reset - reset calibration to default\n"
		<< "h or help - output this help message\n"
		<< "screen_num - number of the screen to calibrate\n"
		<< "message - message to show on calibration screen. Strings separated by \\n. UTF8 is supported\n"
		<< "output_filename - name of the file to write callibration config to\n"
		<< "verbose - print a lot of log messages\n"
		<< "timeout - seconds to wait for users touches. Default - wait forever\n"
		<< "\n\n"
		<< "If no 'device_name' or 'device_id' option given last calibratible device is selected.\n"
		<< "\n\n"
		<< "Example:\n"
		<< "./xorg_calibrator output_filename=/usr/share/X11/xorg.conf.d/99-calibration.conf\n"
		<< "\n\n"
		;
}

int main(int argc, const char* argv[])
{
	config_t config = parse_opts(argc, argv);
	verbose = config.verbose;
	device_info_list_t dev_info_list = device_info_list_get();
	if (config.help)
	{
		usage();
		return EXIT_SUCCESS;
	}

	if (config.list)
	{
		for (auto info : dev_info_list)
			std::cout << "id: " << info.xid << " \"" << info.name << "\" calibratable:" << info.calibratable << "\n";
		return EXIT_SUCCESS;
	}

	device_info_t device_info{.name = "fake"};
	if (!config.fake)
	{
		if (dev_info_list.empty())
		{
			ERR("Error: No devices found.");
			return EXIT_FAILURE;
		}
		device_info = select_device(dev_info_list, config);
		if (!device_info.calibratable)
		{
			ERR("Error: No calibratable devices found.");
			return EXIT_FAILURE;
		}
	}
	LOG("Selected device: id: %d \"%s\" ", device_info.xid, device_info.name.c_str());

	screen_x11_t scr(config.screen_num);
	ASSERT(scr.is_valid);
	LOG("scr.width_:%d scr.height_:%d", scr.width_, scr.height_);

	if (!config.fake)
	{
		if (!reset_calibration(scr.display_, device_info.xid))
		{
			ERR("failed: reset_calibration()");
			return EXIT_FAILURE;
		}
	}
	if (config.reset)
		return EXIT_SUCCESS;

	std::vector<std::string> message{
		"Touchscreen calibration",
		"Press red cross center",
		"Any key to abort"
		};
	if (!config.message.empty())
		message = config.message;
	draw_message(scr, message);

	touch_point_list_t touch_point_list{};

	int cross_offset_x = scr.width_ / 9;
	int cross_offset_y = scr.height_ / 9;
	touch_point_list[UL].point = {cross_offset_x, cross_offset_y};
	touch_point_list[UR].point = {scr.width_ - cross_offset_x, cross_offset_y};
	touch_point_list[LL].point = {cross_offset_x, scr.height_ - cross_offset_y};
	touch_point_list[LR].point = {scr.width_ - cross_offset_x, scr.height_ - cross_offset_y};

	if (!get_touch_point_list(scr, touch_point_list, config.timeout))
	{
		ERR("Aborted");
		return EXIT_FAILURE;
	}

	for (size_t idx = 0; idx < touch_point_list.size(); ++idx)
		LOG("point: %d:%d\ttouch : %d:%d",
			touch_point_list[idx].point.x, touch_point_list[idx].point.y,
			touch_point_list[idx].touch.x, touch_point_list[idx].touch.y);

	transform_matrix_t transform_matrix = ::transform_matrix(touch_point_list, scr.width_, scr.height_);
	if (!transform_matrix_valid(transform_matrix))
	{
		ERR("failed: transform_matrix_valid() transform_matrix: %s",
			transform_matrix_to_str(transform_matrix).c_str());
		ERR("Probably there were misclicks");
		return EXIT_FAILURE;
	}

	if (!config.fake)
	{
		if (!set_matrix(scr.display_, device_info.xid, transform_matrix))
		{
			ERR("failed: set_matrix()");
			return EXIT_FAILURE;
		}
	}

    std::string outstr = xorg_str(transform_matrix, device_info.name.c_str());
    printf("%s", outstr.c_str());
    if (!config.output_filename.empty())
		if (!write_file(config.output_filename, outstr.c_str(), outstr.length()))
			return EXIT_FAILURE;

	return EXIT_SUCCESS;
}
