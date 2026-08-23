#include "repository/redis_repository.hpp"
#include "logger/logger.hpp"
#include <cstdlib>
#include <boost/redis/src.hpp>
#include <charconv>
#include <fmt/format.h>

namespace repository
{

static std::string GetEnvOr(const char* name, const std::string& fallback)
{
    const char* val = std::getenv(name);
    return val ? val : fallback;
}

RedisRepository::RedisRepository(net::any_io_executor executor)
    : m_connection(std::make_shared<boost::redis::connection>(net::make_strand(executor)))
{
    boost::redis::config config;
    config.addr.host = GetEnvOr("REDIS_HOST", "127.0.0.1");
    config.addr.port = GetEnvOr("REDIS_PORT", "6379");
    
    std::string password = GetEnvOr("REDIS_PASSWORD", "");
    if (!password.empty())
    {
        config.password = password;
    }

    config.reconnect_wait_interval = std::chrono::seconds(2);

    config.health_check_interval = std::chrono::seconds(10);

    m_connection->async_run(config, net::consign(net::detached, m_connection));
}

net::awaitable<bool> RedisRepository::Set(const std::string& key, const std::string& value, std::chrono::seconds ttl)
{
    boost::redis::request request;
    request.push("SET", key, value, "EX", std::to_string(ttl.count()));

    boost::redis::generic_response response;
    sys::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::cancel_after(std::chrono::seconds(3), net::redirect_error(net::use_awaitable, ec))
    );

