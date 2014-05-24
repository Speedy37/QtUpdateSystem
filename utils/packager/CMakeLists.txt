include_directories(../../src)
add_executable(packager main.cpp)
target_link_libraries(packager QtUpdateSystem)
target_link_libraries(packager Qt5::Core)
