#pragma once

#include "common.hpp"

namespace controller
{

class HttpController
{
public:
    HttpController() = default;
    ~HttpController() = default;

    HttpController(const HttpController&) = delete;
    HttpController& operator=(const HttpController&) = delete;

    net::awaitable<http::response<http::string_body>> HandleRequest(
        http::request<http::string_body> request
    );
};

} // namespace controller
