#pragma once 

#include <deque>
#include <string>
#include <memory>

#include "common.hpp"
#include "logger/logger.hpp"

#include "server/session_manager.hpp"
#include "server/user_session.hpp"

#include "controllers/protobuf/protobuf_controller.hpp"

#include "envelope.pb.h"

namespace server
{

template <typename Stream>
class WebSocketSession 
    : public session::UserSession
    , public std::enable_shared_from_this<WebSocketSession<Stream>>
{
private:
    using WriteChannel = net::experimental::channel<void(sys::error_code, std::string)>;

    websocket::stream<Stream> m_ws;
    beast::flat_buffer m_buffer;

    WriteChannel m_write_channel;

    std::shared_ptr<controller::ProtobufController> m_controller;
    std::shared_ptr<session::SessionManager> m_session_manager;

    uint64_t m_user_id{0};
    uint64_t m_active_run_id{0};
    std::unordered_set<uint64_t> m_subscribed_zones;

private:
    net::awaitable<void> ReadLoop()
    {
        try
        {
            for (;;)
            {
                m_buffer.clear();
                co_await m_ws.async_read(m_buffer, net::use_awaitable);

                runuram::proto::Envelope envelope;
                auto bytes = m_buffer.data();
                if (!envelope.ParseFromArray(bytes.data(), bytes.size()))
                {
                    LOG_DEBUG("Failed to parse Protobuf payload from user {}", m_user_id);
                    continue;
                }

                co_await m_controller->Dispatch(envelope, *this);
            }
        }
        catch (const std::exception& ex)
        {
            LOG_DEBUG("WebSocket read loop finished for user {}: {}", m_user_id, ex.what());
        }

        m_write_channel.close();
    }

    net::awaitable<void> WriteLoop()
    {
        for (;;)
        {
            auto [ec, msg] = co_await m_write_channel.async_receive(net::as_tuple(net::use_awaitable));
            if (ec)
            {
                break;
            }

            sys::error_code write_ec;
            co_await m_ws.async_write(net::buffer(msg), net::redirect_error(net::use_awaitable, write_ec));

            if (write_ec)
            {
                LOG_DEBUG("Write error for user {}: {}", m_user_id, write_ec.message());
                break;
            }
        }
    }

public:
    explicit WebSocketSession(
        Stream&& stream,
        std::shared_ptr<controller::ProtobufController> controller,
        std::shared_ptr<session::SessionManager> session_manager,
        uint64_t user_id = 0)
        : m_ws(std::move(stream))
        , m_write_channel(m_ws.get_executor(), 128)
        , m_controller(std::move(controller))
        , m_session_manager(std::move(session_manager))
        , m_user_id(user_id)
    {}

    ~WebSocketSession() override
    {
        if (m_session_manager)
        {
            m_session_manager->RemoveSession(m_user_id, this);
        }
    }

    void Close() override
    {
        m_write_channel.close();
        beast::error_code ec;
        beast::get_lowest_layer(m_ws).socket().close(ec);
    }

    template <typename Body, typename Allocator>
    net::awaitable<void> Run(http::request<Body, http::basic_fields<Allocator>> request)
    {
        auto self = this->shared_from_this();

        beast::get_lowest_layer(m_ws).expires_never();

        m_ws.binary(true);
        websocket::stream_base::timeout opt{
            std::chrono::seconds(30),       // handshake_timeout
            websocket::stream_base::none(), // idle_timeout disabled (prevents 30s disconnect)
            false                           // keep_alive_pings
        };
        m_ws.set_option(opt);

        sys::error_code ec;
        co_await m_ws.async_accept(request, net::redirect_error(net::use_awaitable, ec));

        if (ec)
        {
            LOG_DEBUG("WebSocket accept failed for user {}: {}", m_user_id, ec.message());
            co_return;
        }

        if (m_session_manager)
        {
            m_session_manager->AddSession(m_user_id, self);
        }

        LOG_DEBUG("User {} connected via WebSocket", m_user_id);

        auto executor = co_await net::this_coro::executor;

        net::co_spawn(executor, [self]() { return self->WriteLoop(); }, net::detached);

        co_await ReadLoop();
    }

    net::any_io_executor GetExecutor() override
    {
        return m_ws.get_executor();
    }

    uint64_t GetId() const noexcept override
    {
        return m_user_id;
    }

    net::awaitable<void> SendAsync(std::string message) override
    {
        if (!m_write_channel.try_send(sys::error_code{}, std::move(message)))
        {
            LOG_WARN("Write channel overflow for user {}, dropping packet", m_user_id);
        }
        co_return;
    }

    net::awaitable<void> AsyncSendProtobuf(const runuram::proto::Envelope& envelope) override
    {
        std::string payload;
        if (!envelope.SerializeToString(&payload))
        {
            LOG_ERROR("Failed to serialize Protobuf Envelope for user {}", m_user_id);
            co_return;
        }
        co_await SendAsync(std::move(payload));
    }

    void UpdateSubscribedZones(const std::vector<uint64_t>& zones) override
    {
        m_subscribed_zones.clear();
        m_subscribed_zones.insert(zones.begin(), zones.end());

        if (m_session_manager)
        {
            m_session_manager->UpdateViewportSubscription(this->shared_from_this(), zones);
        }
    }

    const std::unordered_set<uint64_t>& GetSubscribedZones() const noexcept override
    {
        return m_subscribed_zones;
    }

    uint64_t GetActiveRunId() const noexcept override { return m_active_run_id; }
    void SetActiveRunId(uint64_t run_id) noexcept override { m_active_run_id = run_id; }
};

} // namespace server