#include "services/run_service.hpp"

#include <fmt/format.h>
#include <chrono>
#include <sstream>

#include "server/user_session.hpp"
#include "logger/logger.hpp"

namespace service
{

RunService::RunService(
    std::shared_ptr<repository::IRedisRepository> redis_repository,
    std::shared_ptr<repository::IUserRepository> user_repository,
    std::shared_ptr<repository::IHexagonRepository> hex_repository,
    std::shared_ptr<repository::IRunRepository> run_repository)
    : m_redis_repository(std::move(redis_repository))
    , m_user_repository(std::move(user_repository))
    , m_hex_repository(std::move(hex_repository))
    , m_run_repository(std::move(run_repository))
{}

net::awaitable<void> RunService::HandleStartRun(
    const events::StartRunRequest& /*request*/,
    server::session::UserSession& session)
{
    runuram::proto::Envelope envelope;
    auto* response = envelope.mutable_start_run_response();

    uint64_t user_id = session.GetId();
    if (user_id == 0)
    {
        response->set_status(common::STATUS_UNAUTHORIZED);
        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    uint64_t run_id = co_await m_run_repository->CreateRun(user_id);
    if (run_id == 0)
    {
        response->set_status(common::STATUS_SERVER_ERROR);
        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    session.SetActiveRunId(run_id);
    co_await m_redis_repository->ClearActiveRun(run_id);

    response->set_status(common::STATUS_OK);
    response->set_run_id(run_id);

    LOG_INFO("User {} started run {}", user_id, run_id);
    co_await session.AsyncSendProtobuf(envelope);
}

net::awaitable<void> RunService::HandleProcessLocationBatch(
    const telemetry::LocationBatch& batch,
    server::session::UserSession& session)
{
    runuram::proto::Envelope envelope;
    auto* ack = envelope.mutable_location_frame_ack();

    uint64_t user_id = session.GetId();
    uint64_t run_id = batch.run_id() != 0 ? batch.run_id() : session.GetActiveRunId();

    if (user_id == 0 || run_id == 0)
    {
        ack->set_status(common::STATUS_UNAUTHORIZED);
        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    double batch_distance = 0.0;
    uint64_t last_h3_cell = 0;
    std::optional<repository::GpsPoint> prev_point;

    for (const auto& pt : batch.points())
    {
        if (pt.latitude() < -90.0 || pt.latitude() > 90.0 ||
            pt.longitude() < -180.0 || pt.longitude() > 180.0 ||
            pt.speed() < 0.0 || pt.speed() > 25.0) // filter invalid speed (>90 km/h)
        {
            continue;
        }

        repository::GpsPoint current_pt{
            .latitude = pt.latitude(),
            .longitude = pt.longitude(),
            .speed = pt.speed(),
            .timestamp = pt.timestamp()
        };

        if (prev_point.has_value())
        {
            double dist = utils::h3::GetDistanceMeters(
                prev_point->latitude, prev_point->longitude,
                current_pt.latitude, current_pt.longitude
            );

            // Filter GPS teleport jumps (> 500m per point)
            if (dist > 0.0 && dist < 500.0)
            {
                batch_distance += dist;
            }
        }

        last_h3_cell = utils::h3::PointToH3(current_pt.latitude, current_pt.longitude, 9);
        co_await m_redis_repository->PushTrackPoint(run_id, current_pt);
        prev_point = current_pt;
    }

    int32_t batch_points = static_cast<int32_t>(batch_distance / 10.0);
    co_await m_redis_repository->UpdateActiveRunStats(run_id, batch_distance, batch_points);

    ack->set_status(common::STATUS_OK);
    if (last_h3_cell != 0)
    {
        ack->mutable_current_cell()->set_index(last_h3_cell);
        ack->mutable_current_cell()->set_resolution(9);
    }
    ack->set_total_distance_meters(batch_distance);

    co_await session.AsyncSendProtobuf(envelope);
}

net::awaitable<void> RunService::HandleFinishRun(
    const events::FinishRunRequest& /*request*/,
    server::session::UserSession& session)
{
    runuram::proto::Envelope envelope;
    auto* response = envelope.mutable_finish_run_response();

    uint64_t user_id = session.GetId();
    uint64_t run_id = session.GetActiveRunId();

    if (user_id == 0 || run_id == 0)
    {
        response->set_status(common::STATUS_INVALID_DATA);
        co_await session.AsyncSendProtobuf(envelope);
        co_return;
    }

    auto points = co_await m_redis_repository->GetTrackPoints(run_id);

    struct HexProgress
    {
        int32_t points{0};
        double distance{0.0};
    };
    std::unordered_map<uint64_t, HexProgress> hex_deltas;

    double total_distance = 0.0;
    int64_t duration_seconds = 0;

    if (!points.empty())
    {
        duration_seconds = std::max<int64_t>(1, points.back().timestamp - points.front().timestamp);

        for (size_t i = 0; i < points.size(); ++i)
        {
            uint64_t hex = utils::h3::PointToH3(points[i].latitude, points[i].longitude, 9);
            double seg_dist = 0.0;
            if (i > 0)
            {
                seg_dist = utils::h3::GetDistanceMeters(
                    points[i - 1].latitude, points[i - 1].longitude,
                    points[i].latitude, points[i].longitude
                );

                if (seg_dist < 500.0)
                {
                    total_distance += seg_dist;
                }
                else
                {
                    seg_dist = 0.0;
                }
            }

            int32_t seg_points = static_cast<int32_t>(seg_dist / 10.0);
            if (seg_points == 0 && seg_dist > 0.0)
            {
                seg_points = 1;
            }

            hex_deltas[hex].points += seg_points;
            hex_deltas[hex].distance += seg_dist;
        }
    }

    uint32_t hexes_captured_count = 0;
    uint32_t total_score = 0;

    auto user_opt = co_await m_user_repository->FindById(user_id);

    // Apply score and captures across touched hexagons
    for (const auto& [hex_idx, delta] : hex_deltas)
    {
        total_score += delta.points;

        auto progress = co_await m_hex_repository->AddUramPoints(
            hex_idx, 
            user_id, 
            delta.points, 
            delta.distance
        );

        co_await m_redis_repository->UpdateHexagonScoreInCache(hex_idx, user_id, progress.new_top_score);

        if (progress.is_captured)
        {
            hexes_captured_count++;
            co_await m_redis_repository->SetHexagonOwner(hex_idx, user_id);

            // Broadcast capture event via Redis Pub/Sub to parent zone (res 6)
            uint64_t parent_zone = utils::h3::GetParent(hex_idx, 6);
            std::string channel = fmt::format("kazan:zone:{:x}", parent_zone);

            runuram::proto::Envelope capture_envelope;
            auto* capture_evt = capture_envelope.mutable_hexagon_capture_event();
            capture_evt->set_h3_index(hex_idx);
            capture_evt->set_new_owner_id(user_id);
            if (user_opt.has_value())
            {
                capture_evt->set_new_owner_name(user_opt->username);
                capture_evt->set_new_owner_color_hex(user_opt->player_color_hex);
            }
            capture_evt->set_prev_owner_id(progress.prev_owner_id.value_or(0));
            capture_evt->set_score_at_capture(progress.new_top_score);
            capture_evt->set_timestamp(
                std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );

            std::string capture_payload;
            if (capture_envelope.SerializeToString(&capture_payload))
            {
                co_await m_redis_repository->Publish(channel, capture_payload);
            }
        }
    }

    // Build track GeoJSON
    std::ostringstream geojson_stream;
    geojson_stream << "{\"type\":\"LineString\",\"coordinates\":[";
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (i > 0) geojson_stream << ",";
        geojson_stream << fmt::format("[{:.6f},{:.6f}]", points[i].longitude, points[i].latitude);
    }
    geojson_stream << "]}";

    repository::RunSummary summary{
        .run_id = run_id,
        .user_id = user_id,
        .distance_meters = total_distance,
        .duration_seconds = duration_seconds,
        .uram_points_earned = static_cast<int32_t>(total_score),
        .hexagons_captured = static_cast<int32_t>(hexes_captured_count),
        .started_at = "",
        .finished_at = "",
        .encoded_track_geojson = geojson_stream.str()
    };

    co_await m_run_repository->SaveFinishedRun(summary);
    co_await m_redis_repository->ClearActiveRun(run_id);
    session.SetActiveRunId(0);

    response->set_status(common::STATUS_OK);
    response->set_run_id(run_id);
    response->set_total_distance_meters(total_distance);
    response->set_duration_seconds(static_cast<uint32_t>(duration_seconds));
    response->set_hexes_claimed_count(hexes_captured_count);
    response->set_total_score(total_score);

    LOG_INFO("User {} finished run {}: dist={:.1f}m, captured={}", user_id, run_id, total_distance, hexes_captured_count);
    co_await session.AsyncSendProtobuf(envelope);
}

} // namespace service