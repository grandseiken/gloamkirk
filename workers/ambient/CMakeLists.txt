# Client build file.
project(ambient)

set(AMBIENT_SOURCE_FILES
  "src/ambient.cc")

source_group(src "${CMAKE_CURRENT_SOURCE_DIR}/src/[^/]*")

add_executable(ambient ${AMBIENT_SOURCE_FILES})
target_include_directories(ambient PRIVATE "${PROJECT_ROOT}")
target_link_libraries(ambient schema core managed worker_sdk glm)
managed_worker_zip(ambient)
