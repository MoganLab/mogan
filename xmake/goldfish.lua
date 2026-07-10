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
local goldfish_src = "$(projectdir)/TeXmacs/plugins/goldfish/src"

package("goldfish")
    set_homepage("https://github.com/goldfishscheme/goldfish")
    set_description("Goldfish Scheme: a Scheme interpreter intended as an extension language for other applications.")

    add_deps("liii-tbox")

    on_load(function (package)
        package:addenv("PATH", "bin")
        package:add("deps", "argh v1.3.2")
    end)

    on_install("bsd", "cross", "cygwin", "linux", "macosx", "mingw", "msys", "wasm", "windows", function (package)
        -- 在包缓存目录中构建，保持工作树干净。先把内置的 s7 源码和我们的
        -- port/xmake.lua（定义了 libgoldfish 静态库 target）拷进缓存目录，
        -- 再交给 package.tools.xmake 构建。这里必须由我们自己放置 xmake.lua，
        -- 否则 xmake 会自动生成默认工程（把 s7.c 误当成二进制 target 链接，
        -- 报缺少 _main）。
        -- {curdir = curdir} 告诉 package.tools.xmake 在该缓存目录下执行
        -- xmake f/build/install。
        local curdir = package:cachedir()
        os.cp(path.join(goldfish_src, "*.c"), curdir)
        os.cp(path.join(goldfish_src, "*.h"), curdir)
        io.writefile(path.join(curdir, "xmake.lua"), [[
add_rules("mode.release", "mode.debug")

target("libgoldfish") do
    set_kind("$(kind)")
    set_languages("c11")
    add_defines("WITH_SYSTEM_EXTRAS=0")
    if not is_plat("wasm") then
        add_defines("HAVE_OVERFLOW_CHECKS=0")
    end
    add_defines("WITH_WARNINGS")
    add_defines("WITH_R7RS=1")
    set_basename("goldfish")
    add_files(
        "s7.c",
        "s7_continuation.c",
        "s7_ctables.c",
        "s7_dtoa.c",
        "s7_module.c",
        "s7_op_names.c",
        "s7_scheme_base.c",
        "s7_scheme_char.c",
        "s7_scheme_complex.c",
        "s7_scheme_format.c",
        "s7_scheme_inexact.c",
        "s7_scheme_predicate.c",
        "s7_scheme_symbol.c",
        "s7_scheme_write.c",
        "s7_liii_bitwise.c",
        "s7_liii_hash_table.c",
        "s7_liii_list.c",
        "s7_liii_string.c",
        "s7_liii_vector.c"
    )
    add_headerfiles("$(curdir)/s7.h")
    add_includedirs(".", {public = true})
    if is_plat("windows") then
        set_optimize("faster")
        add_cxxflags("/fp:precise")
    end
    if is_mode("debug") then
        add_defines("S7_DEBUGGING")
    end
    add_packages("liii-tbox")
end
]])
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        import("package.tools.xmake").install(package, configs, {curdir = curdir})
    end)

    on_test(function (package)
        assert(package:check_csnippets([[
            static s7_pointer old_add;           /* the original "+" function for non-string cases */
            static s7_pointer old_string_append; /* same, for "string-append" */

            static s7_pointer our_add(s7_scheme *sc, s7_pointer args)
            {
                /* this will replace the built-in "+" operator, extending it to include strings:
                *   (+ "hi" "ho") -> "hiho" and  (+ 3 4) -> 7
                */
                if ((s7_is_pair(args)) &&
                    (s7_is_string(s7_car(args))))
                    return(s7_apply_function(sc, old_string_append, args));
                return(s7_apply_function(sc, old_add, args));
            }
        ]], {includes = "s7.h"}))
    end)
package_end()

target ("goldfish") do
    set_languages("c++17")
    set_targetdir("$(projectdir)/TeXmacs/plugins/goldfish/bin/")
    add_files ("$(projectdir)/TeXmacs/plugins/goldfish/src/goldfish.cpp")
    add_files({
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_base64.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_hashlib.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_njson.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_os.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_path.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_subprocess.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/scheme_base.cpp",
    })
    add_files({
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
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_vector.c",
    }, {languages = "c11"})
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
    })

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
    add_packages("argh")
    on_install(function (target)
    end)
end
