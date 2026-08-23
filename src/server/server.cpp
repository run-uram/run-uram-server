#include "server/server.hpp"

namespace server
{

static std::string GetEnvOr(const char* name, const std::string& fallback)
{
    const char* val = std::getenv(name);
    return val ? val : fallback;
}

Server::Server(unsigned int io_threads, unsigned int work_threads)
    : m_context(io_threads, work_threads)
    , m_user_repository(std::make_shared<repository::PostgresUserRepository>(m_context.GetWorkContext()))
    , m_hexagon_repository(std::make_shared<repository::PostgresHexagonRepository>(m_context.GetWorkContext()))
    , m_team_repository(std::make_shared<repository::PostgresTeamRepository>(m_context.GetWorkContext()))
    , m_run_repository(std::make_shared<repository::PostgresRunRepository>(m_context.GetWorkContext()))
    , m_redis_repository(std::make_shared<repository::RedisRepository>(m_context.GetLowerLayourIOContext().get_executor()))
    , m_auth_service(std::make_shared<service::AuthService>(m_user_repository, m_redis_repository, m_context.GetWorkContext()))
    , m_controller(
        std::make_shared<controller::HttpController>(m_auth_service),
        std::make_shared<controller::ProtobufController>()
      )
    , m_session_manager(std::make_shared<session::SessionManager>())
{

    LOG_INFO("--- {} ---", config::ServerConfig::GetServerName());
    LOG_INFO("Version: {}", config::ServerConfig::GetServerVersion());

        // Initialize PostgreSQL connection pool
    std::string connection_str = "host=" + GetEnvOr("DB_HOST", "localhost") +
                                " port=" + GetEnvOr("DB_PORT", "5432") +
                                " dbname=" + GetEnvOr("DB_NAME", "runuram_db") +
                                " user=" + GetEnvOr("DB_USER", "runuram_user") +
                                " password=" + GetEnvOr("DB_PASSWORD", "secret_db_password");
    
    repository::ConnectionPool::Get().Init(connection_str, 5);

    const auto& server_config = config::ServerConfig::Get();

    if (server_config.IsSSLEnabled())
    {
        m_context.EnableSSL();
    }

    net::co_spawn(
        m_context.GetLowerLayourIOContext(),
        WarmupCache(),
        net::detached
    );

    const auto& network_config = server_config.GetNetworkSettings();

    if (m_context.GetSSLContext())
    {
        net::co_spawn(
            m_context.GetLowerLayourIOContext(),
            ListenHttps(network_config.https_port),
            net::detached
        );
    }
    else
    {
        net::co_spawn(
            m_context.GetLowerLayourIOContext(),
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
            net::steady_timer timer(executor, std::chrono::milliseconds(100));
            co_await timer.async_wait(net::use_awaitable);
            continue;
        }

        net::co_spawn(
            net::make_strand(executor),
            [socket = std::move(socket), 
             http_controller  = m_controller.GetHttpController(), 
             proto_controller = m_controller.GetProtobufController(), 
             session_manager  = m_session_manager,
             auth_service     = m_auth_service]() mutable -> net::awaitable<void>
            {
                auto session = std::make_shared<HttpSession<beast::tcp_stream>>(
                    beast::tcp_stream(std::move(socket)),
                    std::move(http_controller),
                    std::move(proto_controller),
                    std::move(session_manager),
                    std::move(auth_service)
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
            net::steady_timer timer(executor, std::chrono::milliseconds(100));
            co_await timer.async_wait(net::use_awaitable);
            continue;
        }

        net::co_spawn(
            net::make_strand(executor),
            [socket = std::move(socket),
             ssl_context = &ssl_context_ptr->GetSSLContext(),
             http_controller = m_controller.GetHttpController(),
             proto_controller = m_controller.GetProtobufController(),
             session_manager = m_session_manager,
             auth_service = m_auth_service]() mutable -> net::awaitable<void>
            {
                auto session = std::make_shared<HttpSession<beast::ssl_stream<beast::tcp_stream>>>(
                    beast::ssl_stream<beast::tcp_stream>(beast::tcp_stream(std::move(socket)), *ssl_context),
                    std::move(http_controller),
                    std::move(proto_controller),
                    std::move(session_manager),
                    std::move(auth_service)
                );

                co_await session->Run();
            },
            net::detached
        );
    }
}

net::awaitable<void> Server::WarmupCache()
{
    LOG_INFO("Starting Redis cache warmup...");

    auto hex_records = co_await m_hexagon_repository->GetAllActiveHexagons();
    
    if (!hex_records.empty())
    {
        bool ok = co_await m_redis_repository->WarmupHexagonOwners(hex_records);
        if (ok)
        {
            LOG_INFO("Warmup completed: {} hexagons loaded into Redis", hex_records.size());
        }
        else
        {
            LOG_ERROR("Redis warmup failed!");
        }
    }
    else
    {
        LOG_INFO("Warmup skipped: database has no active hexagons");
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