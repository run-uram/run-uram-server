#pragma once

#include <memory>
#include <boost/asio.hpp>

#include "repository/iuser_repository.hpp"
#include "repository/iredis_repository.hpp"
#include "repository/ihexagon_repository.hpp"
#include "repository/irun_repository.hpp"

#include "utils/uram_h3.hpp"

#include "common.pb.h"
#include "telemetry.pb.h"
#include "events.pb.h"
#include "gamemap.pb.h"
#include "envelope.pb.h"

namespace net = boost::asio;

namespace server::session
{
class UserSession;
} // namespace server::session

namespace service
{

class RunService
{
private:
    std::shared_ptr<repository::IRedisRepository> m_redis_repository;
    std::shared_ptr<repository::IUserRepository> m_user_repository;
    std::shared_ptr<repository::IHexagonRepository> m_hex_repository;
    std::shared_ptr<repository::IRunRepository> m_run_repository;

public:
    RunService(
        std::shared_ptr<repository::IRedisRepository> redis_repository,
        std::shared_ptr<repository::IUserRepository> user_repository,
        std::shared_ptr<repository::IHexagonRepository> hex_repository,
        std::shared_ptr<repository::IRunRepository> run_repository
    );

    net::awaitable<void> HandleStartRun(
        const events::StartRunRequest& request,
        server::session::UserSession& session
    );

    net::awaitable<void> HandleProcessLocationBatch(
        const telemetry::LocationBatch& batch,
        server::session::UserSession& session
    );

    net::awaitable<void> HandleFinishRun(
        const events::FinishRunRequest& request,
        server::session::UserSession& session
    );
};

} // namespace service