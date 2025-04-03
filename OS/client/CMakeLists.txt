cmake_minimum_required(VERSION 3.21.1)

project(client VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

set(CMAKE_AUTOUIC_SEARCH_PATHS .)

qt_add_executable(mem
  mem.cc
  mem.ui)

target_link_libraries(mem PRIVATE Qt6::Widgets)

set_target_properties(mem
  PROPERTIES
  MACOSX_BUNDLE ON
  WIN32_EXECUTABLE ON)
