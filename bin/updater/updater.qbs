import qbs 1.0

CppApplication {
    Depends { name: "Qt"; submodules: ["core"] }
    Depends { name: "qtupdatesystem" }
    name : "updater"
    type: "application"
    consoleApplication: true
    files : [
        "main.cpp"
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
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.binInstallDir || ""
    }
}
