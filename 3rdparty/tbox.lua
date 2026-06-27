-------------------------------------------------------------------------------
--
-- MODULE      : tbox.lua
-- DESCRIPTION : Xmake package definition for tbox
--
-- Derived from xmake-io/xmake-repo (packages/t/tbox/xmake.lua).
-- Use the vendored source under 3rdparty/tbox.
--

package("liii-tbox")
    set_homepage("https://tboox.org")
    set_description("A glib-like multi-platform c library")

    set_sourcedir(path.join(os.scriptdir(), "tbox"))

    add_configs("micro",      {description = "Compile micro core library for the embed system.", default = false, type = "boolean"})
    add_configs("float",      {description = "Enable or disable the float type.", default = true, type = "boolean"})
    add_configs("force-utf8", {description = "Forcely regard all tb_char* as utf-8.", default = true, type = "boolean"})
    add_configs("hash",       {description = "Enable hash", default = true, type="boolean"})
    add_configs("charset",    {description = "Enable charset", default = true, type="boolean"})
    for _, name in ipairs({"xml", "zip", "regex", "object", "database", "coroutine"}) do
        add_configs(name, {description = "Enable the " .. name .. " module.", default = false, type = "boolean"})
    end
    for _, name in ipairs({"zlib", "mysql", "sqlite3", "openssl", "polarssl", "mbedtls", "pcre2", "pcre"}) do
        add_configs(name, {description = "Enable the " .. name .. " package.", default = false, type = "boolean"})
    end

    if is_plat("windows") then
        add_syslinks("ws2_32", "user32", "kernel32")
    elseif is_plat("mingw") then
        add_syslinks("ws2_32", "pthread")
    elseif is_plat("macosx", "iphoneos") then
        add_frameworks("Foundation", "CoreServices", "CoreFoundation")
    elseif is_plat("linux") then
        add_syslinks("pthread", "m", "dl")
    elseif is_plat("bsd") then
        add_syslinks("execinfo", "pthread", "m", "dl")
    elseif not is_plat("android") then
        add_syslinks("pthread")
    end

    on_load(function (package)
        if package:debug() then
            package:add("defines", "__tb_debug__")
        end
        for _, dep in ipairs({"mbedtls", "openssl", "sqlite3", "pcre2", "pcre", "mysql", "zlib"}) do
            if package:config(dep) then
                package:add("deps", dep)
            end
        end
    end)

    on_install(function (package)
        local configs = {demo = false}
        if package:config("micro") then
            configs.micro = true
        end
        if not package:config("float") then
            configs["float"] = false
        end
        if package:config("force-utf8") then
            configs["force-utf8"] = true
        end
        for _, name in ipairs({"xml", "zip", "hash", "regex", "object", "charset", "database", "coroutine"}) do
            if package:config(name) then
                configs[name] = true
            end
        end
        for _, name in ipairs({"zlib", "mysql", "sqlite3", "openssl", "polarssl", "mbedtls", "pcre2", "pcre"}) do
            if package:config(name) then
                configs[name] = true
            end
        end
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:has_cfuncs("tb_exit", {includes = "tbox/tbox.h", configs = {languages = "c99"}}))
    end)
package_end()
