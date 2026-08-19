target_sources(${PROJECT_NAME}_lib PRIVATE
    controller.cpp
    http/http_controller.cpp
)

add_subdirectory(protobuf)