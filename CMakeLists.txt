cmake_minimum_required(VERSION 3.10)

# Set compiler paths
set(CMAKE_C_COMPILER "c:/Qt/Tools/mingw1310_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "c:/Qt/Tools/mingw1310_64/bin/g++.exe")

project(time_tracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt dependencies
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Test Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Test Svg)

# Fetch and link spdlog
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(spdlog)


# JSON library via local include
add_library(json_headers INTERFACE)
target_include_directories(json_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src/external/json/include")

file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
    src/*.h
)


# Include src/ for headers
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/external/json/include  # for nlohmann/json
)
include_directories(${CMAKE_SOURCE_DIR}/src)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(time_tracker MANUAL_FINALIZATION ${PROJECT_SOURCES})
else()
    add_executable(time_tracker ${PROJECT_SOURCES})
endif()

# Link libraries
target_link_libraries(time_tracker
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Sql
        Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Svg
        spdlog::spdlog
)

if(WIN32)
    target_link_libraries(time_tracker PRIVATE Dbghelp)
endif()

# macOS bundle properties
set_target_properties(time_tracker PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(time_tracker)
endif()
