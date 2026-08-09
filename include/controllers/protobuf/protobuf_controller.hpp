#pragma once 

#include <unordered_map>
#include <memory>
#include <iostream>

#include "controllers/protobuf/ihandler.hpp"

namespace controller
{

/**
 * @brief Routes incoming Protobuf Envelope payloads
 */
class ProtobufController
{
private:
    using CaseType = runuram::proto::Envelope::PayloadCase;
    std::unordered_map<CaseType, std::unique_ptr<IProtobufHandler>> m_handlers;

public:
    ProtobufController() = default;
    ~ProtobufController() = default;

    ProtobufController(const ProtobufController&) = delete;
    ProtobufController& operator=(const ProtobufController&) = delete;

    /**
     * @brief Registers a command handler for Protobuf payload case
     */
    template <typename T, typename... Args>
    void RegisterHandler(CaseType payload_case, Args&&... args)
    {
        static_assert(std::is_base_of_v<IProtobufHandler, T>, 
                      "T must derive from IProtobufHandler");
        m_handlers[payload_case] = std::make_unique<T>(std::forward<Args>(args)...);
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
            co_await it->second->Execute(envelope, session);
        }
        else
        {
            std::cerr << "Unhandled payload case: " << static_cast<int>(envelope.payload_case()) << "\n";
        }
    }
};

} // namespace controller