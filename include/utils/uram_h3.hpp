#pragma once

#include <h3/h3api.h>

#include <vector>

namespace utils::h3
{

H3Index PointToH3(double latitude, double longitude, int resolution = 9);

std::vector<H3Index> GetHexagonsInRadius(H3Index center_cell, int k_ring_radius);

std::vector<H3Index> GetPathBetweenHexes(H3Index start_cell, H3Index end_cell);

} // namespace utils::h3