    if (ec)
    {
        LOG_ERROR("Redis SET for key {} failed: {}", key, ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<std::optional<std::string>> RedisRepository::Get(const std::string& key)
{
    boost::redis::request request;
    request.push("GET", key);

    boost::redis::response<std::optional<std::string>> response;
    sys::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::cancel_after(std::chrono::seconds(3), net::redirect_error(net::use_awaitable, ec))
    );

    if (ec)
    {
        LOG_ERROR("Redis GET failed for key {}: {}", key, ec.message());
        co_return std::nullopt;
    }

    auto& result = std::get<0>(response);

    if (!result.has_value())
    {
        LOG_ERROR("Redis GET response adapter error for key {}", key);
        co_return std::nullopt;
    }

    co_return result.value();
}

net::awaitable<bool> RedisRepository::Delete(const std::string& key)
{
    boost::redis::request request;
    request.push("DEL", key);

    boost::redis::response<int> response;
    sys::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::cancel_after(std::chrono::seconds(3), net::redirect_error(net::use_awaitable, ec))
    );

    if (ec)
    {
        LOG_ERROR("Redis DEL failed for key {}: {}", key, ec.message());
        co_return false;
    }
    
    auto& result = std::get<0>(response);

    if (!result.has_value())
    {
        LOG_ERROR("Redis DEL response adapter error for key {}", key);
        co_return false;
    }

    co_return result.value() > 0;
}

net::awaitable<bool> RedisRepository::Publish(const std::string& channel, const std::string& payload)
{
    boost::redis::request request;
    request.push("PUBLISH", channel, payload);

    boost::redis::generic_response response;
    sys::error_code ec;
    
    co_await m_connection->async_exec(request, response, net::redirect_error(net::use_awaitable, ec));

    if (ec)
    {
        LOG_WARN("Failed to publish to channel {}: {}", channel, ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<bool> RedisRepository::WarmupHexagonOwners(
    const std::vector<HexagonOwnerRecord>& records)
{
    if (records.empty())
    {
        co_return true;
    }

    boost::redis::request request;
    // HSET kazan:hex:owners <h3_index> <owner_id> ...
    request.push("HSET", "kazan:hex:owners");
    for (const auto& record : records)
    {
        request.push(std::to_string(record.h3_index));
        request.push(std::to_string(record.owner_user_id));
    }

    for (const auto& record : records)
    {
        std::string zset_key = "hex:leaderboard:" + std::to_string(record.h3_index);
        request.push("ZADD", zset_key, std::to_string(record.top_score), std::to_string(record.owner_user_id));
    }

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_ERROR("Failed to warmup hexagon owners in Redis: {}", ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<std::optional<uint64_t>> RedisRepository::GetHexagonOwner(uint64_t h3_index)
{
    boost::redis::request request;
    request.push("HGET", "kazan:hex:owners", std::to_string(h3_index));

    boost::redis::response<std::optional<std::string>> response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis HGET failed for hex {}: {}", h3_index, ec.message());
        co_return std::nullopt;
    }

    const auto& opt_val = std::get<0>(response).value();

    if (!opt_val.has_value() || opt_val->empty())
    {
        co_return std::nullopt;
    }

    const std::string& owner_str = opt_val.value();
    uint64_t owner_id = 0;
    
    auto [ptr, parse_ec] = std::from_chars(
        owner_str.data(), 
        owner_str.data() + owner_str.size(), 
        owner_id
    );

    if (parse_ec != std::errc{})
    {
        co_return std::nullopt;
    }

    co_return owner_id;
}

net::awaitable<std::vector<std::pair<uint64_t, uint64_t>>> RedisRepository::GetHexagonOwnersBatch(
    const std::vector<uint64_t>& h3_indices)
{
    if (h3_indices.empty())
    {
        co_return std::vector<std::pair<uint64_t, uint64_t>>{};
    }

    // HMGET kazan:hex:owners idx1 idx2 idx3 ...
    boost::redis::request request;
    request.push("HMGET", "kazan:hex:owners");
    for (uint64_t idx : h3_indices)
    {
        request.push(std::to_string(idx));
    }

    boost::redis::response<std::vector<std::optional<std::string>>> response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis HMGET failed for batch hexagons: {}", ec.message());
        co_return std::vector<std::pair<uint64_t, uint64_t>>{};
    }

    const auto& values = std::get<0>(response).value();
    std::vector<std::pair<uint64_t, uint64_t>> result;
    result.reserve(values.size());

    for (size_t i = 0; i < h3_indices.size() && i < values.size(); ++i)
    {
        if (values[i].has_value() && !values[i]->empty())
        {
            const std::string& owner_str = values[i].value();

            uint64_t owner_id = 0;
            auto [ptr, parse_ec] = std::from_chars(
                owner_str.data(), 
                owner_str.data() + owner_str.size(), 
                owner_id
            );

            if (parse_ec == std::errc{})
            {
                result.emplace_back(h3_indices[i], owner_id);
            }
        }
    }

    co_return result;
}

net::awaitable<bool> RedisRepository::PushTrackPoint(uint64_t run_id, const GpsPoint& point)
{
    boost::redis::request request;
    const std::string track_key = fmt::format("run:track:{}", run_id);

    const std::string point_str = fmt::format("{},{},{},{}", 
        point.latitude, point.longitude, point.speed, point.timestamp
    );

    request.push("RPUSH", track_key, point_str);
    request.push("EXPIRE", track_key, "86400"); // TTL 24h

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis RPUSH failed for run {}: {}", run_id, ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<bool> RedisRepository::UpdateActiveRunStats(
    uint64_t run_id, 
    double delta_meters, 
    int32_t delta_points)
{
    boost::redis::request request;
    const std::string active_key = fmt::format("run:active:{}", run_id);

    request.push("HINCRBYFLOAT", active_key, "distance_meters", std::to_string(delta_meters));
    if (delta_points > 0)
    {
        request.push("HINCRBY", active_key, "uram_points", std::to_string(delta_points));
    }

    request.push("EXPIRE", active_key, "86400"); // TTL 24h

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis HINCRBY failed for run {}: {}", run_id, ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<bool> RedisRepository::ClearActiveRun(uint64_t run_id)
{
    boost::redis::request request;
    const std::string active_key = fmt::format("run:active:{}", run_id);
    const std::string track_key = fmt::format("run:track:{}", run_id);

    request.push("DEL", active_key, track_key);

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis DEL failed for run {}: {}", run_id, ec.message());
        co_return false;
    }

    co_return true;
}

} // namespace repository