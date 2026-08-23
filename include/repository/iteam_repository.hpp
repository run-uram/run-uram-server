#pragma once 

#include <cstdint>
#include <string>

namespace repository
{

struct Team
{
    uint64_t id{0};
    std::string name;
    std::string tag;
    std::string color_hex{"#ffffff"};
    std::string created_at;
    int32_t members_count{0};
};

class ITeamRepository
{
public:
    virtual ~ITeamRepository() = default;

    virtual net::awaitable<std::vector<Team>> GetAllTeams() = 0;
    virtual net::awaitable<std::optional<Team>> FindById(uint64_t team_id) = 0;
    virtual net::awaitable<std::optional<Team>> FindByTag(const std::string& tag) = 0;
    virtual net::awaitable<bool> SetUserTeam(uint64_t user_id, std::optional<uint64_t> team_id) = 0;
    virtual net::awaitable<std::optional<uint64_t>> CreateTeam(const Team& team) = 0;
};

} // namespace repository