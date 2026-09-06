#pragma once

#include <boost/asio.hpp>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

#include "repository/entities.hpp"
#include "repository/run_models.hpp"

namespace net = boost::asio;

namespace repository
{

class IRedisRepository
{
public:
    virtual ~IRedisRepository() = default;

    virtual net::awaitable<bool> Set(const std::string& key, const std::string& value, std::chrono::seconds ttl) = 0;
    virtual net::awaitable<std::optional<std::string>> Get(const std::string& key) = 0;
    virtual net::awaitable<bool> Delete(const std::string& key) = 0;

    virtual net::awaitable<bool> Publish(const std::string& channel, const std::string& payload) = 0;
    virtual net::awaitable<bool> WarmupHexagonOwners(const std::vector<HexagonOwnerRecord>& records) = 0;

    virtual net::awaitable<std::optional<uint64_t>> GetHexagonOwner(uint64_t h3_index) = 0;
    virtual net::awaitable<bool> SetHexagonOwner(uint64_t h3_index, uint64_t owner_id) = 0;

    virtual net::awaitable<std::vector<std::pair<uint64_t, uint64_t>>> GetHexagonOwnersBatch(
        const std::vector<uint64_t>& h3_indices
    ) = 0;

    virtual net::awaitable<std::vector<HexagonLeaderboardCacheItem>> GetHexagonLeaderboardFromCache(
        uint64_t h3_index, 
        size_t limit = 10
    ) = 0;

    virtual net::awaitable<bool> SetHexagonLeaderboard(
        uint64_t h3_index, 
        const std::vector<HexagonLeaderboardEntry>& entries, 
        std::chrono::seconds ttl = std::chrono::seconds(7 * 86400)
    ) = 0;

    virtual net::awaitable<bool> UpdateHexagonScoreInCache(
        uint64_t h3_index, 
        uint64_t user_id, 
        int32_t uram_points
    ) = 0;

    virtual net::awaitable<std::optional<UserProfileCache>> GetUserProfileCache(uint64_t user_id) = 0;

    virtual net::awaitable<std::unordered_map<uint64_t, UserProfileCache>> GetUserProfilesBatch(
        const std::vector<uint64_t>& user_ids
    ) = 0;

    virtual net::awaitable<bool> SetUserProfileCache(
        uint64_t user_id, 
        const UserProfileCache& profile, 
        std::chrono::seconds ttl = std::chrono::seconds(86400)
    ) = 0;
    
    virtual net::awaitable<bool> PushTrackPoint(uint64_t run_id, const GpsPoint& point) = 0;
    virtual net::awaitable<std::vector<GpsPoint>> GetTrackPoints(uint64_t run_id) = 0;
    virtual net::awaitable<bool> UpdateActiveRunStats(uint64_t run_id, double delta_meters, int32_t delta_points) = 0;
    virtual net::awaitable<bool> ClearActiveRun(uint64_t run_id) = 0;
};

} // namespace repository
