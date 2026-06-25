-------------------------------------------------------------------------------
--
-- MODULE      : libaesgm.lua
-- DESCRIPTION : Xmake package definition for libaesgm
--

package("liii-libaesgm")
    set_homepage("https://github.com/xmake-mirror/libaesgm")
    set_description("https://repology.org/project/libaesgm/packages")

    set_sourcedir(path.join(os.scriptdir(), "libaesgm"))

    on_install("linux", "macosx", "windows", "mingw", function (package)
        if package:is_plat("windows", "mingw") and package:is_arch("arm", "arm64") then
            -- Windows is always little endian
            io.replace("brg_endian.h", [[
#elif 0     /* **** EDIT HERE IF NECESSARY **** */
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN]], [[
#elif 1     /* Edited: Windows ARM is little endian */
#  define PLATFORM_BYTE_ORDER IS_LITTLE_ENDIAN]], { plain = true })
        end
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:has_cfuncs("aes_init", {includes = "aes.h"}))
    end)
package_end()
