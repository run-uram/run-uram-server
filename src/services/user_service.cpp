#include "services/user_service.hpp"
#include "server/user_session.hpp"
#include "logger/logger.hpp"

namespace service
{

UserService::UserService(std::shared_ptr<repository::IUserRepository> user_repository)
    : m_user_repository(std::move(user_repository))
{}

net::awaitable<void> UserService::HandleGetUserProfile(
    const user::GetUserProfileRequest& request,
    server::session::UserSession& session)
{
    runuram::proto::Envelope envelope;
    auto* response = envelope.mutable_user_profile_response();

    uint64_t target_user_id = request.user_id() != 0 ? request.user_id() : session.GetId();

    if (target_user_id == 0)
    {
        response->set_status(common::STATUS_UNAUTHORIZED);
        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    auto user_data = co_await m_user_repository->FindById(target_user_id);
    auto user_stats = co_await m_user_repository->GetUserStats(target_user_id);

    if (!user_data.has_value())
    {
        response->set_status(common::STATUS_INVALID_DATA);
        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    response->set_status(common::STATUS_OK);
    response->set_user_id(user_data->id);
    response->set_username(user_data->username);
    response->set_email(user_data->email);
    response->set_player_color_hex(user_data->player_color_hex);
    response->set_team_id(user_data->team_id.value_or(0));

    if (user_stats.has_value())
    {
        response->set_total_distance_meters(user_stats->total_distance_meters);
        response->set_total_duration_seconds(user_stats->total_duration_seconds);
        response->set_total_runs(user_stats->total_runs);
        response->set_total_uram_points(user_stats->total_uram_points);
        response->set_current_held_hexagons(user_stats->current_held_hexagons);
    }

    co_await session.AsyncSendProtobuf(envelope);
}

} // namespace service