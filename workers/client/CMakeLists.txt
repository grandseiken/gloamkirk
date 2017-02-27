# Client build file.
project(client)
cmake_minimum_required(VERSION 3.0)

set(CLIENT_SOURCE_FILES "src/client.cc")
add_executable(client ${CLIENT_SOURCE_FILES})
target_include_directories(client PRIVATE ${PROJECT_ROOT})
target_link_libraries(client PRIVATE worker_sdk schema)
