-- 本地覆盖 xmake-repo 的 cpr 配方（上游见 ~/git/xmake-repo/packages/c/cpr/xmake.lua），
-- 差异仅在 libcurl 依赖：
-- 1. 上游 ssl=true 时给 libcurl 加 {libssh2=true, zlib=true} 并额外依赖 libssh2，
--    与顶层 add_requires("libcurl")（无额外 configs）不是同一实例，会产生第二个
--    静态 curl 构建；libmogan 同时链 goldfish(cpr→curl) 和 libcurl（Loro WebSocket）
--    时两份 curl 符号冲突。
-- 2. goldfish 只需要 HTTPS，不需要 libssh2/zlib，统一依赖不带额外 configs 的
--    libcurl（本仓库 3rdparty/curl-8.21 源码构建，见 xmake/packages/l/libcurl），
--    避免再引入系统 libcurl 与静态 openssl 混用导致的段错误（见 devel/2092.md）。
package("cpr")
    set_homepage("https://docs.libcpr.org/")
    set_description("C++ Requests is a simple wrapper around libcurl inspired by the excellent Python Requests project.")
    set_license("MIT")

    set_urls("https://github.com/libcpr/cpr/archive/refs/tags/$(version).tar.gz",
             "https://github.com/libcpr/cpr.git")

    add_versions("1.14.2", "b9b529b47083bfe80bba855ca5308d12d767ae7c7b629aef5ef018c4343cf62b")
    add_versions("1.14.1", "213ccc7c98683d2ca6304d9760005effa12ec51d664bababf114566cb2b1e23c")
    add_versions("1.12.0", "f64b501de66e163d6a278fbb6a95f395ee873b7a66c905dd785eae107266a709")
    add_versions("1.11.2", "3795a3581109a9ba5e48fbb50f9efe3399a3ede22f2ab606b71059a615cd6084")

    add_configs("ssl", {description = "Enable SSL.", default = false, type = "boolean"})

    add_deps("cmake")
    if is_plat("linux") then
        add_syslinks("pthread")
    end
    add_links("cpr")

    on_load(function (package)
        -- TLS 能力由 libcurl 自身（openssl）提供，cpr 只是透传 CPR_ENABLE_SSL；
        -- 不带额外 configs，保证与顶层 add_requires("libcurl") 解析为同一实例
        package:add("deps", "libcurl")
    end)

    on_install("!wasm and !bsd", function (package)
        io.replace("CMakeLists.txt", "-Werror", "", {plain = true})
        if package:is_plat("windows") or (package:is_plat("android") and is_subhost("windows")) then
            -- fix find_package issue on windows
            io.replace("CMakeLists.txt", "find_package%(CURL COMPONENTS .-%)", "find_package(CURL)")
        end

        local configs = {
            "-DCPR_BUILD_TESTS=OFF",
            "-DCPR_FORCE_USE_SYSTEM_CURL=ON",
            "-DCPR_USE_SYSTEM_CURL=ON",
        }
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        table.insert(configs, "-DCPR_ENABLE_SSL=" .. (package:config("ssl") and "ON" or "OFF"))

        local opt = {}
        opt.packagedeps = {"libcurl"}
        if package:is_plat("windows") and package:has_tool("cxx", "cl", "clang_cl") then
            opt.cxflags = {"/EHsc"}
        end
        if package:config("shared") and package:is_plat("macosx") then
            opt.shflags = {"-framework", "CoreFoundation", "-framework", "Security", "-framework", "SystemConfiguration"}
        end
        import("package.tools.cmake").install(package, configs, opt)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <cassert>
            #include <cpr/cpr.h>
            static void test() {
                cpr::Response r = cpr::Get(cpr::Url{"https://xmake.io"});
                assert(r.status_code == 200);
            }
        ]]}, {configs = {languages = "c++17"}}))
    end)
