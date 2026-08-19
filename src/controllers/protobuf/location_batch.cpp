#include "controllers/protobuf/location_batch.hpp"

namespace controller
{

net::awaitable<void> LocationBatchHandler::Execute(
    const runuram::proto::Envelope& envelope, 
    server::session::UserSession& session)
{
    // const auto& batch = envelope.location_batch();
    // uint64_t user_id = session.GetUserId();

    // runuram::proto::LocationBatchAck ack = co_await m_run_service->ProcessLocationBatch(user_id, batch);

    // runuram::proto::Envelope response_envelope;
    // *response_envelope.mutable_location_batch_ack() = std::move(ack);

    // co_await session.Send(response_envelope);

    co_return;
}

} // namespace controller