#pragma once 

#include <iostream>

#include "controller/protobuf/ihandler.hpp"

namespace contorller
{

class LocationFrameHandler : public IProtobufHandler
{
public:
    net::awaitable<void> Execute(
        const runuram::proto::Envelope& envelope, 
        server::session::UserSession& session
    ) override
    {
        const auto& frame = envelope.location_frame();

        std::cout << "[LocationFrame] Received location frame from user=" 
                  << session.GetId()
                  << " with " << frame.points_size() << " points\n";

        co_return;
    }
};

} // namespace contorller