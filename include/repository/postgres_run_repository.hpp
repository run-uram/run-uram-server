#pragma once 

#include "repository/irun_repository.hpp"
#include "context/work_context.hpp"

namespace repository
{

class PostgresRunRepository : public IRunRepository
{
private:
    context::WorkContext& m_work_context;

public:
    explicit PostgresRunRepository(context::WorkContext& work_context);
    ~PostgresRunRepository() override = default;

    net::awaitable<uint64_t> CreateRun(uint64_t user_id) override;
    net::awaitable<bool> SaveFinishedRun(const RunSummary& summary) override;
    net::awaitable<std::vector<RunSummary>> GetUserRuns(uint64_t user_id, size_t limit) override;
    net::awaitable<std::optional<RunSummary>> GetRunById(uint64_t run_id) override;
};

} // namespace repository