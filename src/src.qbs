import qbs 1.0

Product {
    Depends { name: "cpp" }
    Depends { name: "Qt"; submodules: ["core", "network"] }
    name: "qtupdatesystem"
    type: "dynamiclibrary"

    targetName: (qbs.enableDebugCode && qbs.targetOS.contains("windows")) ? (name + 'd') : name
    destinationDirectory: qbs.targetOS.contains("windows") ? "bin" : "lib"
    cpp.defines: [
        "QTUPDATESYSTEM_LIBRARY", "ERROR_CONTEXT"
    ]

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: ["/W3", "/w34100", "/w34189"]
    }
    Properties {
        condition: qbs.toolchain.contains("llvm") || qbs.toolchain.contains("gcc")
        cpp.cxxFlags: ["-std=c++11"]
    }

    Group {
        fileTagsFilter: product.type.concat("dynamiclibrary_symlink")
        qbs.install: true
        qbs.installDir: project.libInstallDir || ""
    }

    Export {
        Depends { name: "cpp" }
        Depends { name: "Qt"; submodules: ["core", "network"] }
        cpp.includePaths: path
        cpp.defines: ["ERROR_CONTEXT"]
    }

    files: [
        "common/jsonutil.cpp",
        "common/jsonutil.h",
        "common/package.cpp",
        "common/package.h",
        "common/packagemetadata.cpp",
        "common/packagemetadata.h",
        "common/packages.cpp",
        "common/packages.h",
        "common/utils.cpp",
        "common/utils.h",
        "common/version.cpp",
        "common/version.h",
        "common/versions.cpp",
        "common/versions.h",
        "errors/warning.cpp",
        "errors/warning.h",
        "operations/addoperation.cpp",
        "operations/addoperation.h",
        "operations/operation.cpp",
        "operations/operation.h",
        "operations/patchoperation.cpp",
        "operations/patchoperation.h",
        "operations/adddirectoryoperation.cpp",
        "operations/adddirectoryoperation.h",
        "operations/removedirectoryoperation.cpp",
        "operations/removedirectoryoperation.h",
        "operations/removeoperation.cpp",
        "operations/removeoperation.h",
        "packager.cpp",
        "packager.h",
        "packager/packagertask.cpp",
        "packager/packagertask.h",
        "qtupdatesystem_global.h",
        "repository.cpp",
        "repository.h",
        "updater.cpp",
        "updater.h",
        "updater/copythread.cpp",
        "updater/copythread.h",
        "updater/downloadmanager.cpp",
        "updater/downloadmanager.h",
        "updater/filemanager.cpp",
        "updater/filemanager.h",
        "updater/localrepository.cpp",
        "updater/localrepository.h",
        "updater/oneobjectthread.h"
    ]
}
