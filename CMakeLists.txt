cmake_minimum_required(VERSION 3.10)

# Set compiler paths
set(CMAKE_C_COMPILER "c:/Qt/Tools/mingw1310_64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "c:/Qt/Tools/mingw1310_64/bin/g++.exe")
# Set windres path for resource compilation
set(CMAKE_RC_COMPILER "c:/Qt/Tools/mingw1310_64/bin/windres.exe")
set(CMAKE_RC_COMPILER_INIT windres)

project(time_tracker VERSION 0.1 LANGUAGES CXX RC)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt dependencies
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Test Svg LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Test Svg LinguistTools)

# Fetch and link spdlog
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(spdlog)

# ——— Windows resources & icon dir ———
if(WIN32)
  # Use a straightforward resource file approach
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/app.rc"
    @ONLY
  )
  
  set(WIN_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/app.rc")
  message(STATUS "Using resource file: ${WIN_RESOURCES}")
endif()

# ——— Sources ———
file(GLOB_RECURSE PROJECT_SOURCES
    src/*.cpp
    src/*.h
)

# ——— Translation files ———
set(TRANSLATIONS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/translations)
set(TS_FILES
    ${TRANSLATIONS_DIR}/time_tracker_en_US.ts
    ${TRANSLATIONS_DIR}/time_tracker_ru_RU.ts
)

# Generate .qm files from .ts files
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_lrelease(TARGET time_tracker
        TS_FILES ${TS_FILES}
        QM_FILES_OUTPUT_VARIABLE QM_FILES
    )
else()
    qt5_add_translation(QM_FILES ${TS_FILES})
endif()

# Create a resource file for the translations
set(TRANSLATIONS_QRC ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
file(WRITE ${TRANSLATIONS_QRC} "<RCC>\n  <qresource prefix=\"/translations\">\n")

foreach(QM_FILE ${QM_FILES})
    get_filename_component(QM_NAME ${QM_FILE} NAME)
    file(APPEND ${TRANSLATIONS_QRC} "    <file alias=\"${QM_NAME}\">${QM_FILE}</file>\n")
    # Make sure the build depends on the .qm files
    set_source_files_properties(${QM_FILE} PROPERTIES GENERATED TRUE)
    list(APPEND QM_FILES_IN_BINARY_DIR ${QM_FILE})
endforeach()

file(APPEND ${TRANSLATIONS_QRC} "  </qresource>\n</RCC>")

# Add the resource file
qt_add_resources(QRC_SOURCES ${TRANSLATIONS_QRC})

# Add a custom target for updating translations
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_lupdate(TARGET time_tracker
        SOURCES ${PROJECT_SOURCES}
        TS_FILES ${TS_FILES}
    )
else()
    add_custom_target(update_translations
        COMMAND ${Qt5_LUPDATE_EXECUTABLE} -locations relative ${PROJECT_SOURCE_DIR} -ts ${TS_FILES}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Updating translation files..."
    )
endif()

# ——— Create the executable ———
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(time_tracker MANUAL_FINALIZATION 
        ${PROJECT_SOURCES} 
        ${WIN_RESOURCES}
        ${QM_FILES_IN_BINARY_DIR}
        ${QRC_SOURCES}
    )
else()
    add_executable(time_tracker 
        ${PROJECT_SOURCES} 
        ${WIN_RESOURCES}
        ${QM_FILES}
        ${QRC_SOURCES}
    )
endif()

# ——— C++ include dirs ———
target_include_directories(time_tracker
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/external/json/include
)

# ——— Link libraries ———
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

# ——— macOS bundle props ———
set_target_properties(time_tracker PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(time_tracker)
endif()