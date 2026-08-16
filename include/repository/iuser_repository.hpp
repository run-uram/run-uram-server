#pragma once

#include <boost/asio.hpp>

#include <optional>
#include <string>

namespace net = boost::asio;

namespace repository
{

struct User
{
    int id;
    std::string username;
    std::string password_hash;
};

class IUserRepository
{
public:
    virtual ~IUserRepository() = default;

    virtual net::awaitable<std::optional<User>> FindByUsername(const std::string& username) = 0;
    virtual net::awaitable<bool> CreateUser(const User& user) = 0;
};

} // namespace repository