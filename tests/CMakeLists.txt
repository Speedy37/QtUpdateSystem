include_directories(../src)
add_definitions(-DSRCDIR="${CMAKE_CURRENT_LIST_DIR}/")
find_package(Qt5Test)

add_executable(tests
    main.cpp
    testutils.cpp
    testutils.h
    tst_packager.cpp
    tst_packager.h
    tst_repository.cpp
    tst_repository.h
    tst_updatechain.cpp
    tst_updatechain.h
    tst_updater.cpp
    tst_updater.h
)
target_link_libraries(tests QtUpdateSystem)
target_link_libraries(tests Qt5::Core)
target_link_libraries(tests Qt5::Test)
