#pragma once

#include <boost/redis/connection.hpp>
#include <boost/redis/request.hpp>
#include <boost/redis/response.hpp>

#include "server/session_manager.hpp"

namespace repository
{

class RedisPubSubWorker : public std::enable_shared_from_this<RedisPubSubWorker>
{
private:
    std::shared_ptr<boost::redis::connection> m_connection;
    std::shared_ptr<server::session::SessionManager> m_session_manager;
    std::string m_pattern{"kazan:zone:*"};

private:
    // "kazan:zone:86118e667ffffff" -> "86118e667ffffff"
    std::string ExtractH3Zone(const std::string& channel) const;

public:
    RedisPubSubWorker(
        net::any_io_executor executor,
        std::shared_ptr<server::session::SessionManager> session_manager
    );

    net::awaitable<void> Start();

    void Stop();
};

} // namespace repository