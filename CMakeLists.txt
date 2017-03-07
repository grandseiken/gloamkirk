# Main build file.
project(gloamkirk)
cmake_minimum_required(VERSION 3.0)

# Useful directories.
set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}")
set(PROJECT_BUILD "${CMAKE_SOURCE_DIR}/build")
set(PROJECT_GENERATED "${PROJECT_BUILD}/generated")
set(PROJECT_DEPENDENCIES "${PROJECT_BUILD}/dependencies")
set(PROJECT_SNAPSHOT "${PROJECT_BUILD}/snapshot")

add_definitions(${CXX_EXTRA_DEFINITIONS})
if(MSVC)
  add_definitions(/W4)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  add_definitions(-Wall -Wextra -Werror -pedantic)
endif()

# Add the build for dependencies.
add_subdirectory(dependencies)

# Add the build for the schema.
add_subdirectory(schema)

# Add the build for the snapshot generation.
add_subdirectory(snapshot)

# Add the builds for each workers.
add_subdirectory(workers/client)
