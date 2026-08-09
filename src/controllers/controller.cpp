#include "controllers/controller.hpp"

#include "controllers/http/http_controller.hpp"
#include "controllers/protobuf/protobuf_controller.hpp"

namespace controller
{

Controller::Controller()
    : m_http_controller(std::make_shared<HttpController>())
    , m_protobuf_controller(std::make_shared<ProtobufController>())
{}

Controller::Controller(
    std::shared_ptr<HttpController> http_controller,
    std::shared_ptr<ProtobufController> protobuf_controller)
    : m_http_controller(std::move(http_controller))
    , m_protobuf_controller(std::move(protobuf_controller))
{}

} // namespace controller
