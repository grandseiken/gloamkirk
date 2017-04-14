# Master build file.
project(master)

set(MASTER_SOURCE_FILES
  "src/client_handler.cc"
  "src/client_handler.h"
  "src/master.cc"
  "src/world_spawner.cc"
  "src/world_spawner.h"
  "src/worldgen/map_builder.cc"
  "src/worldgen/map_builder.h"
  "src/worldgen/room_builder.cc"
  "src/worldgen/room_builder.h"
  "src/worldgen/world_builder.cc"
  "src/worldgen/world_builder.h")

source_group(src "${CMAKE_CURRENT_SOURCE_DIR}/src/[^/]*")
source_group(src\\worldgen "${CMAKE_CURRENT_SOURCE_DIR}/src/worldgen[^/]*")

add_executable(master ${MASTER_SOURCE_FILES})
target_include_directories(master PRIVATE "${PROJECT_ROOT}")
target_link_libraries(master managed schema worker_sdk glm)
managed_worker_zip(master)
