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

        boost::redis::generic_response response;
        co_await m_connection->async_exec(request, response, net::use_awaitable);

        for (;;)
        {
            auto [ec, n] = co_await m_connection->async_receive(net::as_tuple(net::use_awaitable));
            if (ec)
            {
                break;
            }

            const auto& nodes = response.value();
            // [0]="pmessage", [1]=pattern, [2]=channel, [3]=payload
            if (nodes.size() >= 4 && nodes[0].value == "pmessage")
            {
                std::string channel_name = nodes[2].value;
                std::string payload = nodes[3].value;
                std::string h3_zone = ExtractH3Zone(channel_name);
                if (m_session_manager)
                {
                    m_session_manager->BroadcastToZone(h3_zone, payload);
                }
            }
            response.value().clear();
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Redis Pub/Sub Worker {}", e.what());
    }
}

void RedisPubSubWorker::Stop()
{
    m_connection->cancel();
}

std::string RedisPubSubWorker::ExtractH3Zone(const std::string& channel) const
{
    const std::string prefix = "kazan:zone:";
    if (channel.rfind(prefix, 0) == 0)
    {
        return channel.substr(prefix.length());
    }
    
    return channel;
}

} // namespace repository