# Client build file.
project(snapshot)

set(SNAPSHOT_SOURCE_FILES "src/snapshot_generator.cc")
source_group(src "src/[^/]*")

add_executable(snapshot_generator ${SNAPSHOT_SOURCE_FILES})
target_include_directories(snapshot_generator PRIVATE "${PROJECT_ROOT}")
target_link_libraries(snapshot_generator worker_sdk schema)

set(INITIAL_SNAPSHOT "${PROJECT_BUILD}/initial.snapshot")
add_custom_command(
  OUTPUT "${INITIAL_SNAPSHOT}"
  COMMAND snapshot_generator "${INITIAL_SNAPSHOT}"
  DEPENDS snapshot_generator)

add_custom_target(snapshot ALL DEPENDS "${INITIAL_SNAPSHOT}")
