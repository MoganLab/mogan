package("qt6svg")
    set_base("qt6lib")
    set_kind("library")

    on_load(function (package)
        -- qt6svg 依赖 core + gui
        package:add("deps", "qt6core", {debug = package:is_debug(), version = package:version_str()})
        package:add("deps", "qt6gui",  {debug = package:is_debug(), version = package:version_str()})

        -- Qt module name
        package:data_set("libname", "Svg")

        -- base loader（Qt6通用逻辑）
        package:base():script("load")(package)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <QSvgRenderer>
            #include <QPainter>

            int test() {
                QSvgRenderer renderer(QString("test.svg"));
                return renderer.isValid() ? 0 : 1;
            }
        ]]}, {
            configs = {
                languages = "c++17"
            },
            includes = {
                "QSvgRenderer",
                "QPainter"
            }
        }))
    end)