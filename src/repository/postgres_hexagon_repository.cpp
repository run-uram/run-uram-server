#include "repository/postgres_hexagon_repository.hpp"
#include "repository/connection_pool.hpp"
#include "logger/logger.hpp"

namespace repository
{

PostgresHexagonRepository::PostgresHexagonRepository(context::WorkContext& work_context)
    : m_work_context(work_context)
{}

net::awaitable<HexagonProgressResult> PostgresHexagonRepository::AddUramPoints(
    uint64_t h3_index,
    uint64_t user_id,
    int32_t points_delta,
    double distance_delta_meters)
{
    co_return co_await m_work_context.AsyncPost([=]() -> HexagonProgressResult {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT is_captured, new_owner_id, prev_owner_id, new_top_score "
                "FROM add_uram_points_and_recalc($1, $2, $3, $4)",
                pqxx::params{
                    static_cast<int64_t>(h3_index),
                    static_cast<int64_t>(user_id),
                    points_delta,
                    distance_delta_meters
                }
            );

            txn.commit();

            if (result.empty())
            {
                return {};
            }

            const auto& row = result[0];

            return HexagonProgressResult {
                .is_captured = row["is_captured"].as<bool>(),
                .h3_index = h3_index,
                .new_owner_id = static_cast<uint64_t>(row["new_owner_id"].as<int64_t>()),
                .prev_owner_id = row["prev_owner_id"].is_null() 
                    ? std::nullopt 
                    : std::make_optional(static_cast<uint64_t>(row["prev_owner_id"].as<int64_t>())),
                .new_top_score = row["new_top_score"].as<int32_t>()
            };
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to add Uram Points for hex {}: {}", h3_index, e.what());
            return {};
        }
    });
}

net::awaitable<std::vector<HexagonOwnerRecord>> PostgresHexagonRepository::GetAllActiveHexagons()
{
    co_return co_await m_work_context.AsyncPost([]() -> std::vector<HexagonOwnerRecord> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT h3_index, owner_user_id, top_score "
                "FROM hexagons WHERE owner_user_id IS NOT NULL"
            );
            txn.commit();

            std::vector<HexagonOwnerRecord> records;
            records.reserve(result.size());

            for (const auto& row : result)
            {
                records.push_back({
                    static_cast<uint64_t>(row["h3_index"].as<int64_t>()),
                    static_cast<uint64_t>(row["owner_user_id"].as<int64_t>()),
                    row["top_score"].as<int32_t>()
                });
            }

            return records;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to fetch all active hexagons for warmup: {}", e.what());
            return {};
        }
    });
}

net::awaitable<std::optional<HexagonEntity>> PostgresHexagonRepository::FindByIndex(uint64_t h3_index)
{
    co_return co_await m_work_context.AsyncPost([h3_index]() -> std::optional<HexagonEntity> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT h3_index, owner_user_id, top_score, captured_at::text "
                "FROM hexagons WHERE h3_index = $1",
                pqxx::params{static_cast<int64_t>(h3_index)}
            );

            if (result.empty())
            {
                return std::nullopt;
            }

            const auto& row = result[0];

            HexagonEntity hex;
            hex.h3_index = static_cast<uint64_t>(row["h3_index"].as<int64_t>());
            if (!row["owner_user_id"].is_null())
            {
                hex.owner_user_id = static_cast<uint64_t>(row["owner_user_id"].as<int64_t>());
            }
            hex.top_score = row["top_score"].as<int32_t>();
            hex.captured_at = row["captured_at"].as<std::string>();

            return hex;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to find hexagon {}: {}", h3_index, e.what());
            return std::nullopt;
        }
    });
}

net::awaitable<std::vector<HexagonEntity>> PostgresHexagonRepository::GetHexagonsByIndices(
    const std::vector<uint64_t>& indices)
{
    if (indices.empty())
    {
        co_return std::vector<HexagonEntity>{};
    }

    co_return co_await m_work_context.AsyncPost([indices]() -> std::vector<HexagonEntity> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            std::vector<int64_t> sql_indices;
            sql_indices.reserve(indices.size());
            for (auto idx : indices)
            {
                sql_indices.push_back(static_cast<int64_t>(idx));
            }

            auto result = txn.exec(
                "SELECT h3_index, owner_user_id, top_score, captured_at::text "
                "FROM hexagons WHERE h3_index = ANY($1)",
                pqxx::params{sql_indices}
            );

            std::vector<HexagonEntity> hexagons;
            hexagons.reserve(result.size());

            for (const auto& row : result)
            {
                HexagonEntity hex;
                hex.h3_index = static_cast<uint64_t>(row["h3_index"].as<int64_t>());
                if (!row["owner_user_id"].is_null())
                {
                    hex.owner_user_id = static_cast<uint64_t>(row["owner_user_id"].as<int64_t>());
                }
                hex.top_score = row["top_score"].as<int32_t>();
                hex.captured_at = row["captured_at"].as<std::string>();
                hexagons.push_back(std::move(hex));
            }

            return hexagons;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to get hexagons batch: {}", e.what());
            return {};
        }
    });
}

