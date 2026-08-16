#pragma once

#include "common.hpp"

#include "services/auth_service.hpp"

namespace controller
{

class HttpController
{
private:
    std::shared_ptr<service::AuthService> m_auth_service;

public:
    explicit HttpController(std::shared_ptr<service::AuthService> auth_service)
            : m_auth_service(std::move(auth_service)) {}
    
    ~HttpController() = default;

    HttpController(const HttpController&) = delete;
    HttpController& operator=(const HttpController&) = delete;

    net::awaitable<http::response<http::string_body>> HandleRequest(
        http::request<http::string_body> request
    );
};

} // namespace controller
