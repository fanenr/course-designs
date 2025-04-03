cmake_minimum_required(VERSION 3.21.1)

project(client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

file(GLOB UI_FILES ui/*.ui)
file(GLOB SOURCE_FILES src/*.cc)
file(GLOB HEADER_FILES include/*.h)

set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

qt_add_executable(client
  app.cc
  ${UI_FILES}
  ${SOURCE_FILES}
  ${HEADER_FILES})

target_link_libraries(client
  PRIVATE
  Qt6::Widgets
  Qt6::Network)

target_include_directories(client PRIVATE include)

set_target_properties(client
  PROPERTIES
  MACOSX_BUNDLE ON
  WIN32_EXECUTABLE ON)
