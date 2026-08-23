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
    net::awaitable<std::vector<std::pair<uint64_t, uint64_t>>> GetHexagonOwnersBatch(
        const std::vector<uint64_t>& h3_indices
    ) override;

    net::awaitable<bool> PushTrackPoint(uint64_t run_id, const GpsPoint& point) override;
    net::awaitable<bool> UpdateActiveRunStats(uint64_t run_id, double delta_meters, int32_t delta_points) override;
    net::awaitable<bool> ClearActiveRun(uint64_t run_id) override;
};

} // namespace repository
