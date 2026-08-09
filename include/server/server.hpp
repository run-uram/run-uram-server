#pragma once

#include <memory>
#include <thread>

#include "common.hpp"

#include "context/context.hpp"
#include "controllers/controller.hpp"
#include "server/session_manager.hpp"

namespace server
{

class Server
{
private:
    context::Context m_context;
    controller::Controller m_controller;
    std::shared_ptr<session::SessionManager> m_session_manager;

private:
    net::awaitable<void> ListenHttp(unsigned short port);
    net::awaitable<void> ListenHttps(unsigned short port);

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