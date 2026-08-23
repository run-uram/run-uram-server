#pragma once

#include <memory>
#include <thread>

#include "common.hpp"

#include "context/context.hpp"

#include "controllers/controller.hpp"

#include "server/session_manager.hpp"
#include "server/http_session.hpp"

#include "repository/connection_pool.hpp"

#include "repository/postgres_user_repository.hpp"
#include "repository/postgres_hexagon_repository.hpp"
#include "repository/postgres_team_repository.hpp"
#include "repository/postgres_run_repository.hpp"
#include "repository/redis_repository.hpp"

#include "services/auth_service.hpp"

#include "config/config.hpp"
#include "logger/logger.hpp"

namespace repository
{
class IUserRepository;
class IRedisRepository;
}

namespace service
{
class AuthService;
}

namespace server
{

class Server
{
private:
    context::Context m_context;

    // repository
    std::shared_ptr<repository::IUserRepository> m_user_repository;
    std::shared_ptr<repository::IHexagonRepository> m_hexagon_repository;
    std::shared_ptr<repository::ITeamRepository> m_team_repository;
    std::shared_ptr<repository::IRunRepository> m_run_repository;
    std::shared_ptr<repository::RedisRepository> m_redis_repository;

    // service
    std::shared_ptr<service::AuthService> m_auth_service;

    controller::Controller m_controller;
    std::shared_ptr<session::SessionManager> m_session_manager;

private:
    net::awaitable<void> ListenHttp(unsigned short port);
    net::awaitable<void> ListenHttps(unsigned short port);

    net::awaitable<void> WarmupCache();

public:
    explicit Server(
        unsigned int io_threads = std::thread::hardware_concurrency(),
        unsigned int work_threads = std::thread::hardware_concurrency()
    );

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    void Run();
    void Stop();

    context::Context& GetContext() noexcept { return m_context; }
    const context::Context& GetContext() const noexcept { return m_context; }

    controller::Controller& GetController() noexcept { return m_controller; }
    const controller::Controller& GetController() const noexcept { return m_controller; }

    std::shared_ptr<session::SessionManager> GetSessionManager() const { return m_session_manager; }
};

} // namespace server