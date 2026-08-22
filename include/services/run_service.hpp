#pragma once

#include "repository/iuser_repository.hpp"
#include "repository/iredis_repository.hpp"

#include "utils/uram_h3.hpp"

#include "common.pb.h"
#include "telemetry.pb.h"
#include "events.pb.h"
#include "map.pb.h"

namespace service
{

class RunService
{
private:
    std::shared_ptr<repository::IRedisRepository> m_redis_repository;
    std::shared_ptr<repository::IUserRepository> m_user_repository;

public:
    RunService(
        std::shared_ptr<repository::IRedisRepository> redis_repository,
        std::shared_ptr<repository::IUserRepository> user_repository
    )
    : m_redis_repository(std::move(redis_repository))
    , m_user_repository(std::move(user_repository))
    {}

    net::awaitable<uint64_t> StartRun(uint64_t user_id);

    net::awaitable<telemetry::LocationBatchAck> ProcessLocationBatch(
        uint64_t user_id, const telemetry::LocationBatch& batch);

    net::awaitable<events::FinishRunResponse> FinishRun(uint64_t user_id, uint64_t run_id);

    net::awaitable<map::GetHexagonsInAreaResponse> GetHexagonsInArea(
        map::GetHexagonsInAreaRequest hexagons_request);
};

} // namespace service