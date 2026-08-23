#pragma once

#include <boost/asio.hpp>

#include "repository/entities.hpp"

namespace net = boost::asio;

namespace repository
{

class IHexagonRepository
{
public:
    virtual ~IHexagonRepository() = default;

    virtual net::awaitable<HexagonProgressResult> AddUramPoints(
        uint64_t h3_index,
        uint64_t user_id,
        int32_t points_delta,
        double distance_delta_meters
    ) = 0;

    virtual net::awaitable<std::optional<HexagonEntity>> FindByIndex(uint64_t h3_index) = 0;

    virtual net::awaitable<std::vector<HexagonEntity>> GetHexagonsByIndices(
        const std::vector<uint64_t>& indices
    ) = 0;

    virtual net::awaitable<std::vector<HexagonOwnerRecord>> GetAllActiveHexagons() = 0;

    virtual net::awaitable<std::vector<HexagonLeaderboardEntry>> GetHexagonLeaderboard(
        uint64_t h3_index, 
        size_t limit = 10
    ) = 0;

    virtual net::awaitable<std::vector<HexagonHistoryEntry>> GetHexagonHistory(
        uint64_t h3_index, 
        size_t limit = 20
    ) = 0;

    virtual net::awaitable<std::vector<HexagonEntity>> GetUserOwnedHexagons(
        uint64_t user_id
    ) = 0;
};

} // namespace repository