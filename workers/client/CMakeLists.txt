# Client build file.
project(client)

set(CLIENT_SOURCE_FILES
  "src/client.cc"
  "src/connect_mode.cc"
  "src/connect_mode.h"
  "src/logic/world.cc"
  "src/logic/world.h"
  "src/glo.cc"
  "src/glo.h"
  "src/mode.h"
  "src/renderer.cc"
  "src/renderer.h"
  "src/title_mode.cc"
  "src/title_mode.h"
  "src/shaders/common.h"
  "src/shaders/post.h"
  "src/shaders/simplex.h"
  "src/shaders/text.h"
  "src/shaders/title.h"
  "src/shaders/world.h")

source_group(src "src/[^/]*")
source_group(src\\shaders "src/shaders/[^/]*")
source_group(src\\logic "src/logic/[^/]*")

add_executable(client ${CLIENT_SOURCE_FILES})
target_include_directories(client PRIVATE "${PROJECT_ROOT}")
target_link_libraries(client worker_sdk schema glew glm sfml)
