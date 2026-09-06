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
    
    std::vector<std::string> hset_args;
    hset_args.reserve(records.size() * 2);
    for (const auto& record : records)
    {
        hset_args.push_back(std::to_string(record.h3_index));
        hset_args.push_back(std::to_string(record.owner_user_id));
    }
    request.push_range("HSET", "kazan:hex:owners", hset_args);

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

    boost::redis::request request;
    
    std::vector<std::string> fields;
    fields.reserve(h3_indices.size());
    for (uint64_t idx : h3_indices)
    {
        fields.push_back(std::to_string(idx));
    }
    request.push_range("HMGET", "kazan:hex:owners", fields);

    boost::redis::generic_response response;
    boost::system::error_code ec;
    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );
    if (ec || !response.has_value())
    {
        LOG_WARN("Redis HMGET failed for batch hexagons: {}", ec ? ec.message() : "empty response");
        co_return std::vector<std::pair<uint64_t, uint64_t>>{};
    }
    std::vector<std::pair<uint64_t, uint64_t>> result;
    result.reserve(h3_indices.size());
    const auto& nodes = response.value();

    size_t node_idx = 0;
    if (!nodes.empty() && nodes[0].data_type == boost::redis::resp3::type::array)
    {
        node_idx = 1;
    }
    for (size_t i = 0; i < h3_indices.size() && node_idx < nodes.size(); ++i, ++node_idx)
    {
        const auto& node = nodes[node_idx];
        if (node.data_type == boost::redis::resp3::type::blob_string ||
            node.data_type == boost::redis::resp3::type::simple_string)
        {
            std::string_view owner_str = node.value;
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

net::awaitable<bool> RedisRepository::SetHexagonOwner(uint64_t h3_index, uint64_t owner_id)
{
    boost::redis::request request;
    request.push("HSET", "kazan:hex:owners", std::to_string(h3_index), std::to_string(owner_id));

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis HSET failed for hex {}: {}", h3_index, ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<std::vector<HexagonLeaderboardCacheItem>> RedisRepository::GetHexagonLeaderboardFromCache(
    uint64_t h3_index, 
    size_t limit)
{
    if (limit == 0)
    {
        co_return std::vector<HexagonLeaderboardCacheItem>{};
    }

    boost::redis::request request;
    std::string key = "hex:leaderboard:" + std::to_string(h3_index);
    request.push("ZREVRANGE", key, "0", std::to_string(limit - 1), "WITHSCORES");

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec || !response.has_value())
    {
        co_return std::vector<HexagonLeaderboardCacheItem>{};
    }

    std::vector<HexagonLeaderboardCacheItem> result;
    const auto& nodes = response.value();
    size_t node_idx = 0;
    if (!nodes.empty() && nodes[0].data_type == boost::redis::resp3::type::array)
    {
        node_idx = 1;
    }

    while (node_idx + 1 < nodes.size())
    {
        const auto& member_node = nodes[node_idx++];
        const auto& score_node = nodes[node_idx++];

        if ((member_node.data_type == boost::redis::resp3::type::blob_string ||
             member_node.data_type == boost::redis::resp3::type::simple_string) &&
            (score_node.data_type == boost::redis::resp3::type::blob_string ||
             score_node.data_type == boost::redis::resp3::type::simple_string ||
             score_node.data_type == boost::redis::resp3::type::number ||
             score_node.data_type == boost::redis::resp3::type::doublean))
        {
            uint64_t uid = 0;
            std::string_view uid_str = member_node.value;
            auto [ptr1, ec1] = std::from_chars(uid_str.data(), uid_str.data() + uid_str.size(), uid);

            int32_t score = 0;
            std::string_view score_str = score_node.value;
            auto [ptr2, ec2] = std::from_chars(score_str.data(), score_str.data() + score_str.size(), score);

            if (ec1 == std::errc{} && ec2 == std::errc{})
            {
                result.push_back(HexagonLeaderboardCacheItem{
                    .user_id = uid,
                    .uram_points = score
                });
            }
        }
    }

    co_return result;
}

net::awaitable<bool> RedisRepository::SetHexagonLeaderboard(
    uint64_t h3_index, 
    const std::vector<HexagonLeaderboardEntry>& entries, 
    std::chrono::seconds ttl)
{
    if (entries.empty())
    {
        co_return true;
    }

    boost::redis::request request;
    std::string key = "hex:leaderboard:" + std::to_string(h3_index);
    request.push("DEL", key);

    std::vector<std::string> zadd_args;
    zadd_args.reserve(entries.size() * 2);
    for (const auto& entry : entries)
    {
        zadd_args.push_back(std::to_string(entry.uram_points));
        zadd_args.push_back(std::to_string(entry.user_id));
    }
    request.push_range("ZADD", key, zadd_args);
    request.push("EXPIRE", key, std::to_string(ttl.count()));

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis SetHexagonLeaderboard failed for hex {}: {}", h3_index, ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<bool> RedisRepository::UpdateHexagonScoreInCache(
    uint64_t h3_index, 
    uint64_t user_id, 
    int32_t uram_points)
{
    boost::redis::request request;
    std::string key = "hex:leaderboard:" + std::to_string(h3_index);
    request.push("ZADD", key, std::to_string(uram_points), std::to_string(user_id));
    request.push("EXPIRE", key, "604800"); // 7 days

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis UpdateHexagonScoreInCache failed for hex {}: {}", h3_index, ec.message());
        co_return false;
    }

    co_return true;
}

