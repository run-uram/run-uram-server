#include "repository/postgres_run_repository.hpp"
#include "repository/connection_pool.hpp"
#include "logger/logger.hpp"

namespace repository
{

PostgresRunRepository::PostgresRunRepository(context::WorkContext& work_context)
    : m_work_context(work_context)
{}

net::awaitable<uint64_t> PostgresRunRepository::CreateRun(uint64_t user_id)
{
    co_return co_await m_work_context.AsyncPost([user_id]() -> uint64_t {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            auto result = txn.exec(
                "INSERT INTO runs (user_id, started_at, status) "
                "VALUES ($1, NOW(), 'in_progress') RETURNING id",
                pqxx::params{static_cast<int64_t>(user_id)}
            );
            txn.commit();

            if (result.empty())
            {
                return 0;
            }

            return static_cast<uint64_t>(result[0]["id"].as<int64_t>());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to initialize run for user {}: {}", user_id, e.what());
            return 0;
        }
    });
}

net::awaitable<bool> PostgresRunRepository::SaveFinishedRun(const RunSummary& summary)
{
    co_return co_await m_work_context.AsyncPost([summary]() -> bool {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            txn.exec(
                "UPDATE runs SET "
                "  distance_meters = $1, "
                "  duration_seconds = $2, "
                "  uram_points_earned = $3, "
                "  hexagons_captured = $4, "
                "  track_geojson = $5, "
                "  finished_at = NOW(), "
                "  status = 'completed' "
                "WHERE id = $6",
                pqxx::params{
                    summary.distance_meters,
                    summary.duration_seconds,
                    summary.uram_points_earned,
                    summary.hexagons_captured,
                    summary.encoded_track_geojson,
                    static_cast<int64_t>(summary.run_id)
                }
            );

            txn.exec(
                "INSERT INTO user_stats (user_id, total_distance_meters, total_duration_seconds, total_runs, total_uram_points, current_held_hexagons, updated_at) "
                "VALUES ($1, $2, $3, 1, $4, 0, NOW()) "
                "ON CONFLICT (user_id) DO UPDATE SET "
                "  total_distance_meters = user_stats.total_distance_meters + EXCLUDED.total_distance_meters, "
                "  total_duration_seconds = user_stats.total_duration_seconds + EXCLUDED.total_duration_seconds, "
                "  total_runs = user_stats.total_runs + 1, "
                "  total_uram_points = user_stats.total_uram_points + EXCLUDED.total_uram_points, "
                "  updated_at = NOW()",
                pqxx::params{
                    static_cast<int64_t>(summary.user_id),
                    summary.distance_meters,
                    summary.duration_seconds,
                    summary.uram_points_earned
                }
            );

            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to save finished run {}: {}", summary.run_id, e.what());
            return false;
        }
    });
}

net::awaitable<std::vector<RunSummary>> PostgresRunRepository::GetUserRuns(uint64_t user_id, size_t limit)
{
    co_return co_await m_work_context.AsyncPost([user_id, limit]() -> std::vector<RunSummary> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT id, user_id, distance_meters, duration_seconds, "
                "       uram_points_earned, hexagons_captured, started_at::text, finished_at::text "
                "FROM runs "
                "WHERE user_id = $1 AND status = 'completed' "
                "ORDER BY started_at DESC LIMIT $2",
                pqxx::params{
                    static_cast<int64_t>(user_id),
                    static_cast<int64_t>(limit)
                }
            );

            txn.commit();

            std::vector<RunSummary> runs;
            runs.reserve(result.size());

            for (const auto& row : result)
            {
                runs.push_back({
                    .run_id = static_cast<uint64_t>(row["id"].as<int64_t>()),
                    .user_id = static_cast<uint64_t>(row["user_id"].as<int64_t>()),
                    .distance_meters = row["distance_meters"].as<double>(),
                    .duration_seconds = row["duration_seconds"].as<int64_t>(),
                    .uram_points_earned = row["uram_points_earned"].as<int32_t>(),
                    .hexagons_captured = row["hexagons_captured"].as<int32_t>(),
                    .started_at = row["started_at"].as<std::string>(),
                    .finished_at = row["finished_at"].as<std::string>(),
                    .encoded_track_geojson = ""
                });
            }

            return runs;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to get runs for user {}: {}", user_id, e.what());
            return {};
        }
    });
}

net::awaitable<std::optional<RunSummary>> PostgresRunRepository::GetRunById(uint64_t run_id)
{
    co_return co_await m_work_context.AsyncPost([run_id]() -> std::optional<RunSummary> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT id, user_id, distance_meters, duration_seconds, "
                "       uram_points_earned, hexagons_captured, started_at::text, "
                "       COALESCE(finished_at::text, '') AS finished_at, "
                "       COALESCE(track_geojson, '') AS track_geojson "
                "FROM runs WHERE id = $1",
                pqxx::params{static_cast<int64_t>(run_id)}
            );

            txn.commit();

            if (result.empty())
            {
                return std::nullopt;
            }

            const auto& row = result[0];
            return RunSummary{
                .run_id = static_cast<uint64_t>(row["id"].as<int64_t>()),
                .user_id = static_cast<uint64_t>(row["user_id"].as<int64_t>()),
                .distance_meters = row["distance_meters"].as<double>(),
                .duration_seconds = row["duration_seconds"].as<int64_t>(),
                .uram_points_earned = row["uram_points_earned"].as<int32_t>(),
                .hexagons_captured = row["hexagons_captured"].as<int32_t>(),
                .started_at = row["started_at"].as<std::string>(),
                .finished_at = row["finished_at"].as<std::string>(),
                .encoded_track_geojson = row["track_geojson"].as<std::string>()
            };
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to get run by id {}: {}", run_id, e.what());
            return std::nullopt;
        }
    });
}

} // namespace repository