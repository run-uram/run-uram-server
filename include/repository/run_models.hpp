#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace repository
{

struct GpsPoint
{
    double latitude{0.0};
    double longitude{0.0};
    double speed{0.0};
    int64_t timestamp{0};
};

struct RunSummary
{
    uint64_t run_id{0};
    uint64_t user_id{0};
    double distance_meters{0.0};
    int64_t duration_seconds{0};
    int32_t uram_points_earned{0};
    int32_t hexagons_captured{0};
    std::string started_at;
    std::string finished_at;
    std::string encoded_track_geojson;
};

} // namespace repository