#pragma once

#include <memory>
#include <thread>

#include "common.hpp"

#include "config/config.hpp"
#include "logger/logger.hpp"

#include "context/context.hpp"

#include "server/session_manager.hpp"
#include "server/http_session.hpp"

#include "repository/postgres_user_repository.hpp"
#include "repository/redis_repository.hpp"
#include "repository/connection_pool.hpp"

#include "services/auth_service.hpp"
#include "services/run_service.hpp"

#include "controllers/controller.hpp"
#include "controllers/protobuf/start_run.hpp"
#include "controllers/protobuf/location_batch.hpp"

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
    std::shared_ptr<repository::IUserRepository> m_user_repository;
    std::shared_ptr<repository::IRedisRepository> m_redis_repository;
    std::shared_ptr<service::AuthService> m_auth_service;
    controller::Controller m_controller;
    std::shared_ptr<session::SessionManager> m_session_manager;

private:
    net::awaitable<void> ListenHttp(unsigned short port);
    net::awaitable<void> ListenHttps(unsigned short port);

    void InitProtobufRouter();

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