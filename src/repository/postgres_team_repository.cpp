#include "repository/postgres_team_repository.hpp"
#include "repository/connection_pool.hpp"
#include "logger/logger.hpp"

namespace repository
{

namespace
{

Team ParseTeamRow(const pqxx::row& row)
{
    Team team;
    team.id = static_cast<uint64_t>(row["id"].as<int64_t>());
    team.name = row["name"].as<std::string>();
    team.tag = row["tag"].as<std::string>();
    team.color_hex = row["color_hex"].as<std::string>();
    team.created_at = row["created_at"].as<std::string>();
    if (!row["members_count"].is_null())
    {
        team.members_count = row["members_count"].as<int32_t>();
    }
    return team;
}

} // namespace

PostgresTeamRepository::PostgresTeamRepository(context::WorkContext& work_context)
    : m_work_context(work_context)
{}

net::awaitable<std::vector<Team>> PostgresTeamRepository::GetAllTeams()
{
    co_return co_await m_work_context.AsyncPost([]() -> std::vector<Team> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT t.id, t.name, t.tag, t.color_hex, t.created_at::text, "
                "       COUNT(u.id)::int AS members_count "
                "FROM teams t "
                "LEFT JOIN users u ON u.team_id = t.id "
                "GROUP BY t.id "
                "ORDER BY members_count DESC, t.name ASC"
            );
            txn.commit();

            std::vector<Team> teams;
            teams.reserve(result.size());

            for (const auto& row : result)
            {
                teams.push_back(ParseTeamRow(row));
            }

            return teams;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to fetch all teams: {}", e.what());
            return {};
        }
    });
}

net::awaitable<std::optional<Team>> PostgresTeamRepository::FindById(uint64_t team_id)
{
    co_return co_await m_work_context.AsyncPost([team_id]() -> std::optional<Team> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT t.id, t.name, t.tag, t.color_hex, t.created_at::text, "
                "       COUNT(u.id)::int AS members_count "
                "FROM teams t "
                "LEFT JOIN users u ON u.team_id = t.id "
                "WHERE t.id = $1 "
                "GROUP BY t.id",
                pqxx::params{static_cast<int64_t>(team_id)}
            );
            txn.commit();

            if (result.empty())
            {
                return std::nullopt;
            }

            return ParseTeamRow(result[0]);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to find team by id {}: {}", team_id, e.what());
            return std::nullopt;
        }
    });
}

net::awaitable<std::optional<Team>> PostgresTeamRepository::FindByTag(const std::string& tag)
{
    co_return co_await m_work_context.AsyncPost([tag]() -> std::optional<Team> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::read_transaction txn(*connection_ptr);

            auto result = txn.exec(
                "SELECT t.id, t.name, t.tag, t.color_hex, t.created_at::text, "
                "       COUNT(u.id)::int AS members_count "
                "FROM teams t "
                "LEFT JOIN users u ON u.team_id = t.id "
                "WHERE UPPER(t.tag) = UPPER($1) "
                "GROUP BY t.id",
                pqxx::params{tag}
            );
            txn.commit();

            if (result.empty())
            {
                return std::nullopt;
            }

            return ParseTeamRow(result[0]);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to find team by tag '{}': {}", tag, e.what());
            return std::nullopt;
        }
    });
}

net::awaitable<bool> PostgresTeamRepository::SetUserTeam(
    uint64_t user_id, 
    std::optional<uint64_t> team_id)
{
    co_return co_await m_work_context.AsyncPost([user_id, team_id]() -> bool {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            pqxx::params p;
            if (team_id.has_value())
            {
                p.append(static_cast<int64_t>(*team_id));
            }
            else
            {
                p.append(nullptr);
            }
            p.append(static_cast<int64_t>(user_id));

            auto res = txn.exec(
                "UPDATE users SET team_id = $1, updated_at = NOW() WHERE id = $2",
                p
            );

            txn.commit();
            return res.affected_rows() > 0;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to update team for user {}: {}", user_id, e.what());
            return false;
        }
    });
}

net::awaitable<std::optional<uint64_t>> PostgresTeamRepository::CreateTeam(const Team& team)
{
    co_return co_await m_work_context.AsyncPost([team]() -> std::optional<uint64_t> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            auto result = txn.exec(
                "INSERT INTO teams (name, tag, color_hex) "
                "VALUES ($1, $2, $3) RETURNING id",
                pqxx::params{
                    team.name,
                    team.tag,
                    team.color_hex.empty() ? "#ffffff" : team.color_hex
                }
            );

            txn.commit();

            if (result.empty())
            {
                return std::nullopt;
            }

            return static_cast<uint64_t>(result[0]["id"].as<int64_t>());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to create team '{}' (tag: '{}'): {}", team.name, team.tag, e.what());
            return std::nullopt;
        }
    });
}

} // namespace repository