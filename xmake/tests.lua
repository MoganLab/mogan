function add_target_cpp_test(filepath, dep1, dep2)
    local testname = path.basename(filepath)
    target(testname) do
        -- QWindowKit on macOS
        if is_plat("macosx") then
            add_includedirs("deps/qwindowkit/include/QWindowKit")
            add_linkdirs("deps/qwindowkit/lib")
            add_links("QWKCore", "QWKWidgets")
            add_rpathdirs("$(projectdir)/deps/qwindowkit/lib")
        end
        set_enabled(not is_plat("wasm"))
        add_runenvs("TEXMACS_PATH", path.join(os.projectdir(), "TeXmacs"))
        set_group("tests")
        add_deps(dep1)
        add_deps(dep2)
        set_languages("c++17")
        set_policy("check.auto_ignore_flags", false)
        set_encodings("utf-8") -- eliminate warning C4819 on msvc
        if is_plat("windows") then
            add_ldflags("/LTCG")
            set_runtimes("MT")
        end
        if is_plat("windows", "mingw") then
            add_syslinks("secur32")
        end
        add_rules("qt.console")
        add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtTest")
        if not is_plat("windows") then
            add_syslinks("pthread")
        end
        add_packages("lolly")
        add_packages("moebius")
        add_packages("s7")
        add_packages("liii-pdfhummus")

        add_includedirs({"$(buildir)", "tests/Base"})
        add_includedirs(moe_includedirs)
        add_includedirs(libstem_headers)
        build_glue_on_config()
        add_files("tests/Base/base.cpp")
        add_files(filepath)
        add_files(filepath, {rules = "qt.moc"})
        before_build(function (target)
            target:add("forceincludes", path.absolute("$(buildir)/config.h"))
        end)

        if is_plat("wasm") then
            on_run(function (target)
                node = os.getenv("EMSDK_NODE")
                cmd = node .. " $(buildir)/wasm/wasm32/$(mode)/" .. testname .. ".js"
                print("> " .. cmd)
                os.exec(cmd)
            end)
        end
    end
end

function add_target_cpp_bench(filepath, dep)
    local testname = path.basename(filepath)
    target(testname) do
        set_enabled(not is_plat("wasm"))
        add_runenvs("TEXMACS_PATH", path.join(os.projectdir(), "TeXmacs"))
        set_group("benchmarks")
        add_deps(dep)
        set_languages("c++17")
        set_policy("check.auto_ignore_flags", false)
        set_encodings("utf-8")
        if is_plat("windows") then
            add_ldflags("/LTCG")
            set_runtimes("MT")
        end
        if is_plat("windows", "mingw") then
            add_syslinks("secur32")
        end
        add_rules("qt.console")
        add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtTest")
        if not is_plat("windows") then
            add_syslinks("pthread")
        end
        add_packages("lolly")
        add_packages("moebius")
        add_packages("s7")
        add_packages("liii-pdfhummus")

        add_includedirs({"$(buildir)", "tests/Base"})
        add_includedirs(moe_includedirs)
        add_includedirs(libstem_headers)
        build_glue_on_config()
        add_files("tests/Base/base.cpp")
        add_files(filepath)
        add_files(filepath, {rules = "qt.moc"})
        before_build(function (target)
            target:add("forceincludes", path.absolute("$(buildir)/config.h"))
        end)
    end
end

function add_target_scheme_test(filepath, INSTALL_DIR, RUN_ENVS)
    local testname = path.basename(filepath)
    target(testname) do
        set_enabled(not is_plat("wasm"))
        set_kind("phony")
        set_group("scheme_tests")
        add_deps("stem")
        add_runenvs("TEXMACS_PATH", path.join(os.projectdir(), "TeXmacs"))
        INSTALL_DIR = INSTALL_DIR or os.projectdir()
        on_run(function (target)
            name = target:name()
            regtest_name = "(regtest-"..string.sub(name, 1, -6)..")"
            print("------------------------------------------------------")
            print("Executing: " .. regtest_name)
            params = {
                "-headless",
                "-b", filepath,
                "-x", regtest_name,
                "-q"
            }
            if is_plat("macosx", "linux") then
                binary = target:deps()["stem"]:targetfile()
            elseif is_plat("mingw", "windows") then
                binary = path.join(INSTALL_DIR, "build", "packages", "stem", "data", "bin", "MoganSTEM.exe")
            else
                print("Unsupported plat $(plat)")
            end
            cmd = binary
            if is_plat("macosx", "linux") then
                os.execv(cmd, params, {envs=RUN_ENVS})
            else
                os.execv(cmd, params)
            end
        end)
    end
end

function add_target_integration_test(filepath, INSTALL_DIR, RUN_ENVS)
    local testname = path.basename(filepath)
    target(testname) do
        set_enabled(not is_plat("wasm"))
        set_kind("phony")
        set_group("integration_tests")
        add_deps("stem")
        add_runenvs("TEXMACS_PATH", path.join(os.projectdir(), "TeXmacs"))
        INSTALL_DIR = INSTALL_DIR or os.projectdir()
        on_run(function (target)
            name = target:name()
            test_name = "(test_"..name..")"
            print("------------------------------------------------------")
            print("Executing: " .. test_name)
            params = {
                "-headless",
                "-b", path.join("TeXmacs","tests",name..".scm"),
                "-x", test_name,
                "-q"
            }
            if is_plat("macosx", "linux") then
                binary = target:deps()["stem"]:targetfile()
            elseif is_plat("mingw", "windows") then
                binary = path.join(INSTALL_DIR, "build", "packages", "stem", "data", "bin", "MoganSTEM.exe")
            else
                print("Unsupported plat $(plat)")
            end
            cmd = binary
            if is_plat("macosx", "linux") then
                os.execv(cmd, params, {envs=RUN_ENVS})
            else
                os.execv(cmd, params)
            end
        end)
    end
end 