#include "controllers/http/http_controller.hpp"
#include <nlohmann/json.hpp>

namespace controller
{

net::awaitable<http::response<http::string_body>> HttpController::HandleRequest(
        http::request<http::string_body> request) 
{
    http::response<http::string_body> response{http::status::ok, request.version()};
    response.set(http::field::server, "Run Uram Server");
    response.set(http::field::content_type, "application/json");
    response.keep_alive(request.keep_alive());

    if (request.method() == http::verb::post && request.target() == "/api/v1/auth/login")
    {
        try
        {
            auto body_json = nlohmann::json::parse(request.body());
            std::string login = body_json.value("login", "");
            std::string password = body_json.value("password", "");

            auto auth_opt = co_await m_auth_service->Authenticate(login, password);

            if (auth_opt.has_value())
            {
                nlohmann::json resp_json;
                resp_json["status"] = "success";
                resp_json["access_token"] = auth_opt->access_token;
                resp_json["ws_ticket"] = auth_opt->ws_ticket;
                resp_json["expires_in"] = auth_opt->expires_in;
                response.body() = resp_json.dump();
            }
            else
            {
                response.result(http::status::unauthorized);
                response.body() = R"({"status":"error","message":"Invalid credentials"})";
            }
        }
        catch (const std::exception& e)
        {
            response.result(http::status::bad_request);
            response.body() = R"({"status":"error","message":"Invalid request body"})";
        }
    }
    else if (request.method() == http::verb::post && request.target() == "/api/v1/auth/ws-ticket")
    {
        try
        {
            std::string token;

            // Check Authorization header first
            auto auth_header_it = request.find(http::field::authorization);
            if (auth_header_it != request.end())
            {
                token = auth_header_it->value();
            }
            else if (!request.body().empty())
            {
                auto body_json = nlohmann::json::parse(request.body());
                token = body_json.value("access_token", "");
            }

            auto ticket_opt = co_await m_auth_service->CreateWsTicketFromJwt(token);

            if (ticket_opt.has_value())
            {
                nlohmann::json resp_json;
                resp_json["status"] = "success";
                resp_json["ws_ticket"] = ticket_opt.value();
                response.body() = resp_json.dump();
            }
            else
            {
                response.result(http::status::unauthorized);
                response.body() = R"({"status":"error","message":"Invalid or expired access token"})";
            }
        }
        catch (const std::exception& e)
        {
            response.result(http::status::bad_request);
            response.body() = R"({"status":"error","message":"Invalid request"})";
        }
    }
    else
    {
        response.result(http::status::not_found);
        response.body() = R"({"status":"error","message":"Resource not found"})";
    }

    response.prepare_payload();
    co_return response;
}

} // namespace controller
