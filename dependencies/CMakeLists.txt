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
target_include_directories(glew SYSTEM INTERFACE "glew-cmake/include")
target_compile_definitions(glew INTERFACE GLEW_STATIC)
target_link_libraries(glew INTERFACE libglew_static)

# GLM.
add_library(glm INTERFACE)
target_include_directories(glm SYSTEM INTERFACE "glm")

# SFML.
set(BUILD_SHARED_LIBS FALSE CACHE BOOL "")
set(SFML_BUILD_DOC FALSE CACHE BOOL "")
set(SFML_BUILD_EXAMPLES FALSE CACHE BOOL "")
set(SFML_INSTALL_PKGCONFIG_FILES FALSE CACHE BOOL "")
add_subdirectory(SFML EXCLUDE_FROM_ALL)
add_library(sfml INTERFACE)
target_include_directories(sfml SYSTEM INTERFACE "SFML/include")
target_compile_definitions(sfml INTERFACE SFML_STATIC)
target_link_libraries(sfml INTERFACE sfml-audio sfml-graphics sfml-window sfml-system)

# OIS.
include(OIS.cmake)

# Find all the worker SDK libraries.
find_package(Threads)
macro(find_worker_sdk DIR PREFIX)
  find_library(${PREFIX}_CORE_SDK CoreSdk "${DIR}/lib" DEFAULT_PATH)
  find_library(${PREFIX}_WORKER_SDK WorkerSdk "${DIR}/lib" NO_DEFAULT_PATH)
  find_library(${PREFIX}_PROTOBUF NAMES protobuf libprotobuf PATHS "${DIR}/lib" NO_DEFAULT_PATH)
  find_library(${PREFIX}_RAKNET RakNetLibStatic "${DIR}/lib" NO_DEFAULT_PATH)
  find_library(${PREFIX}_SSL ssl "${DIR}/lib" NO_DEFAULT_PATH)
  find_library(${PREFIX}_ZLIB NAMES z zlibstatic PATHS "${DIR}/lib" NO_DEFAULT_PATH)
  find_library(${PREFIX}_GPR NAMES gpr PATHS "${DIR}/lib" NO_DEFAULT_PATH)
  find_library(${PREFIX}_GRPC NAMES grpc PATHS "${DIR}/lib" NO_DEFAULT_PATH)
  find_library(${PREFIX}_GRPCXX NAMES grpc++ PATHS "${DIR}/lib" NO_DEFAULT_PATH)
endmacro()

set(WORKER_SDK_DIR  "${PROJECT_DEPENDENCIES}/worker_sdk")
find_worker_sdk("${WORKER_SDK_DIR}" "LIB")
if(MSVC)
  find_worker_sdk("${WORKER_SDK_DIR}_debug" "DEBUG_LIB")
endif()

# Worker SDK target.
add_library(worker_sdk INTERFACE)
target_include_directories(worker_sdk SYSTEM INTERFACE "${WORKER_SDK_DIR}/include")
if (MSVC)
  target_link_libraries(worker_sdk INTERFACE
    optimized ${LIB_WORKER_SDK}
    optimized ${LIB_CORE_SDK}
    optimized ${LIB_PROTOBUF}
    optimized ${LIB_RAKNET}
    optimized ${LIB_ZLIB}
    optimized ${LIB_GRPCXX}
    optimized ${LIB_GRPC}
    optimized ${LIB_GPR}
    optimized ${LIB_SSL}
    debug ${DEBUG_LIB_WORKER_SDK}
    debug ${DEBUG_LIB_CORE_SDK}
    debug ${DEBUG_LIB_PROTOBUF}
    debug ${DEBUG_LIB_RAKNET}
    debug ${DEBUG_LIB_ZLIB}
    debug ${DEBUG_LIB_GRPCXX}
    debug ${DEBUG_LIB_GRPC}
    debug ${DEBUG_LIB_GPR}
    debug ${DEBUG_LIB_SSL})
else()
  target_link_libraries(worker_sdk INTERFACE
    ${LIB_WORKER_SDK} ${LIB_CORE_SDK} ${LIB_PROTOBUF}
    ${LIB_RAKNET} ${LIB_ZLIB} ${LIB_GRPCXX} ${LIB_GRPC} ${LIB_GPR} ${LIB_SSL})
endif()
target_link_libraries(worker_sdk INTERFACE ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
