#include "repository/redis_pub_sub_worker.hpp"
#include "logger/logger.hpp"

namespace repository
{

static std::string GetEnvOr(const char* name, const std::string& fallback)
{
    const char* val = std::getenv(name);
    return val ? val : fallback;
}

RedisPubSubWorker::RedisPubSubWorker(
    net::any_io_executor executor,
    std::shared_ptr<server::session::SessionManager> session_manager)
    : m_connection(std::make_shared<boost::redis::connection>(net::make_strand(executor)))
    , m_session_manager(std::move(session_manager))
{
    boost::redis::config config;
    config.addr.host = GetEnvOr("REDIS_HOST", "127.0.0.1");
    config.addr.port = GetEnvOr("REDIS_PORT", "6379");

    std::string password = GetEnvOr("REDIS_PASSWORD", "");
    if (!password.empty()) {
        config.password = password;
    }

    config.reconnect_wait_interval = std::chrono::seconds(2);
    config.health_check_interval = std::chrono::seconds(10);

    m_connection->async_run(config, net::consign(net::detached, m_connection));
}

net::awaitable<void> RedisPubSubWorker::Start()
{
    auto self = shared_from_this();

    try
    {
        boost::redis::request request;
        request.push("PSUBSCRIBE", m_pattern);

        co_await m_connection->async_exec(request, m_response, net::use_awaitable);

        LOG_INFO("Redis Pub/Sub Worker subscribed to pattern: {}", m_pattern);

        boost::redis::generic_response push_response;

        for (;;)
        {
            push_response.value().clear();

            auto [ec, n] = co_await m_connection->async_receive(
                net::as_tuple(net::use_awaitable)
            );

            if (ec)
            {
                LOG_WARN("Redis Pub/Sub receive error: {}", ec.message());
                break;
            }

            const auto& nodes = m_response.value();
            // [0]="pmessage", [1]=pattern, [2]=channel, [3]=payload
            if (nodes.size() >= 4 && nodes[0].value == "pmessage")
            {
                const std::string& channel_name = nodes[2].value;
                const std::string& payload = nodes[3].value;

                uint64_t h3_zone = ExtractH3Zone(channel_name);

                if (h3_zone != 0 && m_session_manager)
                {
                    m_session_manager->BroadcastToZone(h3_zone, payload);
                }
            }

            m_response.value().clear();
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Redis Pub/Sub Worker exception: {}", e.what());
    }
}

void RedisPubSubWorker::Stop()
{
    m_connection->cancel();
    m_response.value().clear();
}

uint64_t RedisPubSubWorker::ExtractH3Zone(std::string_view channel_name)
{
    // "kazan:zone:<h3_index_hex>" or "kazan:zone:<h3_index_dec>"
    constexpr std::string_view prefix = "kazan:zone:";
    if (!channel_name.starts_with(prefix)) {
        return 0;
    }

    std::string_view zone_view = channel_name.substr(prefix.size());
    uint64_t h3_zone = 0;

    auto [ptr, ec] = std::from_chars(
        zone_view.data(), 
        zone_view.data() + zone_view.size(), 
        h3_zone, 
        16 // base 16 (hex)
    );

    if (ec != std::errc{})
    {
        LOG_WARN("Failed to parse H3 zone from channel: {}", channel_name);
        return 0;
    }

    return h3_zone;
}

} // namespace repository