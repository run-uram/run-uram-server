target_sources(${PROJECT_NAME}_lib PRIVATE
    main.cpp
)

add_subdirectory(logger)
add_subdirectory(config)
add_subdirectory(context)
add_subdirectory(controllers)
add_subdirectory(server)

