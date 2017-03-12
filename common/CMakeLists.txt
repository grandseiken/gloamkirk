# Common libraries build file.
project(common)

set(COLLISION_SOURCE_FILES
  "src/collision/collision.cc"
  "src/collision/collision.h")

set(MANAGED_SOURCE_FILES
  "src/managed/managed.cc"
  "src/managed/managed.h")

source_group(src\\collision "src/collision/[^/]*")
source_group(src\\managed "src/managed/[^/]*")

add_library(collision STATIC ${COLLISION_SOURCE_FILES})
target_include_directories(collision PRIVATE "${PROJECT_ROOT}")
target_link_libraries(collision PRIVATE schema worker_sdk glm)

add_library(managed STATIC ${MANAGED_SOURCE_FILES})
target_include_directories(managed PRIVATE "${PROJECT_ROOT}")
target_link_libraries(managed PRIVATE worker_sdk)
