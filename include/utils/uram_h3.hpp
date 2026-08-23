#pragma once

#include <h3/h3api.h>

#include <algorithm>
#include <unordered_set>
#include <vector>

namespace utils::h3
{

H3Index PointToH3(double latitude, double longitude, int resolution);
std::vector<H3Index> GetHexagonsInRadius(H3Index center_cell, int k_ring_radius);
std::vector<H3Index> GetPathBetweenHexes(H3Index start_cell, H3Index end_cell);

std::vector<H3Index> GetCellsInBoundingBox(
    double south_west_lat, double south_west_lng,
    double north_east_lat, double north_east_lng,
    int resolution
);

H3Index GetParent(H3Index cell, int parent_resolution);

std::vector<H3Index> GetParentZones(const std::vector<H3Index>& cells, int parent_resolution);

double GetDistanceMeters(double lat1, double lng1, double lat2, double lng2);

} // namespace utils::h3