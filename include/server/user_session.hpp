#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <unordered_set>

#include "envelope.pb.h"
#include "common.hpp"

namespace server::session
{

/**
 * @brief Abstract interface representing an active user connection session
 */
class UserSession
{
public:
    virtual ~UserSession() = default;

    virtual net::any_io_executor GetExecutor() = 0;
    virtual uint64_t GetId() const noexcept = 0;
    virtual net::awaitable<void> SendAsync(std::string bytes) = 0;
    virtual void Close() = 0;

    virtual net::awaitable<void> AsyncSendProtobuf(const runuram::proto::Envelope& envelope) = 0;

    // res=6
    virtual void UpdateSubscribedZones(const std::vector<uint64_t>& zones) = 0;

    virtual const std::unordered_set<uint64_t>& GetSubscribedZones() const noexcept = 0;
};

} // namespace server::session