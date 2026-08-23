#pragma once

#include <memory>

namespace controller
{

class HttpController;
class ProtobufController;

class Controller
{
private:
    std::shared_ptr<HttpController> m_http_controller;
    std::shared_ptr<ProtobufController> m_protobuf_controller;

public:
    Controller(
        std::shared_ptr<HttpController> http_controller,
        std::shared_ptr<ProtobufController> protobuf_controller
    );

    ~Controller() = default;

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    std::shared_ptr<HttpController> GetHttpController() const noexcept { return m_http_controller; }
    std::shared_ptr<ProtobufController> GetProtobufController() const noexcept { return m_protobuf_controller; }
};

} // namespace controller
