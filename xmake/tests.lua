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
        add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtTest", "QtNetwork")
        add_frameworks("QtQml", "QtQuick", "QtBodymovin")
        if not is_plat("windows") then
            add_syslinks("pthread")
        end
        add_packages("goldfish")
        add_packages("liii-pdfhummus")

        add_includedirs({"$(builddir)", "tests/Base"})
        add_includedirs(libstem_headers)
        add_rules("mogan.glue")
        add_files("src/Scheme/**/glue_*.lua", {rule = "mogan.glue"})
        add_files("tests/Base/base.cpp")
        add_files(filepath)
        add_files(filepath, {rules = "qt.moc"})
        on_load(function (target)
            target:add("forceincludes", path.absolute("$(builddir)/config.h"))
            target:add("forceincludes", path.absolute("$(builddir)/tm_configure.hpp"))
        end)

        if is_plat("wasm") then
            on_run(function (target)
                node = os.getenv("EMSDK_NODE")
                cmd = node .. " $(builddir)/wasm/wasm32/$(mode)/" .. testname .. ".js"
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
        add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtTest", "QtNetwork")
        add_frameworks("QtQml", "QtQuick", "QtBodymovin")
        if not is_plat("windows") then
            add_syslinks("pthread")
        end
        add_packages("goldfish")
        add_packages("liii-pdfhummus")

        add_includedirs({"$(builddir)", "tests/Base"})
        add_includedirs(libstem_headers)
        add_rules("mogan.glue")
        add_files("src/Scheme/**/glue_*.lua", {rule = "mogan.glue"})
        add_files("tests/Base/base.cpp")
        add_files(filepath)
        add_files(filepath, {rules = "qt.moc"})
        on_load(function (target)
            target:add("forceincludes", path.absolute("$(builddir)/config.h"))
            target:add("forceincludes", path.absolute("$(builddir)/tm_configure.hpp"))
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
                "-d",
                "-b", filepath,
                "-x", "(catch #t (lambda () " .. regtest_name .. " (quit-TeXmacs)) (lambda args (display \"Error: \") (display args) (newline) (exit 1)))"
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
            local name = target:name()
            local test_name = "(test_"..name..")"
            print("------------------------------------------------------")
            -- MOGAN_TEST_GUI=1: 真实 GUI 进程跑（无 -headless），驱动 GUI 专属
            -- 路径，调试日志进终端；不自动 quit，由测试脚本自己延迟退出。
            local gui_mode = os.getenv("MOGAN_TEST_GUI") == "1"
            print(("Executing: %s (mode: %s)"):format(test_name, gui_mode and "GUI" or "headless"))
            local scm = path.join("TeXmacs","tests",name..".scm")
            -- GUI 模式不追加 (quit-TeXmacs)，让测试可串异步链后自退。
            local quit = gui_mode and "" or " (quit-TeXmacs)"
            local expr = ("(catch #t (lambda () %s%s) (lambda args (display \"Error: \") (display args) (newline) (exit 1)))"):format(test_name, quit)
            local params = gui_mode and {"-d", "-b", scm, "-x", expr}
                                      or {"-headless", "-d", "-b", scm, "-x", expr}
            local binary
            if is_plat("macosx", "linux") then
                binary = target:deps()["stem"]:targetfile()
            elseif is_plat("mingw", "windows") then
                binary = path.join(INSTALL_DIR, "build", "packages", "stem", "data", "bin", "MoganSTEM.exe")
            else
                print("Unsupported plat $(plat)")
                return
            end
            os.execv(binary, params, is_plat("macosx", "linux") and {envs=RUN_ENVS} or nil)
        end)
    end
end 
