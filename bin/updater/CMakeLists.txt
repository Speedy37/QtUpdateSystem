include_directories(../../src)
add_executable(updater main.cpp)
target_link_libraries(updater QtUpdateSystem)
target_link_libraries(updater Qt5::Core)
