-------------------------------------------------------------------------------
--
-- MODULE      : xmake.lua
-- DESCRIPTION : Xmake config file for TeXmacs
-- COPYRIGHT   : (C) 2022       jingkaimori
--                   2022-2024  Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

includes("xmake/vars.lua")
includes("3rdparty/doctest.lua")
includes("3rdparty/libaesgm.lua")
includes("3rdparty/pdfhummus.lua")
includes("3rdparty/tbox.lua")

includes("moebius")
includes("lolly")
includes("xmake/stem.lua")
includes("xmake/rules/glue.lua")
includes("xmake/goldfish.lua")
includes("xmake/velopack.lua")

set_project(stem_project_name)
set_policy("run.autobuild", false)
set_languages("c++17")
set_encodings("utf-8")

TBOX_VERSION= "1.7.5"
S7_VERSION = "20240816"
local LIBICONV_VERSION = "1.17"

add_repositories("liii-repo xmake")

-- because this cpp project use variant length arrays which is not supported by
-- msvc, this project will not support windows env.
-- because some package is not ported to cygwin env, this project will not
-- support cygwin env.
set_allowedplats("linux", "macosx", "windows", "wasm")

-- add releasedbg, debug and release modes for different platforms.
-- debug mode cannot run on mingw with qt precompiled binary
set_allowedmodes("releasedbg", "release", "debug")

add_rules("mode.releasedbg", "mode.release", "mode.debug")

set_config("debug_with_timestamp", true)

if is_plat("windows") then
    set_runtimes("MT")
end

if is_plat("linux") then
    set_configvar("OS_GNU_LINUX", true)
else
    set_configvar("OS_GNU_LINUX", false)
end
if is_plat("macosx") then
    set_configvar("OS_MACOS", true)
else
    set_configvar("OS_MACOS", false)
end

-- Work around Qt 6.8.3 bug on Apple Silicon (QTBUG-133471):
-- qyieldcpu.h uses __yield() without including <arm_acle.h>.
if is_plat("macosx") and is_arch("arm64") then
    add_cxxflags("-Wno-error=implicit-function-declaration")
    add_mxflags("-Wno-error=implicit-function-declaration")
end
if is_plat("windows") then
    set_configvar("OS_WIN", true)
else
    set_configvar("OS_WIN", false)
end

if is_plat("wasm") then
    set_configvar("OS_WASM", true)
    add_requires("emscripten 3.1.74")
    set_toolchains("emcc@emscripten")
else
    set_configvar("OS_WASM", false)
end

includes("@builtin/check")
configvar_check_cxxtypes("HAVE_INTPTR_T", "intptr_t", {includes = {"memory"}})
configvar_check_cxxincludes("HAVE_INTTYPES_H", "inttypes.h")
configvar_check_cxxincludes("HAVE_STDINT_H", "stdint.h")

set_configvar("USE_FREETYPE", 1)

includes("xmake/options.lua")

includes("xmake/requires.lua")

includes("xmake/targets/stem.lua")
includes("xmake/targets/libmogan.lua")
includes("xmake/targets/stem_packager.lua")
includes("xmake/targets/web_shell.lua")
if has_config("qt_frontend") then
    includes("xmake/targets/qwkcore.lua")
    includes("xmake/targets/qwkwidgets.lua")
else
    includes("xmake/targets/imgui.lua")
end

if has_config("loro") then
    includes("xmake/targets/loro.lua")
end

if is_plat("windows") then
    includes("xmake/targets/liii_windows_icon.lua")
end

if is_mode("release") then
    includes("xmake/targets/xpack.lua")
end

if has_config("qt_frontend") then
    includes("xmake/tests.lua")
    -- Tests in C++
    all_cpp_tests = os.files("tests/**_test.cpp")
    cpp_benches = os.files("bench/**_bench.cpp")

    for _, filepath in ipairs(cpp_benches) do
        add_target_cpp_bench(filepath, "libmogan")
    end

    if not (is_plat("linux") and (linuxos.name () == "ubuntu" and linuxos.version():major() == 20)) then
        for _, filepath in ipairs(all_cpp_tests) do
            if not string.find(filepath, "tests/L3/") then
                add_target_cpp_test(filepath, "libmogan", "libmoebius")
            end
        end
    end

    -- Tests in Scheme
    for _, filepath in ipairs(os.files("TeXmacs/progs/**/*-test.scm")) do
        add_target_scheme_test(filepath, INSTALL_DIR, RUN_ENVS)
    end

    for _, filepath in ipairs(os.files("TeXmacs/progs/kernel/**/*-test.scm")) do
        add_target_scheme_test(filepath, INSTALL_DIR, RUN_ENVS)
    end

    -- Integration tests
    for _, filepath in ipairs(os.files("TeXmacs/tests/*.scm")) do
        add_target_integration_test(filepath, INSTALL_DIR, RUN_ENVS)
    end
end -- has_config("qt_frontend")
