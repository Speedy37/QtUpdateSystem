import qbs 1.0

CppApplication {
    Depends { name: "Qt"; submodules: ["core"] }
    Depends { name: "qtupdatesystem" }
    name : "packager"
    type: "application"
    consoleApplication: true
    cpp.defines: [
        "SRCDIR=\""+path+"/\""
    ]
    files : [
        "main.cpp"
    ]
    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.binInstallDir || ""
    }
}
