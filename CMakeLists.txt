# Gloamkerk main build file.
project(gloamkerk)
cmake_minimum_required(VERSION 3.0)

# Useful directories.
set(PROJECT_ROOT "${CMAKE_SOURCE_DIR}")
set(PROJECT_BUILD "${CMAKE_SOURCE_DIR}/build")
set(PROJECT_GENERATED "${PROJECT_BUILD}/generated")
set(PROJECT_SDK "${PROJECT_BUILD}/worker_sdk")

if(MSVC)
  add_definitions(/W4)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  add_definitions(-Wall -Wextra -Werror -pedantic)
endif()

# Find the worker SDK libraries.
find_package(Threads)
find_library(LIB_CORE_SDK CoreSdk "${PROJECT_SDK}/lib" DEFAULT_PATH)
find_library(LIB_WORKER_SDK WorkerSdk "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)
find_library(LIB_PROTOBUF NAMES protobuf libprotobuf PATHS "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)
find_library(LIB_RAKNET RakNetLibStatic "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)
find_library(LIB_SSL ssl "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)
find_library(LIB_ZLIB NAMES z zlibstatic PATHS "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)
find_library(LIB_GPR NAMES gpr PATHS "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)
find_library(LIB_GRPC NAMES grpc PATHS "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)
find_library(LIB_GRPCXX NAMES grpc++ PATHS "${PROJECT_SDK}/lib" NO_DEFAULT_PATH)

# The worker SDK dependency.
add_library(worker_sdk INTERFACE)
target_include_directories(worker_sdk INTERFACE "${PROJECT_SDK}/include")
target_link_libraries(worker_sdk INTERFACE
  ${LIB_WORKER_SDK} ${LIB_CORE_SDK} ${CMAKE_THREAD_LIBS_INIT} ${LIB_PROTOBUF}
  ${LIB_RAKNET} ${LIB_ZLIB} ${LIB_GRPCXX} ${LIB_GRPC} ${LIB_GPR} ${LIB_SSL} ${CMAKE_DL_LIBS})

# Add the build for the schema.
add_subdirectory(schema)
# Add the builds for each worker.
add_subdirectory(workers/client)
