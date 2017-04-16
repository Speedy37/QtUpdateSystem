include_directories(../../src)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Workaround for : http://public.kitware.com/Bug/view.php?id=14292
if(CMAKE_VERSION VERSION_LESS 2.8.12)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "cmake version < 2.8.12, adding QT_NO_DEBUG manually")
        add_definitions(-DQT_NO_DEBUG)
    endif()
endif()

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()

find_package(Qt5Core)
find_package(Qt5Gui)
find_package(Qt5Widgets)
find_package(Qt5Qml)
find_package(Qt5Quick)

qt5_wrap_ui(LAUNCHER_UIS updatedialog.ui)
qt5_add_resources(LAUNCHER_QRC ressources.qrc)

add_executable(Launcher
    main.cpp
    borderlesswindow.cpp
    borderlesswindow.h
    mainwindow.cpp
    mainwindow.h
    updatedialog.cpp
    updatedialog.h
    main.qml
    content/Input.qml
    content/Label.qml
    content/Button.qml
    ${LAUNCHER_QRC}
    ${LAUNCHER_UIS}
)
target_link_libraries(Launcher QtUpdateSystem)
target_link_libraries(Launcher Qt5::Core)
target_link_libraries(Launcher Qt5::Widgets)
target_link_libraries(Launcher Qt5::Qml)
target_link_libraries(Launcher Qt5::Quick)
