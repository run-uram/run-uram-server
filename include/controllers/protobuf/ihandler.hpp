#pragma once 

#include "server/user_session.hpp"

#include "envelope.pb.h"

namespace controller
{

class IProtobufHandler
{
public:
    virtual ~IProtobufHandler() = default;

    virtual net::awaitable<void> Execute(
        const runuram::proto::Envelope& envelope, 
        server::session::UserSession& session
    ) = 0;
};

} // namespace controller
