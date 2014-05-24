cmake_minimum_required (VERSION 2.8.11)
project (QtUpdateSystem)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_PREFIX_PATH "C:\\Qt\\5.3\\msvc2013")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin-debug)
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin-debug)
else()
    set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin-release)
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin-release)
endif()

find_package(Qt5Core)

add_subdirectory (src) 
add_subdirectory (tests)
add_subdirectory (bin)
