#pragma once 

#include <unordered_map>
#include <memory>
#include <iostream>

#include "server/user_session.hpp"
#include "envelope.pb.h"
#include "logger/logger.hpp"

namespace controller
{

/**
 * @brief Routes incoming Protobuf Envelope payloads
 */
class ProtobufController
{
private:
    using CaseType = runuram::proto::Envelope::PayloadCase;
    using HandlerFunc = std::function<net::awaitable<void>(
        const runuram::proto::Envelope&, 
        server::session::UserSession&
    )>;
    std::unordered_map<CaseType, HandlerFunc> m_handlers;

public:
    ProtobufController() = default;
    ~ProtobufController() = default;

    ProtobufController(const ProtobufController&) = delete;
    ProtobufController& operator=(const ProtobufController&) = delete;

    /**
     * @brief Registers a command handler for Protobuf payload case
     */
    void RegisterHandler(CaseType payload_case, HandlerFunc handler)
    {
        m_handlers[payload_case] = std::move(handler);
    }

    /**
     * @brief Dispatches an incoming Envelope to its matching registered handler
     */
    net::awaitable<void> Dispatch(
        const runuram::proto::Envelope& envelope, 
        server::session::UserSession& session) const 
    {
        auto it = m_handlers.find(envelope.payload_case());
        if (it != m_handlers.end())
        {
            co_await it->second(envelope, session);
        }
        else
        {
            LOG_WARN("Unhandled protobuf payload case: {}", static_cast<int>(envelope.payload_case()));
        }
    }
};

} // namespace controller