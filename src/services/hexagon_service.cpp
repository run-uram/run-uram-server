#include "services/hexagon_service.hpp"

#include "server/user_session.hpp"
#include "utils/uram_h3.hpp"
#include "logger/logger.hpp"

namespace service
{

HexagonService::HexagonService(
    std::shared_ptr<repository::IHexagonRepository> hex_repository,
    std::shared_ptr<repository::IRedisRepository> redis_repository)
    : m_hex_repository(std::move(hex_repository))
    , m_redis_repository(std::move(redis_repository))
{}

net::awaitable<void> HexagonService::HandleViewportSubscription(
    const gamemap::SubscribeViewportRequest& request,
    server::session::UserSession& session)
{
    auto h3_cells = utils::h3::GetCellsInBoundingBox(
        request.south_west_lat(), request.south_west_lng(),
        request.north_east_lat(), request.north_east_lng(),
        9 // resolution
    );

    auto parent_zones = utils::h3::GetParentZones(h3_cells, 6);
    session.UpdateSubscribedZones(parent_zones); // typedef uint64_t H3Index;

    auto owners = co_await m_redis_repository->GetHexagonOwnersBatch(h3_cells);

    runuram::proto::Envelope envelope;
    auto* response = envelope.mutable_subscribe_viewport_response();
    response->set_status(common::STATUS_OK);

    for (const auto& [h3_idx, owner_id] : owners)
    {
        auto* hex_state = response->add_hexagons();
        hex_state->set_h3_index(h3_idx);
        hex_state->set_owner_user_id(owner_id);
    }

    co_await session.AsyncSendProtobuf(envelope);
}

net::awaitable<void> HexagonService::HandleGetHexagonDetails(
    const gamemap::GetHexagonDetailsRequest& request,
    server::session::UserSession& session)
{
    runuram::proto::Envelope envelope;
    auto* response = envelope.mutable_hexagon_details_response();

    auto hex_entity = co_await m_hex_repository->FindByIndex(request.h3_index());
    auto leaderboard = co_await m_hex_repository->GetHexagonLeaderboard(request.h3_index(), 10);

    if (!hex_entity.has_value())
    {
        response->set_status(common::STATUS_INVALID_DATA);
        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    response->set_status(common::STATUS_OK);
    
    auto* state = response->mutable_state();
    state->set_h3_index(hex_entity->h3_index);
    state->set_owner_user_id(hex_entity->owner_user_id.value_or(0));
    state->set_top_score(hex_entity->top_score);

    for (const auto& entry : leaderboard)
    {
        auto* item = response->add_leaderboard();
        item->set_user_id(entry.user_id);
        item->set_username(entry.username);
        item->set_player_color_hex(entry.player_color_hex);
        item->set_uram_points(entry.uram_points);
        item->set_total_distance_meters(entry.total_distance_meters);
        item->set_visits_count(entry.visits_count);
    }

    co_await session.AsyncSendProtobuf(envelope);
}

} // namespace service