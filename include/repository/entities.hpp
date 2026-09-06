#pragma once

#include <optional>
#include <string>
#include <vector>
#include <cstdint>

namespace repository
{

struct HexagonLeaderboardEntry
{
    uint64_t user_id{0};
    std::string username;
    std::string player_color_hex;
    int32_t uram_points{0};
    double total_distance_meters{0.0};
    int32_t visits_count{0};
    std::string last_visited_at;
};

struct HexagonHistoryEntry
{
    uint64_t id{0};
    uint64_t h3_index{0};
    std::optional<uint64_t> previous_owner_id{std::nullopt};
    std::string previous_owner_name;
    uint64_t new_owner_id{0};
    std::string new_owner_name;
    int32_t score_at_capture{0};
    std::string captured_at;
};

struct HexagonEntity
{
    uint64_t h3_index{0};
    std::optional<uint64_t> owner_user_id{std::nullopt};
    int32_t top_score{0};
    std::string captured_at;
};

struct HexagonOwnerRecord
{
    uint64_t h3_index{0};
    uint64_t owner_user_id{0};
    int32_t top_score{0};
};

struct HexagonProgressResult
{
    bool is_captured{false};
    uint64_t h3_index{0};
    uint64_t new_owner_id{0};
    std::optional<uint64_t> prev_owner_id{std::nullopt};
    int32_t new_top_score{0};
};

struct HexagonLeaderboardCacheItem
{
    uint64_t user_id{0};
    int32_t uram_points{0};
};

struct UserProfileCache
{
    uint64_t user_id{0};
    std::string username;
    std::string avatar_url;
    std::string player_color_hex;
    uint64_t team_id{0};
    std::string team_color_hex;
};

} // namespace repository