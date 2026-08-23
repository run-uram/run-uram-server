#include "utils/uram_h3.hpp"

namespace utils::h3
{

H3Index PointToH3(double latitude, double longitude, int resolution)
{
    LatLng location;
    location.lat = degsToRads(latitude);
    location.lng = degsToRads(longitude);
    
    H3Index cell = 0;
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

std::vector<H3Index> GetCellsInBoundingBox(
    double south_west_lat, double south_west_lng,
    double north_east_lat, double north_east_lng,
    int resolution)
{
    LatLng verts[4];
    verts[0].lat = degsToRads(south_west_lat);
    verts[0].lng = degsToRads(south_west_lng); 

    verts[1].lat = degsToRads(north_east_lat);
    verts[1].lng = degsToRads(south_west_lng);

    verts[2].lat = degsToRads(north_east_lat);
    verts[2].lng = degsToRads(north_east_lng);

    verts[3].lat = degsToRads(south_west_lat);
    verts[3].lng = degsToRads(north_east_lng);

    GeoLoop outer_loop;
    outer_loop.numVerts = 4;
    outer_loop.verts = verts;

    GeoPolygon polygon;
    polygon.geoloop = outer_loop;
    polygon.numHoles = 0;
    polygon.holes = nullptr;

    int64_t max_cells = 0;
    if (maxPolygonToCellsSize(&polygon, resolution, 0, &max_cells) != E_SUCCESS || max_cells <= 0)
    {
        return {};
    }

    std::vector<H3Index> cells(max_cells);
    if (polygonToCells(&polygon, resolution, 0, cells.data()) != E_SUCCESS)
    {
        return {};
    }

    cells.erase(std::remove(cells.begin(), cells.end(), 0), cells.end());
    return cells;
}

H3Index GetParent(H3Index cell, int parent_resolution)
{
    H3Index parent = 0;
    cellToParent(cell, parent_resolution, &parent);
    return parent;
}

std::vector<H3Index> GetParentZones(const std::vector<H3Index>& cells, int parent_resolution)
{
    std::unordered_set<H3Index> unique_parents;
    unique_parents.reserve(cells.size() / 7 + 1);

    for (H3Index cell : cells)
    {
        H3Index parent = GetParent(cell, parent_resolution);
        if (parent != 0)
        {
            unique_parents.insert(parent);
        }
    }

    return std::vector<H3Index>(unique_parents.begin(), unique_parents.end());
}

double GetDistanceMeters(double lat1, double lng1, double lat2, double lng2)
{
    LatLng a{degsToRads(lat1), degsToRads(lng1)};
    LatLng b{degsToRads(lat2), degsToRads(lng2)};
    return greatCircleDistanceM(&a, &b);
}

} // namespace utils::h3