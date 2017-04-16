cmake_minimum_required (VERSION 2.8.11)
project (QtUpdateSystem)

set(BUILD_SHARED_LIBS OFF)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Workaround for : http://public.kitware.com/Bug/view.php?id=14292
if(CMAKE_VERSION VERSION_LESS 2.8.12)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "cmake version < 2.8.12, adding QT_NO_DEBUG manually")
        add_definitions(-DQT_NO_DEBUG)
    endif()
endif()

if(MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /w34100 /w34189")
else()
    # clang || gcc
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()

if(NOT DEFINED QtUpdateSystem_NOOUPUTPATH)
    set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
endif()

find_package(Qt5Network)
find_package(Qt5Core)

add_subdirectory (src)
add_subdirectory (deps/brotli) 
add_subdirectory (tests)
if(NOT DEFINED QtUpdateSystem_NOUTILS)
    add_subdirectory (utils)
endif()
