# Master build file.
project(master)

set(MASTER_SOURCE_FILES
  "src/client_handler.cc"
  "src/client_handler.h"
  "src/master.cc"
  "src/world_spawner.cc"
  "src/world_spawner.h")

source_group(src "src/[^/]*")

add_executable(master ${MASTER_SOURCE_FILES})
target_include_directories(master PRIVATE "${PROJECT_ROOT}")
target_link_libraries(master managed schema worker_sdk glm)
managed_worker_zip(master)
