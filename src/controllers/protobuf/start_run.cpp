#include "controllers/protobuf/start_run.hpp"

namespace controller
{

net::awaitable<void> StartRunHandler::Execute(
    const runuram::proto::Envelope& envelope, 
    server::session::UserSession& session
)
{
    // const auto& request = envelope.start_run_request();

    co_return;
}

} // namespace controller