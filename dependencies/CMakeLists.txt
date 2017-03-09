# Dependencies build file.
if(MSVC)
  add_definitions(/W0)
else()
  add_definitions(-w)
endif()

# GLEW.
set(glew-cmake_BUILD_STATIC ON CACHE BOOL "")
add_subdirectory(glew-cmake EXCLUDE_FROM_ALL)
add_library(glew INTERFACE)
target_include_directories(glew SYSTEM INTERFACE glew-cmake/include)
target_compile_definitions(glew INTERFACE GLEW_STATIC)
target_link_libraries(glew INTERFACE libglew_static)

# GLM.
add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE glm)

# SFML.
set(BUILD_SHARED_LIBS FALSE CACHE BOOL "")
set(SFML_BUILD_DOC FALSE CACHE BOOL "")
set(SFML_BUILD_EXAMPLES FALSE CACHE BOOL "")
set(SFML_INSTALL_PKGCONFIG_FILES FALSE CACHE BOOL "")
add_subdirectory(SFML EXCLUDE_FROM_ALL)
add_library(sfml INTERFACE)
target_include_directories(sfml SYSTEM INTERFACE SFML/include)
target_compile_definitions(sfml INTERFACE SFML_STATIC)
target_link_libraries(sfml INTERFACE sfml-audio sfml-graphics sfml-window sfml-system)

# OIS.
include(OIS.cmake)

# Find all the worker SDK libraries.
set(WORKER_SDK_DIR "${PROJECT_DEPENDENCIES}/worker_sdk")
find_package(Threads)
find_library(LIB_CORE_SDK CoreSdk "${WORKER_SDK_DIR}/lib" DEFAULT_PATH)
find_library(LIB_WORKER_SDK WorkerSdk "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIB_PROTOBUF NAMES protobuf libprotobuf PATHS "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIB_RAKNET RakNetLibStatic "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIB_SSL ssl "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIB_ZLIB NAMES z zlibstatic PATHS "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIB_GPR NAMES gpr PATHS "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIB_GRPC NAMES grpc PATHS "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)
find_library(LIB_GRPCXX NAMES grpc++ PATHS "${WORKER_SDK_DIR}/lib" NO_DEFAULT_PATH)

# Worker SDK target.
add_library(worker_sdk INTERFACE)
target_include_directories(worker_sdk SYSTEM INTERFACE "${WORKER_SDK_DIR}/include")
target_link_libraries(worker_sdk INTERFACE
  ${LIB_WORKER_SDK} ${LIB_CORE_SDK} ${CMAKE_THREAD_LIBS_INIT} ${LIB_PROTOBUF}
  ${LIB_RAKNET} ${LIB_ZLIB} ${LIB_GRPCXX} ${LIB_GRPC} ${LIB_GPR} ${LIB_SSL} ${CMAKE_DL_LIBS})
