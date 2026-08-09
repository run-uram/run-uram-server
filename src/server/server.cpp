#include "server/server.hpp"

#include "config/config.hpp"
#include "logger/logger.hpp"
#include "server/http_session.hpp"

namespace server
{

Server::Server(unsigned int io_threads, unsigned int work_threads)
    : m_context(io_threads, work_threads)
    , m_controller()
    , m_session_manager(std::make_shared<session::SessionManager>())
{
    const auto& server_config = config::ServerConfig::Get();

    LOG_INFO("--- {} ---", config::ServerConfig::GetServerName());
    LOG_INFO("Version: {}", config::ServerConfig::GetServerVersion());
    
    if (server_config.IsSSLEnabled())
    {
        m_context.EnableSSL();
    }

    const auto& network_config = server_config.GetNetworkSettings();

    if (m_context.GetSSLContext())
    {
        net::co_spawn(
            m_context.GetIOContext().GetIOContext(),
            ListenHttps(network_config.https_port),
            net::detached
        );
    }
    else
    {
        net::co_spawn(
            m_context.GetIOContext().GetIOContext(),
            ListenHttp(network_config.http_port),
            net::detached
        );
    }
}

net::awaitable<void> Server::ListenHttp(unsigned short port)
{
    auto executor = co_await net::this_coro::executor;
    const auto& address_str = config::ServerConfig::Get().GetNetworkSettings().address;
    tcp::endpoint endpoint(net::ip::make_address(address_str), port);

    tcp::acceptor acceptor(executor);
    sys::error_code ec;

    acceptor.open(endpoint.protocol(), ec);
    if (ec)
    {
        LOG_ERROR("HTTP Acceptor open failed on port {}: {}", port, ec.message());
        co_return;
    }

    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    acceptor.bind(endpoint, ec);
    if (ec)
    {
        LOG_ERROR("HTTP Acceptor bind failed on port {}: {}", port, ec.message());
        co_return;
    }

    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        LOG_ERROR("HTTP Acceptor listen failed on port {}: {}", port, ec.message());
        co_return;
    }

    LOG_INFO("HTTP server listening on {}:{}", address_str, port);

    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(net::redirect_error(net::use_awaitable, ec));
        if (ec)
        {
            LOG_WARN("HTTP accept error: {}", ec.message());
            continue;
        }

        net::co_spawn(
            net::make_strand(executor),
            [s = std::move(socket), http_ctrl = m_controller.GetHttpController(), proto_ctrl = m_controller.GetProtobufController(), mgr = m_session_manager]() mutable -> net::awaitable<void> {
                auto session = std::make_shared<HttpSession<beast::tcp_stream>>(
                    beast::tcp_stream(std::move(s)),
                    std::move(http_ctrl),
                    std::move(proto_ctrl),
                    std::move(mgr)
                );
                co_await session->Run();
            },
            net::detached
        );
    }
}

net::awaitable<void> Server::ListenHttps(unsigned short port)
{
    auto* ssl_context_ptr = m_context.GetSSLContext();
    if (!ssl_context_ptr)
    {
        LOG_ERROR("SSL Context not initialized, cannot start HTTPS listener");
        co_return;
    }

    auto executor = co_await net::this_coro::executor;
    const auto& address_str = config::ServerConfig::Get().GetNetworkSettings().address;
    tcp::endpoint endpoint(net::ip::make_address(address_str), port);

    tcp::acceptor acceptor(executor);
    sys::error_code ec;

    acceptor.open(endpoint.protocol(), ec);
    if (ec)
    {
        LOG_ERROR("HTTPS Acceptor open failed on port {}: {}", port, ec.message());
        co_return;
    }

    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    acceptor.bind(endpoint, ec);
    if (ec)
    {
        LOG_ERROR("HTTPS Acceptor bind failed on port {}: {}", port, ec.message());
        co_return;
    }

    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        LOG_ERROR("HTTPS Acceptor listen failed on port {}: {}", port, ec.message());
        co_return;
    }

    LOG_INFO("HTTPS server listening on {}:{}", address_str, port);

    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(net::redirect_error(net::use_awaitable, ec));
        if (ec)
        {
            LOG_WARN("HTTPS accept error: {}", ec.message());
            continue;
        }

        net::co_spawn(
            net::make_strand(executor),
            [s = std::move(socket), ssl_ctx = &ssl_context_ptr->GetSSLContext(), http_ctrl = m_controller.GetHttpController(), proto_ctrl = m_controller.GetProtobufController(), mgr = m_session_manager]() mutable -> net::awaitable<void> {
                auto session = std::make_shared<HttpSession<beast::ssl_stream<beast::tcp_stream>>>(
                    beast::ssl_stream<beast::tcp_stream>(beast::tcp_stream(std::move(s)), *ssl_ctx),
                    std::move(http_ctrl),
                    std::move(proto_ctrl),
                    std::move(mgr)
                );
                co_await session->Run();
            },
            net::detached
        );
    }
}

void Server::Run()
{
    m_context.Run();
}

void Server::Stop()
{
    m_context.Stop();
}

} // namespace server