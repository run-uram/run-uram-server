#pragma once

#include "repository/iuser_repository.hpp"
#include "context/work_context.hpp"

namespace repository
{

class PostgresUserRepository : public IUserRepository
{
private:
    context::WorkContext& m_work_context;
    
public:
    explicit PostgresUserRepository(context::WorkContext& work_context);
    ~PostgresUserRepository() override = default;

    net::awaitable<std::optional<User>> FindByUsername(const std::string& username) override;
    net::awaitable<bool> CreateUser(const User& user) override;
};

} // namespace repository