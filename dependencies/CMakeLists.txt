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
target_include_directories(glew INTERFACE glew-cmake/include)
target_compile_definitions(glew INTERFACE GLEW_STATIC)
target_link_libraries(glew INTERFACE libglew_static)

# GLM.
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE glm)

# SFML.
set(BUILD_SHARED_LIBS FALSE CACHE BOOL "")
set(SFML_BUILD_DOC FALSE CACHE BOOL "")
set(SFML_BUILD_EXAMPLES FALSE CACHE BOOL "")
set(SFML_INSTALL_PKGCONFIG_FILES FALSE CACHE BOOL "")
add_subdirectory(SFML EXCLUDE_FROM_ALL)
add_library(sfml INTERFACE)
target_include_directories(sfml INTERFACE SFML/include)
target_compile_definitions(sfml INTERFACE SFML_STATIC)
target_link_libraries(sfml INTERFACE
  sfml-audio sfml-graphics sfml-window sfml-system)
