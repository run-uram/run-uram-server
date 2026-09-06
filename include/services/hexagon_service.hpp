#pragma once

#include <boost/asio.hpp>
#include <memory>
#include "gamemap.pb.h"
#include "envelope.pb.h"

#include "repository/ihexagon_repository.hpp"
#include "repository/iredis_repository.hpp"
#include "repository/iuser_repository.hpp"

namespace net = boost::asio;

namespace server::session
{
class UserSession;
} // namespace server::session

namespace service
{

class HexagonService
{
private:
    std::shared_ptr<repository::IHexagonRepository> m_hex_repository;
    std::shared_ptr<repository::IRedisRepository> m_redis_repository;
    std::shared_ptr<repository::IUserRepository> m_user_repository;

public:
    HexagonService(
        std::shared_ptr<repository::IHexagonRepository> hex_repository,
        std::shared_ptr<repository::IRedisRepository> redis_repository,
        std::shared_ptr<repository::IUserRepository> user_repository = nullptr
    );

    net::awaitable<void> HandleViewportSubscription(
        const gamemap::SubscribeViewportRequest& request,
        server::session::UserSession& session
    );

    net::awaitable<void> HandleGetHexagonDetails(
        const gamemap::GetHexagonDetailsRequest& request,
        server::session::UserSession& session
    );
};

} // namespace service