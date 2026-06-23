package("s7")
    set_homepage("https://ccrma.stanford.edu/software/snd/snd/s7.html")
    set_description("s7 is a Scheme interpreter intended as an extension language for other applications.")

    -- s7 的源码随 goldfish 插件内置在 TeXmacs/plugins/goldfish/src/，与 goldfish
    -- 目标内联编译的那一份完全相同。这里不使用 set_sourcedir() 直接指向工作树
    -- （否则 package.tools.xmake 会把 port 出来的 xmake.lua 写进工作树，污染源码目录），
    -- 而是在 on_install 里把源码拷贝到包缓存目录后再构建。
    local goldfish_s7_src = path.join(os.scriptdir(), "../../../../TeXmacs/plugins/goldfish/src")

    add_configs("gmp", {description = "enable gmp support", default = false, type = "boolean"})

    on_load(function (package)
        package:addenv("PATH", "bin")
        if package:config("gmp") then
            package:add("deps", "gmp")
        end
    end)

    if is_plat("linux") then
        add_syslinks("pthread", "dl", "m")
    end

    on_install("bsd", "cross", "cygwin", "linux", "macosx", "mingw", "msys", "wasm", "windows", function (package)
        -- 在包缓存目录中构建，保持工作树干净。先把内置的 s7 源码和我们的
        -- port/xmake.lua（定义了包含完整源文件集合的 libs7 静态库 target）拷进缓存目录，
        -- 再交给 package.tools.xmake 构建。这里必须由我们自己放置 xmake.lua，否则 xmake
        -- 会自动生成默认工程（把 s7.c 误当成二进制 target 链接，报缺少 _main）。
        -- {curdir = curdir} 告诉 package.tools.xmake 在该缓存目录下执行 xmake f/build/install。
        local curdir = package:cachedir()
        os.cp(path.join(goldfish_s7_src, "*.c"), curdir)
        os.cp(path.join(goldfish_s7_src, "*.h"), curdir)
        os.cp(path.join(os.scriptdir(), "port", "xmake.lua"), path.join(curdir, "xmake.lua"))
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        import("package.tools.xmake").install(package, configs, {curdir = curdir})
    end)

    on_test(function(package)
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