net::awaitable<std::vector<HexagonLeaderboardEntry>> PostgresHexagonRepository::GetHexagonLeaderboard(
    uint64_t h3_index, 
    size_t limit)
{
    co_return co_await m_work_context.AsyncPost([h3_index, limit]() -> std::vector<HexagonLeaderboardEntry> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT s.user_id, u.username, u.player_color_hex, "
                "       s.uram_points, s.total_distance_meters, s.visits_count, s.last_visited_at::text "
                "FROM hexagon_user_stats s "
                "JOIN users u ON s.user_id = u.id "
                "WHERE s.h3_index = $1 "
                "ORDER BY s.uram_points DESC, s.last_visited_at ASC "
                "LIMIT $2",
                pqxx::params{
                    static_cast<int64_t>(h3_index),
                    static_cast<int64_t>(limit)
                }
            );
            txn.commit();

            std::vector<HexagonLeaderboardEntry> leaderboard;
            leaderboard.reserve(result.size());

            for (const auto& row : result)
            {
                leaderboard.push_back({
                    .user_id = static_cast<uint64_t>(row["user_id"].as<int64_t>()),
                    .username = row["username"].as<std::string>(),
                    .player_color_hex = row["player_color_hex"].as<std::string>(),
                    .uram_points = row["uram_points"].as<int32_t>(),
                    .total_distance_meters = row["total_distance_meters"].as<double>(),
                    .visits_count = row["visits_count"].as<int32_t>(),
                    .last_visited_at = row["last_visited_at"].as<std::string>()
                });
            }

            return leaderboard;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to get leaderboard for hex {}: {}", h3_index, e.what());
            return {};
        }
    });
}

net::awaitable<std::vector<HexagonHistoryEntry>> PostgresHexagonRepository::GetHexagonHistory(
    uint64_t h3_index, 
    size_t limit)
{
    co_return co_await m_work_context.AsyncPost([h3_index, limit]() -> std::vector<HexagonHistoryEntry> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT h.id, h.h3_index, "
                "       h.previous_owner_id, COALESCE(prev_u.username, '') AS prev_username, "
                "       h.new_owner_id, new_u.username AS new_username, "
                "       h.score_at_capture, h.captured_at::text "
                "FROM hexagon_history h "
                "LEFT JOIN users prev_u ON h.previous_owner_id = prev_u.id "
                "JOIN users new_u ON h.new_owner_id = new_u.id "
                "WHERE h.h3_index = $1 "
                "ORDER BY h.captured_at DESC "
                "LIMIT $2",
                pqxx::params{
                    static_cast<int64_t>(h3_index),
                    static_cast<int64_t>(limit)
                }
            );
            txn.commit();

            std::vector<HexagonHistoryEntry> history;
            history.reserve(result.size());

            for (const auto& row : result)
            {
                HexagonHistoryEntry entry;
                entry.id = static_cast<uint64_t>(row["id"].as<int64_t>());
                entry.h3_index = static_cast<uint64_t>(row["h3_index"].as<int64_t>());
                if (!row["previous_owner_id"].is_null())
                {
                    entry.previous_owner_id = static_cast<uint64_t>(row["previous_owner_id"].as<int64_t>());
                }
                entry.previous_owner_name = row["prev_username"].as<std::string>();
                entry.new_owner_id = static_cast<uint64_t>(row["new_owner_id"].as<int64_t>());
                entry.new_owner_name = row["new_username"].as<std::string>();
                entry.score_at_capture = row["score_at_capture"].as<int32_t>();
                entry.captured_at = row["captured_at"].as<std::string>();

                history.push_back(std::move(entry));
            }

            return history;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to get capture history for hex {}: {}", h3_index, e.what());
            return {};
        }
    });
}

net::awaitable<std::vector<HexagonEntity>> PostgresHexagonRepository::GetUserOwnedHexagons(
    uint64_t user_id)
{
    co_return co_await m_work_context.AsyncPost([user_id]() -> std::vector<HexagonEntity> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT h3_index, owner_user_id, top_score, captured_at::text "
                "FROM hexagons "
                "WHERE owner_user_id = $1 "
                "ORDER BY captured_at DESC",
                pqxx::params{static_cast<int64_t>(user_id)}
            );
            txn.commit();

            std::vector<HexagonEntity> hexagons;
            hexagons.reserve(result.size());

            for (const auto& row : result)
            {
                hexagons.push_back({
                    .h3_index = static_cast<uint64_t>(row["h3_index"].as<int64_t>()),
                    .owner_user_id = static_cast<uint64_t>(row["owner_user_id"].as<int64_t>()),
                    .top_score = row["top_score"].as<int32_t>(),
                    .captured_at = row["captured_at"].as<std::string>()
                });
            }

            return hexagons;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to get owned hexagons for user {}: {}", user_id, e.what());
            return {};
        }
    });
}

} // namespace repository