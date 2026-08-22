#include "utils/uram_h3.hpp"

namespace utils::h3
{

H3Index PointToH3(double latitude, double longitude, int resolution)
{
    LatLng location;
    location.lat = degsToRads(latitude);
    location.lng = degsToRads(longitude);
    
    H3Index cell;
    latLngToCell(&location, resolution, &cell);
    return cell;
}

std::vector<H3Index> GetHexagonsInRadius(H3Index center_cell, int k_ring_radius)
{
    int64_t max_cells = 0;
    maxGridDiskSize(k_ring_radius, &max_cells);
    std::vector<H3Index> result(max_cells);
    gridDisk(center_cell, k_ring_radius, result.data());
    
    result.erase(std::remove(result.begin(), result.end(), 0), result.end());
    return result;
}

std::vector<H3Index> GetPathBetweenHexes(H3Index start_cell, H3Index end_cell)
{
    int64_t path_size = 0;
    gridPathCellsSize(start_cell, end_cell, &path_size);
    std::vector<H3Index> path(path_size);
    gridPathCells(start_cell, end_cell, path.data());
    return path;
}

} // namespace utils::h3