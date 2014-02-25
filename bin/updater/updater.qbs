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
    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.binInstallDir || ""
    }
}
