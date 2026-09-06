#include "services/hexagon_service.hpp"

#include "server/user_session.hpp"
#include "utils/uram_h3.hpp"
#include "logger/logger.hpp"

namespace service
{

HexagonService::HexagonService(
    std::shared_ptr<repository::IHexagonRepository> hex_repository,
    std::shared_ptr<repository::IRedisRepository> redis_repository,
    std::shared_ptr<repository::IUserRepository> user_repository)
    : m_hex_repository(std::move(hex_repository))
    , m_redis_repository(std::move(redis_repository))
    , m_user_repository(std::move(user_repository))
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
    session.UpdateSubscribedZones(parent_zones);

    // Hot Read from Redis (< 1ms)
    auto owners = co_await m_redis_repository->GetHexagonOwnersBatch(h3_cells);

    std::vector<uint64_t> owner_ids;
    owner_ids.reserve(owners.size());
    for (const auto& [h3_idx, owner_id] : owners)
    {
        if (owner_id != 0)
        {
            owner_ids.push_back(owner_id);
        }
    }

    // Enrich with cached user profiles
    auto profiles = co_await m_redis_repository->GetUserProfilesBatch(owner_ids);

    runuram::proto::Envelope envelope;
    auto* response = envelope.mutable_subscribe_viewport_response();
    response->set_status(common::STATUS_OK);

    for (const auto& [h3_idx, owner_id] : owners)
    {
        auto* hex_state = response->add_hexagons();
        hex_state->set_h3_index(h3_idx);
        hex_state->set_owner_user_id(owner_id);

        if (auto it = profiles.find(owner_id); it != profiles.end())
        {
            hex_state->set_owner_username(it->second.username);
            const auto& color = !it->second.player_color_hex.empty() 
                ? it->second.player_color_hex 
                : it->second.team_color_hex;
            hex_state->set_owner_color_hex(color);
        }
    }

    co_await session.AsyncSendProtobuf(envelope);
}

net::awaitable<void> HexagonService::HandleGetHexagonDetails(
    const gamemap::GetHexagonDetailsRequest& request,
    server::session::UserSession& session)
{
    runuram::proto::Envelope envelope;
    auto* response = envelope.mutable_hexagon_details_response();

    uint64_t h3_idx = request.h3_index();

    // 1. Try Redis ZSET Leaderboard & Owner Hash
    auto cached_leaderboard = co_await m_redis_repository->GetHexagonLeaderboardFromCache(h3_idx, 10);
    auto cached_owner = co_await m_redis_repository->GetHexagonOwner(h3_idx);

    if (!cached_leaderboard.empty())
    {
        response->set_status(common::STATUS_OK);

        std::vector<uint64_t> user_ids;
        user_ids.reserve(cached_leaderboard.size() + 1);
        for (const auto& item : cached_leaderboard)
        {
            user_ids.push_back(item.user_id);
        }
        if (cached_owner.has_value() && *cached_owner != 0)
        {
            user_ids.push_back(*cached_owner);
        }

        auto profiles = co_await m_redis_repository->GetUserProfilesBatch(user_ids);

        // Fallback fetch missing profiles from DB if needed
        if (m_user_repository)
        {
            for (uint64_t uid : user_ids)
            {
                if (!profiles.contains(uid))
                {
                    auto u = co_await m_user_repository->FindById(uid);
                    if (u.has_value())
                    {
                        repository::UserProfileCache p{
                            .user_id = u->id,
                            .username = u->username,
                            .avatar_url = "",
                            .player_color_hex = u->player_color_hex,
                            .team_id = u->team_id.value_or(0),
                            .team_color_hex = ""
                        };
                        co_await m_redis_repository->SetUserProfileCache(uid, p);
                        profiles[uid] = std::move(p);
                    }
                }
            }
        }

        auto* state = response->mutable_state();
        state->set_h3_index(h3_idx);
        uint64_t owner_id = cached_owner.value_or(cached_leaderboard[0].user_id);
        state->set_owner_user_id(owner_id);
        state->set_top_score(cached_leaderboard[0].uram_points);

        if (auto it = profiles.find(owner_id); it != profiles.end())
        {
            state->set_owner_username(it->second.username);
            state->set_owner_color_hex(it->second.player_color_hex);
        }

        for (const auto& entry : cached_leaderboard)
        {
            auto* item = response->add_leaderboard();
            item->set_user_id(entry.user_id);
            item->set_uram_points(entry.uram_points);

            if (auto it = profiles.find(entry.user_id); it != profiles.end())
            {
                item->set_username(it->second.username);
                item->set_player_color_hex(it->second.player_color_hex);
            }
        }

        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    // 2. Cache Miss: Lazy Load from PostgreSQL
    auto hex_entity = co_await m_hex_repository->FindByIndex(h3_idx);
    auto leaderboard = co_await m_hex_repository->GetHexagonLeaderboard(h3_idx, 10);

    response->set_status(common::STATUS_OK);
    
    auto* state = response->mutable_state();
    state->set_h3_index(h3_idx);
    if (hex_entity.has_value())
    {
        state->set_owner_user_id(hex_entity->owner_user_id.value_or(0));
        state->set_top_score(hex_entity->top_score);

        if (hex_entity->owner_user_id.has_value() && *hex_entity->owner_user_id != 0)
        {
            co_await m_redis_repository->SetHexagonOwner(h3_idx, *hex_entity->owner_user_id);
        }
    }

    if (!leaderboard.empty())
    {
        // Populate Redis ZSET with TTL
        co_await m_redis_repository->SetHexagonLeaderboard(h3_idx, leaderboard);

        for (const auto& entry : leaderboard)
        {
            auto* item = response->add_leaderboard();
            item->set_user_id(entry.user_id);
            item->set_username(entry.username);
            item->set_player_color_hex(entry.player_color_hex);
            item->set_uram_points(entry.uram_points);
            item->set_total_distance_meters(entry.total_distance_meters);
            item->set_visits_count(entry.visits_count);

            // Populate user profile cache
            repository::UserProfileCache p{
                .user_id = entry.user_id,
                .username = entry.username,
                .avatar_url = "",
                .player_color_hex = entry.player_color_hex,
                .team_id = 0,
                .team_color_hex = ""
            };
            co_await m_redis_repository->SetUserProfileCache(entry.user_id, p);
        }
    }

    co_await session.AsyncSendProtobuf(envelope);
}

} // namespace service