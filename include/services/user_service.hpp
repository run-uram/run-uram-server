#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "user.pb.h"
#include "envelope.pb.h"

#include "repository/iuser_repository.hpp"
#include "repository/iredis_repository.hpp"

namespace net = boost::asio;

namespace server::session
{
class UserSession;
} // namespace server::session

namespace service
{

class UserService
{
private:
    std::shared_ptr<repository::IUserRepository> m_user_repository;
    std::shared_ptr<repository::IRedisRepository> m_redis_repository;

public:
    explicit UserService(
        std::shared_ptr<repository::IUserRepository> user_repository,
        std::shared_ptr<repository::IRedisRepository> redis_repository = nullptr
    );

    net::awaitable<void> HandleGetUserProfile(
        const user::GetUserProfileRequest& request,
        server::session::UserSession& session
    );

};

} // namespace service