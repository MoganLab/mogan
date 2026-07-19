-------------------------------------------------------------------------------
--
-- MODULE      : goldfish.lua
-- DESCRIPTION : goldfish scheme (package + binary target)
-- COPYRIGHT   : (C) 2025  Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
--

-- goldfish 的 s7 源码内置在 TeXmacs/plugins/goldfish/src/，与下面 goldfish
-- binary target 内联编译的那一份完全相同。不使用 set_sourcedir() 直接指向
-- 工作树（否则 package.tools.xmake 会把 port 出来的 xmake.lua 写进工作树，
-- 污染源码目录），而是在 on_install 里把源码拷贝到包缓存目录后构建。

target("libgoldfish") do
    set_kind("static")
    set_languages("c11")
    add_packages("liii-tbox")
    add_packages("argh", {public = true})
    add_defines("WITH_SYSTEM_EXTRAS=0")
    if not is_plat("wasm") then
        add_defines("HAVE_OVERFLOW_CHECKS=0")
    end
    add_defines("WITH_WARNINGS")
    add_defines("WITH_R7RS=1")
    set_basename("libgoldfish")
    -- s7 内部有与 GLib 同名的全局函数（g_log 等），静态链入 stem 后会被
    -- --export-dynamic 导出，运行时被 GLib 抢占导致崩溃。隐藏这些符号，
    -- 使其不进 binary 的 .dynsym（参考 qwkcore.lua / qwkwidgets.lua 同款做法）。
    if not is_plat("windows") then
        add_cflags("-fPIC", "-fvisibility=hidden")
    end
    add_files(
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_continuation.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_ctables.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_dtoa.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_module.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_op_names.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_base.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_char.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_complex.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_format.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_inexact.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_predicate.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_symbol.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_write.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_bitwise.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_hash_table.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_list.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_string.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_vector.c"
    )
    add_headerfiles("$(projectdir)/TeXmacs/plugins/goldfish/src/s7.h")
    if is_plat("windows") then
        set_optimize("faster")
        add_cxxflags("/fp:precise")
    end
    if is_mode("debug") then
        add_defines("S7_DEBUGGING")
    end
end

target ("goldfish") do
    set_kind("static")
    set_languages("c++17")
    add_deps("libgoldfish")
    add_files({
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_base64.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_hashlib.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_njson.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_os.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_path.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_subprocess.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/scheme_base.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/scheme_char.cpp",
    })
    add_files({
        "$(projectdir)/3rdparty/json-schema-validator/src/smtp-address-validator.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-schema-draft7.json.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-uri.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-validator.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-patch.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/string-format-check.cpp",
    })
    if not is_plat("wasm") then
        add_files ("$(projectdir)/TeXmacs/plugins/goldfish/src/liii_http.cpp")
        add_defines("GOLDFISH_ENABLE_HTTP")
    end
    add_includedirs({
        "$(projectdir)/TeXmacs/plugins/goldfish/src",
        "$(projectdir)/3rdparty/nlohmann_json/include",
        "$(projectdir)/3rdparty/json-schema-validator/src",
    }, {public = true})

    -- 同 libgoldfish：隐藏 goldfish 内部符号，避免与 GLib 的 g_log 等同名符号
    -- 冲突（见 libgoldfish target 注释）。
    if not is_plat("windows") then
        add_cxxflags("-fPIC", "-fvisibility=hidden", "-fvisibility-inlines-hidden")
    end

    add_defines("WITH_SYSTEM_EXTRAS=0")
    if not is_plat("wasm") then
        add_defines("HAVE_OVERFLOW_CHECKS=0")
    end
    add_defines("WITH_WARNINGS")
    add_defines("WITH_R7RS=1")
    if is_mode("debug") then
        add_defines("S7_DEBUGGING")
    end

    if is_plat("linux") then
        add_syslinks("stdc++")
    end
    add_packages("liii-tbox")
    if not is_plat("wasm") then
        add_packages("cpr")
    end
    add_packages("argh", {public = true})
end

target ("goldfish-bin") do
    set_kind("binary")
    add_deps("goldfish")
    set_basename("goldfish")
    add_packages("liii-tbox")
    add_packages("argh", {public = true})
    set_targetdir("$(projectdir)/TeXmacs/plugins/goldfish/bin/")
    add_files ("$(projectdir)/TeXmacs/plugins/goldfish/src/goldfish.cpp")
end
