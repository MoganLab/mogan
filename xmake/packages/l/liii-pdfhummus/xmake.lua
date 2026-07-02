package("liii-pdfhummus")
    set_homepage("https://www.pdfhummus.com/")
    set_description("High performance library for creating, modiyfing and parsing PDF files in C++ ")
    set_license("Apache-2.0")

    set_sourcedir(path.join(os.scriptdir(), "../../../../3rdparty/pdfhummus"))

    add_deps("zlib", "liii-libaesgm")
    add_deps("freetype", {configs={png=true}})

    add_configs("libtiff", {description = "Supporting tiff image", default = false, type = "boolean"})
    add_configs("libjpeg", {description = "Support DCT encoding", default = false, type = "boolean"})
    add_configs("libpng", {description = "Support png image", default = false, type = "boolean"})

    if is_plat("linux") then
        add_syslinks("m")
    end

    on_load(function (package)
        for _, dep in ipairs({"libtiff", "libpng", "libjpeg"}) do
            if package:config(dep) then
                package:add("deps", dep)
            end
        end
        if package:is_plat("wasm") then
            package:add("dep", "zlib")
        end
    end)
    on_install("linux", "windows", "mingw", "macosx", "wasm", function (package)
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        for _, dep in ipairs({"libtiff", "libpng", "libjpeg"}) do
            if package:config(dep) then
                configs[dep] = true
            end
        end
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include "PDFWriter/PDFWriter.h"
            #include <iostream>
            using namespace std;
            using namespace PDFHummus;
            void test() {
                PDFWriter pdfWriter;
                pdfWriter.Reset();
            }
        ]]}, {configs = {languages = "c++11"}}))
    end)
package_end()