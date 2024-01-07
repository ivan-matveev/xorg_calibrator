#ifndef TRANSFORM_MATRIX_H
#define TRANSFORM_MATRIX_H

#include "common.h"

#include <array>
#include <string>
#include <vector>

using transform_matrix_t = std::array<float, 9>;

transform_matrix_t transform_matrix(const touch_point_list_t& touch_point_list, int width, int height);
bool transform_matrix_valid(const transform_matrix_t& matr);
std::string transform_matrix_to_str(const transform_matrix_t& matr);

#endif  // TRANSFORM_MATRIX_H
