#pragma once 

#include "controllers/protobuf/ihandler.hpp"

#include "services/run_service.hpp"

namespace controller
{

class LocationBatchHandler : public IProtobufHandler
{
private:
    std::shared_ptr<service::RunService> m_run_service;
     
public:
    explicit LocationBatchHandler(std::shared_ptr<service::RunService> run_service)
    : m_run_service(std::move(run_service))
    {}

    net::awaitable<void> Execute(
        const runuram::proto::Envelope& envelope, 
        server::session::UserSession& session
    ) override;
};

} // namespace controller