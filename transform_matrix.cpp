// algorithm from matrix.pl by Frank Rysanek and Petr Mikse
// http://support.fccps.cz/download/adv/frr/matrix_calibrator/matrix-1.1.tgz
// http://support.fccps.cz/download/adv/frr/matrix_calibrator/matrix_calibrator.htm
// ##########################
// # Calculation equations:
// # 1.	x0x * a + x0y * b + c = 1/8	(the left point equation)
// # 2.	x1x * a + x1y * b + c = 7/8	(the right point equation)
// # 3.	a / b = dxx / dxy		(tg alfa)
// # Subtract 1. from 2.:
// # 4.	(x1x - x0x) * a + (x1y - x0y) * b = 6/8
// # 5.	dxx * a + dxy * b = 6/8
// # Isolate (b) from 3.:
// # 6.	b = a * dxy / dxx
// # Substitute (b) in 5.:
// # 7.	dxx * a + dxy * a * dxy / dxx = 6/8
// # Isolate (a) from 7.:
// # 8.	a * (dxx + dxy^2 / dxx) = 6/8
// # 9.	a * (dxx^2 + dxy^2) / dxx = 6/8
// # 10.	a = 6/8 * dxx / (dxx^2 + dxy^2)	!RESULT!
// # Analogically for (b):
// # 11.	b = 6/8 * dxy / (dxx^2 + dxy^2) !RESULT!
// # We will get (c) substituting (a) and (b) in 1.
// # For (d), (e) and (f) we use the same solution analogically.

#include "transform_matrix.h"
#include "log.h"

#include <cmath>
#include <string>
#include <sstream>

struct tm_xy_t
{
	double x;
	double y;
};

transform_matrix_t transform_matrix(const touch_point_list_t& touch_point_list, int width, int height)
{
	double width_coef = 1. * touch_point_list[UL].point.x / width;
	double height_coef = 1. *
		(touch_point_list[LR].point.y - touch_point_list[UR].point.y)/
		height;

	LOG("width:%d height:%d", width, height);
	LOG("touch_point_list[UL].point.x:%d touch_point_list[LR].point.y:%d", touch_point_list[UL].point.x, touch_point_list[LR].point.y);
	LOG("width_coef:%f height_coef:%f", width_coef, height_coef);
	std::array<tm_xy_t, 4> clicks;
	for(size_t idx = 0;
		idx < touch_point_list.size() && idx < clicks.size();
		++idx)
	{
		tm_xy_t xy{};
		xy.x = touch_point_list[idx].touch.x;
		xy.y = touch_point_list[idx].touch.y;
		clicks[idx].x = xy.x / width;
		clicks[idx].y = xy.y / height;
	}
	double x0x = (clicks[0].x + clicks[2].x) / 2;
	double x0y = (clicks[0].y + clicks[2].y) / 2;
	double x1x = (clicks[1].x + clicks[3].x) / 2;
	double x1y = (clicks[1].y + clicks[3].y) / 2;
	double y0x = (clicks[0].x + clicks[1].x) / 2;
	double y0y = (clicks[0].y + clicks[1].y) / 2;
	double y1x = (clicks[2].x + clicks[3].x) / 2;
	double y1y = (clicks[2].y + clicks[3].y) / 2;
	double dxx = x1x - x0x;
	double dxy = x1y - x0y;
	double dyx = y1x - y0x;
	double dyy = y1y - y0y;
	double a = height_coef * (dxx / (( dxx *  dxx) + (dxy *  dxy)));
	double b = height_coef * (dxy / (( dxx *  dxx) + (dxy *  dxy)));
	double c = width_coef - (a * x0x) - (b *  x0y);
	double d = height_coef * (dyx / ((dyx *  dyx) + (dyy *  dyy)));
	double e = height_coef * (dyy / ((dyx *  dyx) + (dyy *  dyy)));
	double f = width_coef - (d *  y0x) - (e *  y0y);

	return transform_matrix_t{
		static_cast<float>(a),
		static_cast<float>(b),
		static_cast<float>(c),
		static_cast<float>(d),
		static_cast<float>(e),
		static_cast<float>(f),
		0, 0, 1};
}

bool transform_matrix_valid(const transform_matrix_t& matr)
{
	for (const auto& val : matr)
	{
		if (std::isnan(val))
			return false;
	}
	return true;
}

std::string transform_matrix_to_str(const transform_matrix_t& matr)
{
	std::stringstream ss;
	for (const auto& val : matr)
		ss << val << " ";
	return ss.str();
}