net::awaitable<std::optional<UserProfileCache>> RedisRepository::GetUserProfileCache(uint64_t user_id)
{
    boost::redis::request request;
    std::string key = "user:profile:" + std::to_string(user_id);
    request.push("HGETALL", key);

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec || !response.has_value())
    {
        co_return std::nullopt;
    }

    const auto& nodes = response.value();
    if (nodes.empty())
    {
        co_return std::nullopt;
    }

    size_t node_idx = 0;
    if (nodes[0].data_type == boost::redis::resp3::type::map ||
        nodes[0].data_type == boost::redis::resp3::type::array)
    {
        node_idx = 1;
    }

    if (node_idx >= nodes.size())
    {
        co_return std::nullopt;
    }

    UserProfileCache profile;
    profile.user_id = user_id;

    while (node_idx + 1 < nodes.size())
    {
        std::string_view field = nodes[node_idx++].value;
        std::string_view val = nodes[node_idx++].value;

        if (field == "username" || field == "nickname")
        {
            profile.username = std::string(val);
        }
        else if (field == "avatar_url")
        {
            profile.avatar_url = std::string(val);
        }
        else if (field == "player_color_hex" || field == "team_color")
        {
            profile.player_color_hex = std::string(val);
        }
        else if (field == "team_id" || field == "faction_id")
        {
            uint64_t tid = 0;
            std::from_chars(val.data(), val.data() + val.size(), tid);
            profile.team_id = tid;
        }
        else if (field == "team_color_hex")
        {
            profile.team_color_hex = std::string(val);
        }
    }

    if (profile.username.empty())
    {
        co_return std::nullopt;
    }

    co_return profile;
}

