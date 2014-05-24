add_definitions( -DQTUPDATESYSTEM_LIBRARY )

find_package(Qt5Network)

set(QtUpdateSystem_FILES
    common/jsonutil.cpp
    common/jsonutil.h
    common/package.cpp
    common/package.h
    common/packagemetadata.cpp
    common/packagemetadata.h
    common/packages.cpp
    common/packages.h
    common/utils.cpp
    common/utils.h
    common/version.cpp
    common/version.h
    common/versions.cpp
    common/versions.h
    errors/warning.cpp
    errors/warning.h
    exceptions.h
    operations/addoperation.cpp
    operations/addoperation.h
    operations/operation.cpp
    operations/operation.h
    operations/patchoperation.cpp
    operations/patchoperation.h
    operations/adddirectoryoperation.cpp
    operations/adddirectoryoperation.h
    operations/removedirectoryoperation.cpp
    operations/removedirectoryoperation.h
    operations/removeoperation.cpp
    operations/removeoperation.h
    packager.cpp
    packager.h
    packager/packagertask.cpp
    packager/packagertask.h
    qtupdatesystem_global.h
    repository.cpp
    repository.h
    updater.cpp
    updater.h
    updater/copythread.cpp
    updater/copythread.h
    updater/downloadmanager.cpp
    updater/downloadmanager.h
    updater/filemanager.cpp
    updater/filemanager.h
    updater/localrepository.cpp
    updater/localrepository.h
    updater/oneobjectthread.h)

add_library(QtUpdateSystem SHARED ${QtUpdateSystem_FILES})
target_link_libraries(QtUpdateSystem Qt5::Core)
target_link_libraries(QtUpdateSystem Qt5::Network)
