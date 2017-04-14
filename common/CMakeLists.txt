# Common libraries build file.
project(common)

set(CORE_SOURCE_FILES
  "src/core/collision.cc"
  "src/core/collision.h"
  "src/core/tile_map.cc"
  "src/core/tile_map.h")

set(MANAGED_SOURCE_FILES
  "src/managed/managed.cc"
  "src/managed/managed.h")

source_group(src\\core "${CMAKE_CURRENT_SOURCE_DIR}/src/core/[^/]*")
source_group(src\\managed "${CMAKE_CURRENT_SOURCE_DIR}/src/managed/[^/]*")

add_library(core STATIC ${CORE_SOURCE_FILES})
target_include_directories(core PRIVATE "${PROJECT_ROOT}")
target_link_libraries(core PRIVATE schema worker_sdk glm)

add_library(managed STATIC ${MANAGED_SOURCE_FILES})
target_include_directories(managed PRIVATE "${PROJECT_ROOT}")
target_link_libraries(managed PRIVATE worker_sdk)
