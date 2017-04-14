# Schema build file.
project(schema)

if(MSVC)
  add_definitions(/W0)
else()
  add_definitions(-w)
endif()

set(SCHEMA_FILES
  "schema/chunk.schema"
  "schema/common.schema"
  "schema/master.schema"
  "schema/player.schema")

set(SCHEMA_SOURCE_FILES
  "${PROJECT_GENERATED}/schema/chunk.cc"
  "${PROJECT_GENERATED}/schema/chunk.h"
  "${PROJECT_GENERATED}/schema/chunk.pb.cc"
  "${PROJECT_GENERATED}/schema/chunk.pb.h"
  "${PROJECT_GENERATED}/schema/common.cc"
  "${PROJECT_GENERATED}/schema/common.h"
  "${PROJECT_GENERATED}/schema/common.pb.cc"
  "${PROJECT_GENERATED}/schema/common.pb.h"
  "${PROJECT_GENERATED}/schema/master.cc"
  "${PROJECT_GENERATED}/schema/master.h"
  "${PROJECT_GENERATED}/schema/master.pb.cc"
  "${PROJECT_GENERATED}/schema/master.pb.h"
  "${PROJECT_GENERATED}/schema/player.cc"
  "${PROJECT_GENERATED}/schema/player.h"
  "${PROJECT_GENERATED}/schema/player.pb.cc"
  "${PROJECT_GENERATED}/schema/player.pb.h"
  "${PROJECT_GENERATED}/improbable/standard_library.cc"
  "${PROJECT_GENERATED}/improbable/standard_library.h"
  "${PROJECT_GENERATED}/improbable/standard_library.pb.cc"
  "${PROJECT_GENERATED}/improbable/standard_library.pb.h")

source_group(schema "${CMAKE_CURRENT_SOURCE_DIR}/schema/[^/]*")
source_group(generated\\schema "${PROJECT_GENERATED}/schema/[^/]*")
source_group(generated\\improbable "${PROJECT_GENERATED}/improbable/[^/]*")

add_library(schema STATIC ${SCHEMA_FILES} ${SCHEMA_SOURCE_FILES})
target_include_directories(schema SYSTEM PUBLIC "${PROJECT_GENERATED}")
target_link_libraries(schema PRIVATE worker_sdk)
