# Client build file.
project(client)

set(CLIENT_SOURCE_FILES
  "src/client.cc"
  "src/components.cc"
  "src/components.h"
  "src/connect_mode.cc"
  "src/connect_mode.h"
  "src/input.cc"
  "src/input.h"
  "src/glo.cc"
  "src/glo.h"
  "src/mode.h"
  "src/renderer.cc"
  "src/renderer.h"
  "src/title_mode.cc"
  "src/title_mode.h"
  "src/shaders/common.h"
  "src/shaders/entity.h"
  "src/shaders/fog.h"
  "src/shaders/light.h"
  "src/shaders/post.h"
  "src/shaders/simplex.h"
  "src/shaders/text.h"
  "src/shaders/title.h"
  "src/shaders/world.h"
  "src/world/player_controller.cc"
  "src/world/player_controller.h"
  "src/world/vertex_data.cc"
  "src/world/vertex_data.h"
  "src/world/world_renderer.cc"
  "src/world/world_renderer.h")

source_group(src "${CMAKE_CURRENT_SOURCE_DIR}/src/[^/]*")
source_group(src\\shaders "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/[^/]*")
source_group(src\\world "${CMAKE_CURRENT_SOURCE_DIR}/src/world/[^/]*")

add_executable(client ${CLIENT_SOURCE_FILES})
target_include_directories(client PRIVATE "${PROJECT_ROOT}")
target_link_libraries(client
  schema core worker_sdk glew glm sfml ois)
