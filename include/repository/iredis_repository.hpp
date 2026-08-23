#pragma once

#include <boost/asio.hpp>

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

    virtual net::awaitable<std::vector<std::pair<uint64_t, uint64_t>>> GetHexagonOwnersBatch(
        const std::vector<uint64_t>& h3_indices
    ) = 0;
    
    virtual net::awaitable<bool> PushTrackPoint(uint64_t run_id, const GpsPoint& point) = 0;
    virtual net::awaitable<bool> UpdateActiveRunStats(uint64_t run_id, double delta_meters, int32_t delta_points) = 0;
    virtual net::awaitable<bool> ClearActiveRun(uint64_t run_id) = 0;
};

} // namespace repository
