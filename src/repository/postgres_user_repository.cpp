#include "repository/postgres_user_repository.hpp"
#include "repository/connection_pool.hpp"
#include "logger/logger.hpp"

namespace repository
{

PostgresUserRepository::PostgresUserRepository(context::WorkContext& work_context)
    : m_work_context(work_context)
{}

net::awaitable<std::optional<User>> PostgresUserRepository::FindByUsername(const std::string& username)
{
    co_return co_await m_work_context.AsyncPost([username]() -> std::optional<User> {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);
            
            auto result = txn.exec(
                "SELECT id, username, password_hash FROM users WHERE username = $1",
                pqxx::params{username}
            );

            if (result.empty())
            {
                return std::nullopt;
            }

            auto row = result[0];
            return User{
                row["id"].as<int>(),
                row["username"].as<std::string>(),
                row["password_hash"].as<std::string>()
            };
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to find user by username '{}': {}", username, e.what());
            return std::nullopt;
        }
    });
}

net::awaitable<bool> PostgresUserRepository::CreateUser(const User& user)
{
    co_return co_await m_work_context.AsyncPost([user]() -> bool {
        try
        {
            auto connection_ptr = ConnectionPool::Get().GetConnection();
            pqxx::work txn(*connection_ptr);

            txn.exec(
                "INSERT INTO users (username, password_hash) VALUES ($1, $2)",
                pqxx::params{user.username, user.password_hash}
            );
            txn.commit();
            return true;
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to create user '{}': {}", user.username, e.what());
            return false;
        }
    });
}

} // namespace repository
