# Main build file.
project(gloamkirk)
cmake_minimum_required(VERSION 3.0)

# Useful directories.
set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}")
set(PROJECT_BUILD "${CMAKE_SOURCE_DIR}/build")
set(PROJECT_GENERATED "${PROJECT_BUILD}/generated")
set(PROJECT_DEPENDENCIES "${PROJECT_BUILD}/dependencies")
set(PROJECT_ASSEMBLY "${PROJECT_BUILD}/assembly")

add_definitions(${CXX_EXTRA_DEFINITIONS})
if(MSVC)
  add_definitions(/W4 /wd4201)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  add_definitions(-Wall -Wextra -Werror -pedantic)
endif()

# Find toolbelt.
find_program(SPATIAL_COMMAND spatial)
# Function to package workers.
function(managed_worker_zip TARGET)
  add_custom_target("${TARGET}_zip" ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_ASSEMBLY}/worker"
    COMMAND ${SPATIAL_COMMAND} file zip
            -b "$<TARGET_FILE_DIR:${TARGET}>"
            -o "${PROJECT_ASSEMBLY}/worker/${TARGET}"
            --worker_platform=current "$<TARGET_FILE_NAME:${TARGET}>"
    DEPENDS ${TARGET})
endfunction()

# Add the build for dependencies.
add_subdirectory(dependencies)

# Add the build for the schema.
add_subdirectory(schema)

# Add the build for the common libraries.
add_subdirectory(common)

# Add the build for the snapshot generation.
add_subdirectory(snapshot)

# Add the builds for each workers.
add_subdirectory(workers/client)
add_subdirectory(workers/master)
