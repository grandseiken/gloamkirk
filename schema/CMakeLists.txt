# Schema build file.
project(schema)

if(MSVC)
  add_definitions(/W0)
else()
  add_definitions(-w)
endif()

set(SCHEMA_SOURCE_FILES
  "${PROJECT_GENERATED}/schema/gloamkerk.cc"
  "${PROJECT_GENERATED}/schema/gloamkerk.h"
  "${PROJECT_GENERATED}/schema/gloamkerk.pb.cc"
  "${PROJECT_GENERATED}/schema/gloamkerk.pb.h"
  "${PROJECT_GENERATED}/improbable/standard_library.cc"
  "${PROJECT_GENERATED}/improbable/standard_library.h"
  "${PROJECT_GENERATED}/improbable/standard_library.pb.cc"
  "${PROJECT_GENERATED}/improbable/standard_library.pb.h")

add_library(schema STATIC ${SCHEMA_SOURCE_FILES})
target_include_directories(schema PUBLIC "${PROJECT_GENERATED}")
target_link_libraries(schema PRIVATE worker_sdk)
