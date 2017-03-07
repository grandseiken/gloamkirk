# Common libraries build file.
project(common)

set(MANAGED_SOURCE_FILES
  "src/managed/managed.cc"
  "src/managed/managed.h")

add_library(managed STATIC ${MANAGED_SOURCE_FILES})
target_include_directories(managed SYSTEM PRIVATE "${PROJECT_ROOT}")
target_link_libraries(managed PRIVATE worker_sdk)
