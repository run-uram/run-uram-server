#include "controllers/http/http_controller.hpp"

namespace controller
{

net::awaitable<http::response<http::string_body>> HttpController::HandleRequest(
    http::request<http::string_body> request)
{
    http::response<http::string_body> response{http::status::ok, request.version()};
    response.set(http::field::server, "runuram");
    response.set(http::field::content_type, "application/json");
    response.keep_alive(request.keep_alive());
    response.body() = R"({"status":"ok","server":"runuram"})";
    response.prepare_payload();

    co_return response;
}

} // namespace controller
