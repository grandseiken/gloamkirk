# Client build file.
project(client)
cmake_minimum_required(VERSION 3.0)

set(CLIENT_SOURCE_FILES
  "src/client.cc"
  "src/glo.h"
  "src/renderer.cc"
  "src/renderer.h"
  "src/shaders/fog.h"
  "src/shaders/post.h"
  "src/shaders/simplex.h"
  "src/shaders/upscale.h")

source_group(src "src/[^/]*")
source_group(src\\shaders "src/shaders/[^/]*")

add_executable(client ${CLIENT_SOURCE_FILES})
target_include_directories(client PRIVATE "${PROJECT_ROOT}")
target_link_libraries(client worker_sdk schema glew glm sfml)
