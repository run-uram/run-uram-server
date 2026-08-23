#pragma once

#include <boost/asio.hpp>

#include "repository/run_models.hpp"

namespace net = boost::asio;

namespace repository
{

class IRunRepository
{
public:
    virtual ~IRunRepository() = default;

    virtual net::awaitable<uint64_t> CreateRun(uint64_t user_id) = 0;
    virtual net::awaitable<bool> SaveFinishedRun(const RunSummary& summary) = 0;
    virtual net::awaitable<std::vector<RunSummary>> GetUserRuns(uint64_t user_id, size_t limit = 20) = 0;
    virtual net::awaitable<std::optional<RunSummary>> GetRunById(uint64_t run_id) = 0;
};

} // namespace repository