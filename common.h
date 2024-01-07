#ifndef COMMON_H
#define COMMON_H

#define HAVE_XFT 1

#include <cstdint>
#include <array>
#include <vector>

using coordinate_t = int32_t;

struct xy_t
{
	coordinate_t x;
	coordinate_t y;
	bool operator == (const xy_t& other)
	{
		return x == other.x && y == other.y;
	}
	bool operator != (const xy_t& other)
	{
		return x != other.x || y != other.y;
	}
};

enum {
    UL = 0, // Upper-left
    UR = 1, // Upper-right
    LL = 2, // Lower-left
    LR = 3, // Lower-right
    NUM_POINTS
};

struct touch_point_t
{
	xy_t point;
	xy_t touch;
};

using touch_point_list_t = std::array<touch_point_t, 4>;

#endif  // COMMON_H
