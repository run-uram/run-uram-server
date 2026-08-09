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
    websocket::stream<Stream> m_ws;
    beast::flat_buffer m_buffer;

    std::deque<std::string> m_write_queue;

    std::shared_ptr<controller::ProtobufController> m_controller;
    std::shared_ptr<session::SessionManager> m_session_manager;

    uint64_t m_user_id{0};
    uint64_t m_active_run_id{0};

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
                if (!envelope.ParseFromArray(m_buffer.data().data(), static_cast<int>(m_buffer.size())))
                {
                    LOG_DEBUG("Failed to parse Protobuf payload from user {}", m_user_id);
                    continue;
                }

                co_await m_controller->Dispatch(envelope, *this);
            }
        }
        catch (const std::exception& ex)
        {
            LOG_DEBUG("WebSocket closed for user {}: {}", m_user_id, ex.what());
        }
    }

    net::awaitable<void> WriteLoop()
    {
        while (!m_write_queue.empty())
        {
            sys::error_code ec;
            
            co_await m_ws.async_write(
                net::buffer(m_write_queue.front()), 
                net::redirect_error(net::use_awaitable, ec)
            );

            if (ec)
            {
                LOG_DEBUG("Write error for user {}: {}", m_user_id, ec.message());
                m_write_queue.clear();
                co_return;
            }

            m_write_queue.pop_front();
        }
    }

public:
    explicit WebSocketSession(
        Stream&& stream,
        std::shared_ptr<controller::ProtobufController> controller,
        std::shared_ptr<session::SessionManager> session_manager,
        uint64_t user_id = 0)
        : m_ws(std::move(stream))
        , m_controller(std::move(controller))
        , m_session_manager(std::move(session_manager))
        , m_user_id(user_id)
    {}

    ~WebSocketSession() override
    {
        if (m_session_manager && m_user_id != 0)
        {
            m_session_manager->RemoveSession(m_user_id);
        }
    }

    template <typename Body, typename Allocator>
    net::awaitable<void> Run(http::request<Body, http::basic_fields<Allocator>> req)
    {
        auto self = this->shared_from_this();

        m_ws.binary(true);
        m_ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

        sys::error_code ec;
        co_await m_ws.async_accept(req, net::redirect_error(net::use_awaitable, ec));

        if (ec)
        {
            LOG_DEBUG("WebSocket accept failed for user {}: {}", m_user_id, ec.message());
            co_return;
        }

        if (m_session_manager && m_user_id != 0)
        {
            m_session_manager->AddSession(m_user_id, self);
        }
        LOG_DEBUG("User {} connected via WebSocket", m_user_id);

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
        bool write_in_progress = !m_write_queue.empty();
        m_write_queue.push_back(std::move(message));

        if (write_in_progress)
        {
            co_return;
        }

        co_await WriteLoop();
    }

    uint64_t GetActiveRunId() const noexcept { return m_active_run_id; }
    void SetActiveRunId(uint64_t run_id) noexcept { m_active_run_id = run_id; }
};

} // namespace server