# Master build file.
project(master)

set(MASTER_SOURCE_FILES "src/master.cc")
source_group(src "src/[^/]*")

add_executable(master ${MASTER_SOURCE_FILES})
target_include_directories(master PRIVATE "${PROJECT_ROOT}")
target_link_libraries(master managed worker_sdk schema)
managed_worker_zip(master)
