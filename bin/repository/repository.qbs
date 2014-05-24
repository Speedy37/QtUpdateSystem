import qbs 1.0

CppApplication {
    Depends { name: "Qt"; submodules: ["core"] }
    Depends { name: "qtupdatesystem" }
    name : "repository"
    type: "application"
    consoleApplication: true
    cpp.defines: [
        "SRCDIR=\""+path+"/\""
    ]

    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: ["/W3", "/w34100", "/w34189"]
    }
    Properties {
        condition: qbs.toolchain.contains("llvm") || qbs.toolchain.contains("gcc")
        cpp.cxxFlags: ["-std=c++11"]
    }
    files : [
        "main.cpp"
    ]
    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.binInstallDir || ""
    }
}
