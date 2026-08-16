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

    net::awaitable<bool> Set(const std::string& key, const std::string& value, std::chrono::seconds ttl) override;
    net::awaitable<std::optional<std::string>> Get(const std::string& key) override;
    net::awaitable<bool> Delete(const std::string& key) override;
};

} // namespace repository
