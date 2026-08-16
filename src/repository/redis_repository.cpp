#include "repository/redis_repository.hpp"
#include "logger/logger.hpp"
#include <cstdlib>
#include <boost/redis/src.hpp>

namespace repository
{

static std::string GetEnvOr(const char* name, const std::string& fallback)
{
    const char* val = std::getenv(name);
    return val ? val : fallback;
}

RedisRepository::RedisRepository(net::any_io_executor executor)
    : m_connection(std::make_shared<boost::redis::connection>(executor))
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
        net::bind_executor(
            m_connection->get_executor(),
            net::cancel_after(std::chrono::seconds(3), net::redirect_error(net::use_awaitable, ec))
        )
    );

    if (ec)
    {
        LOG_ERROR("Redis SET for key {} failed {}", key, ec.message());
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
        net::bind_executor(
            m_connection->get_executor(), 
            net::redirect_error(net::use_awaitable, ec)
        )
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
        net::bind_executor(
            m_connection->get_executor(), 
            net::redirect_error(net::use_awaitable, ec)
        )
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

} // namespace repository