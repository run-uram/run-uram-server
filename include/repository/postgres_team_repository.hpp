#pragma once

#include "repository/iteam_repository.hpp"
#include "context/work_context.hpp"

namespace repository {

class PostgresTeamRepository : public ITeamRepository
{
private:
    context::WorkContext& m_work_context;

public:
    explicit PostgresTeamRepository(context::WorkContext& work_context);
    ~PostgresTeamRepository() override = default;

    net::awaitable<std::vector<Team>> GetAllTeams() override;
    net::awaitable<std::optional<Team>> FindById(uint64_t team_id) override;
    net::awaitable<std::optional<Team>> FindByTag(const std::string& tag) override;
    net::awaitable<bool> SetUserTeam(uint64_t user_id, std::optional<uint64_t> team_id) override;
    net::awaitable<std::optional<uint64_t>> CreateTeam(const Team& team) override;
};

} // namespace repository