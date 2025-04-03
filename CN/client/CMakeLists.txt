cmake_minimum_required(VERSION 3.21.1)

project(client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

file(GLOB UI_FILES *.ui)
file(GLOB HEADER_FILES *.h)
file(GLOB SOURCE_FILES *.cc)

set(CMAKE_AUTOUIC_SEARCH_PATHS .)

qt_add_executable(client
  ${UI_FILES}
  ${HEADER_FILES}
  ${SOURCE_FILES})

target_link_libraries(client
  PRIVATE
  Qt6::Widgets
  Qt6::Network)

set_target_properties(client
  PROPERTIES
  MACOSX_BUNDLE ON
  WIN32_EXECUTABLE ON)
