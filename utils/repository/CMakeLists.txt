include_directories(../../src)
add_executable(repository main.cpp)
target_link_libraries(repository QtUpdateSystem)
target_link_libraries(repository Qt5::Core)
