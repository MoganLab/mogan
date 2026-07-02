package("qt6core")
    set_base("qt6lib")
    set_kind("library")

    on_load(function (package)
        if package:is_plat("wasm") then
            package:add("deps", "pcre2", {configs = {bitwidth = "16"}})
        end
        package:data_set("libname", "Core")
        if package:is_plat("android") then
            package:data_set("syslinks", "z")
        elseif package:is_plat("iphoneos") then
            package:data_set("frameworks", {"UIKit", "CoreText", "CoreGraphics", "CoreServices", "CoreFoundation"})
            package:data_set("syslinks", "z")
        end

        package:base():script("load")(package)
    end)

    on_test(function (package)
        local cxflags
        local ldflags
        if package:is_plat("windows") then
            cxflags = {"/Zc:__cplusplus", "/permissive-"}
        elseif package:is_plat("wasm") then
            ldflags = {"-lQt6BundledZLIB"}
        else
            cxflags = "-fPIC"
        end
        assert(package:check_cxxsnippets({test = [[
            int test(int argc, char** argv) {
                QCoreApplication app (argc, argv);
                return app.exec();
            }
        ]]}, {configs = {languages = "c++17", cxflags = cxflags, ldflags = ldflags}, includes = {"QCoreApplication"}}))
    end)