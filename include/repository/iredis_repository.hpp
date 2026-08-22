#pragma once

#include <boost/asio.hpp>
#include <string>
#include <optional>
#include <chrono>

#include "map.pb.h"

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

    virtual net::awaitable<std::vector<map::HexagonInfo>> GetHexagonsState(
        const std::vector<uint64_t>& h3_indices
    ) = 0;
};

} // namespace repository
