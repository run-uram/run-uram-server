#include "services/run_service.hpp"

namespace service
{

net::awaitable<map::GetHexagonsInAreaResponse> RunService::GetHexagonsInArea(
        map::GetHexagonsInAreaRequest request)
{
    int32_t safe_radius = std::min(request.k_ring_radius(), 10);

    H3Index center_cell = utils::h3::PointToH3(
        request.center_latitude(),
        request.center_longitude(),
        request.h3_resolution()
    );

    std::vector<H3Index> hex_indices = utils::h3::GetHexagonsInRadius(center_cell, safe_radius);

    auto hexagons_infos = co_await m_redis_repository->GetHexagonsState(hex_indices);

    map::GetHexagonsInAreaResponse response;

    for (auto& hexagon : hexagons_infos)
    {
        *response.add_hexagons() = std::move(hexagon);
    }

    response.set_status(common::STATUS_OK);
    co_return response;
}

} // namespace service