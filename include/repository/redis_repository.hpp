#pragma once

#include <boost/redis/connection.hpp>
#include <boost/redis/request.hpp>
#include <boost/redis/response.hpp>
#include <boost/asio/awaitable.hpp>
#include <memory>
#include <chrono>

#include "repository/iredis_repository.hpp"


namespace repository
{

class RedisRepository : public IRedisRepository
{
private:
    std::shared_ptr<boost::redis::connection> m_connection;

public:
    explicit RedisRepository(net::any_io_executor executor);
    ~RedisRepository() override = default;

    // Pub/Sub
    net::awaitable<bool> Publish(const std::string& channel, const std::string& payload) override;

    // Cache-Aside
    net::awaitable<bool> Set(const std::string& key, const std::string& value, std::chrono::seconds ttl) override;
    net::awaitable<std::optional<std::string>> Get(const std::string& key) override;
    net::awaitable<bool> Delete(const std::string& key) override;

    net::awaitable<bool> WarmupHexagonOwners(const std::vector<HexagonOwnerRecord>& records) override;
    
    net::awaitable<std::optional<uint64_t>> GetHexagonOwner(uint64_t h3_index) override;
    net::awaitable<bool> SetHexagonOwner(uint64_t h3_index, uint64_t owner_id) override;
    net::awaitable<std::vector<std::pair<uint64_t, uint64_t>>> GetHexagonOwnersBatch(
        const std::vector<uint64_t>& h3_indices
    ) override;

    net::awaitable<std::vector<HexagonLeaderboardCacheItem>> GetHexagonLeaderboardFromCache(
        uint64_t h3_index, 
        size_t limit = 10
    ) override;

    net::awaitable<bool> SetHexagonLeaderboard(
        uint64_t h3_index, 
        const std::vector<HexagonLeaderboardEntry>& entries, 
        std::chrono::seconds ttl = std::chrono::seconds(7 * 86400)
    ) override;

    net::awaitable<bool> UpdateHexagonScoreInCache(
        uint64_t h3_index, 
        uint64_t user_id, 
        int32_t uram_points
    ) override;

    net::awaitable<std::optional<UserProfileCache>> GetUserProfileCache(uint64_t user_id) override;

    net::awaitable<std::unordered_map<uint64_t, UserProfileCache>> GetUserProfilesBatch(
        const std::vector<uint64_t>& user_ids
    ) override;

    net::awaitable<bool> SetUserProfileCache(
        uint64_t user_id, 
        const UserProfileCache& profile, 
        std::chrono::seconds ttl = std::chrono::seconds(86400)
    ) override;

    net::awaitable<bool> PushTrackPoint(uint64_t run_id, const GpsPoint& point) override;
    net::awaitable<std::vector<GpsPoint>> GetTrackPoints(uint64_t run_id) override;
    net::awaitable<bool> UpdateActiveRunStats(uint64_t run_id, double delta_meters, int32_t delta_points) override;
    net::awaitable<bool> ClearActiveRun(uint64_t run_id) override;
};

} // namespace repository
