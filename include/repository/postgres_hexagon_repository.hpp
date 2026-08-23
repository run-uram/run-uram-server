#pragma once

#include "repository/ihexagon_repository.hpp"
#include "context/work_context.hpp"

namespace repository
{

class PostgresHexagonRepository : public IHexagonRepository
{
private:
    context::WorkContext& m_work_context;

public:
    explicit PostgresHexagonRepository(context::WorkContext& work_context);
    ~PostgresHexagonRepository() override = default;

    net::awaitable<HexagonProgressResult> AddUramPoints(
        uint64_t h3_index,
        uint64_t user_id,
        int32_t points_delta,
        double distance_delta_meters
    ) override;

    net::awaitable<std::optional<HexagonEntity>> FindByIndex(uint64_t h3_index) override;

    net::awaitable<std::vector<HexagonEntity>> GetHexagonsByIndices(
        const std::vector<uint64_t>& indices
    ) override;

    net::awaitable<std::vector<HexagonOwnerRecord>> GetAllActiveHexagons() override;

    net::awaitable<std::vector<HexagonLeaderboardEntry>> GetHexagonLeaderboard(
        uint64_t h3_index, 
        size_t limit = 10
    ) override;

    net::awaitable<std::vector<HexagonHistoryEntry>> GetHexagonHistory(
        uint64_t h3_index, 
        size_t limit = 20
    ) override;

    net::awaitable<std::vector<HexagonEntity>> GetUserOwnedHexagons(
        uint64_t user_id
    ) override;
};

} // namespace repository