net::awaitable<std::unordered_map<uint64_t, UserProfileCache>> RedisRepository::GetUserProfilesBatch(
    const std::vector<uint64_t>& user_ids)
{
    if (user_ids.empty())
    {
        co_return std::unordered_map<uint64_t, UserProfileCache>{};
    }

    std::vector<uint64_t> unique_ids = user_ids;
    std::sort(unique_ids.begin(), unique_ids.end());
    unique_ids.erase(std::unique(unique_ids.begin(), unique_ids.end()), unique_ids.end());

    boost::redis::request request;
    for (uint64_t uid : unique_ids)
    {
        request.push("HGETALL", "user:profile:" + std::to_string(uid));
    }

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec || !response.has_value())
    {
        co_return std::unordered_map<uint64_t, UserProfileCache>{};
    }

    std::unordered_map<uint64_t, UserProfileCache> result;
    const auto& nodes = response.value();
    size_t node_idx = 0;

    for (size_t i = 0; i < unique_ids.size() && node_idx < nodes.size(); ++i)
    {
        uint64_t uid = unique_ids[i];
        if (nodes[node_idx].data_type == boost::redis::resp3::type::map ||
            nodes[node_idx].data_type == boost::redis::resp3::type::array)
        {
            size_t map_elements = nodes[node_idx].aggregate_size;
            node_idx++;

            UserProfileCache profile;
            profile.user_id = uid;

            for (size_t elem = 0; elem < map_elements && node_idx + 1 < nodes.size(); elem += 2)
            {
                std::string_view field = nodes[node_idx++].value;
                std::string_view val = nodes[node_idx++].value;

                if (field == "username" || field == "nickname")
                {
                    profile.username = std::string(val);
                }
                else if (field == "avatar_url")
                {
                    profile.avatar_url = std::string(val);
                }
                else if (field == "player_color_hex" || field == "team_color")
                {
                    profile.player_color_hex = std::string(val);
                }
                else if (field == "team_id" || field == "faction_id")
                {
                    uint64_t tid = 0;
                    std::from_chars(val.data(), val.data() + val.size(), tid);
                    profile.team_id = tid;
                }
                else if (field == "team_color_hex")
                {
                    profile.team_color_hex = std::string(val);
                }
            }

            if (!profile.username.empty())
            {
                result[uid] = std::move(profile);
            }
        }
        else
        {
            node_idx++;
        }
    }

    co_return result;
}

net::awaitable<bool> RedisRepository::SetUserProfileCache(
    uint64_t user_id, 
    const UserProfileCache& profile, 
    std::chrono::seconds ttl)
{
    boost::redis::request request;
    std::string key = "user:profile:" + std::to_string(user_id);

    std::vector<std::string> args = {
        "username", profile.username,
        "avatar_url", profile.avatar_url,
        "player_color_hex", profile.player_color_hex,
        "team_id", std::to_string(profile.team_id),
        "team_color_hex", profile.team_color_hex
    };

    request.push_range("HSET", key, args);
    request.push("EXPIRE", key, std::to_string(ttl.count()));

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec)
    {
        LOG_WARN("Redis SetUserProfileCache failed for user {}: {}", user_id, ec.message());
        co_return false;
    }

    co_return true;
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

net::awaitable<std::vector<GpsPoint>> RedisRepository::GetTrackPoints(uint64_t run_id)
{
    boost::redis::request request;
    std::string track_key = fmt::format("run:track:{}", run_id);
    request.push("LRANGE", track_key, "0", "-1");

    boost::redis::generic_response response;
    boost::system::error_code ec;

    co_await m_connection->async_exec(
        request, 
        response, 
        net::redirect_error(net::use_awaitable, ec)
    );

    if (ec || !response.has_value())
    {
        co_return std::vector<GpsPoint>{};
    }

    std::vector<GpsPoint> points;
    const auto& nodes = response.value();
    size_t node_idx = 0;
    if (!nodes.empty() && nodes[0].data_type == boost::redis::resp3::type::array)
    {
        node_idx = 1;
    }

    for (; node_idx < nodes.size(); ++node_idx)
    {
        const auto& node = nodes[node_idx];
        if (node.data_type == boost::redis::resp3::type::blob_string ||
            node.data_type == boost::redis::resp3::type::simple_string)
        {
            std::string_view point_str = node.value;
            auto comma1 = point_str.find(',');
            if (comma1 == std::string_view::npos) continue;
            auto comma2 = point_str.find(',', comma1 + 1);
            if (comma2 == std::string_view::npos) continue;
            auto comma3 = point_str.find(',', comma2 + 1);
            if (comma3 == std::string_view::npos) continue;

            GpsPoint pt;
            try
            {
                pt.latitude = std::stod(std::string(point_str.substr(0, comma1)));
                pt.longitude = std::stod(std::string(point_str.substr(comma1 + 1, comma2 - comma1 - 1)));
                pt.speed = std::stod(std::string(point_str.substr(comma2 + 1, comma3 - comma2 - 1)));
                pt.timestamp = std::stoll(std::string(point_str.substr(comma3 + 1)));
                points.push_back(pt);
            }
            catch (...)
            {
                // skip malformed point
            }
        }
    }

    co_return points;
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