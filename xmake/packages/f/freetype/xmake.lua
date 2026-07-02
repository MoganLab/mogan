package("freetype")
    set_homepage("https://www.freetype.org")
    set_description("A freely available software library to render fonts.")
    set_license("BSD") -- FreeType License (FTL) is a BSD-style license

    add_urls("https://downloads.sourceforge.net/project/freetype/freetype2/$(version)/freetype-$(version).tar.gz",
             "https://download.savannah.gnu.org/releases/freetype/freetype-$(version).tar.gz", {alias="archive"})
    add_urls("https://gitlab.freedesktop.org/freetype/freetype.git",
             "https://github.com/freetype/freetype.git", {alias = "git"})

    add_versions("archive:2.13.3", "5c3a8e78f7b24c20b25b54ee575d6daa40007a5f4eea2845861c3409b3021747")
    add_versions("git:2.13.3", "VER-2-13-3")

    if not is_host("windows") then
        add_extsources("pkgconfig::freetype2")
    end

    if is_plat("mingw") and is_subhost("msys") then
        add_extsources("pacman::freetype")
    elseif is_plat("linux") then
        add_extsources("pacman::freetype2", "apt::libfreetype-dev")
    elseif is_plat("macosx") then
        add_extsources("brew::freetype")
    end

    add_configs("bzip2", {description = "Support bzip2 compressed fonts", default = false, type = "boolean"})
    add_configs("png", {description = "Support PNG compressed OpenType embedded bitmaps", default = false, type = "boolean"})
    add_configs("woff2", {description = "Use Brotli library to support decompressing WOFF2 fonts", default = false, type = "boolean"})
    add_configs("zlib", {description = "Support reading gzip-compressed font files", default = true, type = "boolean"})
    add_configs("harfbuzz", {description = "Support harfbuzz", default = false, type = "boolean"})

    add_deps("cmake")
    if is_plat("windows", "mingw") and is_subhost("windows") then
        add_deps("pkgconf")
    elseif is_plat("wasm") then
        add_configs("shared", {description = "Build shared library.", default = false, type = "boolean", readonly = true})
    end

    add_includedirs("include/freetype2")

    on_load(function (package)
        local function add_configdep(conf, pkg, depconf)
            if package:config(conf) then
                package:add("deps", pkg or conf, depconf)
            end
        end

        add_configdep("bzip2")
        add_configdep("zlib")
        add_configdep("png", "libpng")
        add_configdep("woff2", "brotli")
        add_configdep("harfbuzz", nil, {configs = {freetype = false}}) -- we have to disable freetype in harfbuzz to prevent a circular dependency
    end)

    on_install(function (package)
        local configs = {"-DCMAKE_INSTALL_LIBDIR=lib"}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=" .. (package:config("shared") and "ON" or "OFF"))
        if package:is_plat("wasm") then
            table.insert(configs, "-DCMAKE_C_FLAGS=-matomics -mbulk-memory -pthread")
            table.insert(configs, "-DCMAKE_CXX_FLAGS=-matomics -mbulk-memory -pthread")
            table.insert(configs, "-DCMAKE_EXE_LINKER_FLAGS=-matomics -mbulk-memory -pthread -sUSE_PTHREADS=1 -sSHARED_MEMORY=1")
            table.insert(configs, "-DCMAKE_SHARED_LINKER_FLAGS=-matomics -mbulk-memory -pthread -sUSE_PTHREADS=1 -sSHARED_MEMORY=1")
        end
        local function add_dep(opt)
            if package:config(opt.conf) then
                if package:version():ge("2.11.1") then
                    table.insert(configs, "-DFT_REQUIRE_" .. opt.cmakewith .. "=ON")
                else
                    table.insert(configs, "-DFT_WITH_" .. opt.cmakewith .. "=ON")
                end

                local lib = package:dep(opt.pkg or opt.conf)
                if lib and not lib:is_system() then
                    local fetchinfo = lib:fetch()
                    if fetchinfo then
                        local includedirs = fetchinfo.includedirs or fetchinfo.sysincludedirs
                        if includedirs and #includedirs > 0 then
                            local includeconfs = opt.cmakeinclude and table.wrap(opt.cmakeinclude) or {opt.cmakewith .. "_INCLUDE_DIRS"}
                            for _, includeconf in ipairs(includeconfs) do
                                table.insert(configs, "-D" .. includeconf .. "=" .. table.concat(fetchinfo.includedirs or fetchinfo.sysincludedirs, ";"))
                            end
                        end
                        -- libfiles may include .dll (https://github.com/xmake-io/xmake-repo/pull/8155)
                        local libfiles = table.remove_if(table.clone(fetchinfo.libfiles or {}), function (i, file) return path.extension(file):lower() == ".dll" end)
                        if #libfiles > 0 then
                            local libconfs = opt.cmakelib and table.wrap(opt.cmakelib) or {opt.cmakewith .. "_LIBRARIES"}
                            for _, libconf in ipairs(libconfs) do
                                table.insert(configs, "-D" .. libconf .. "=" .. table.concat(libfiles, ";"))
                            end
                        end
                    end
                end
            else
                if package:version():ge("2.11.1") then
                    table.insert(configs, "-DFT_DISABLE_" .. opt.cmakewith .. "=ON")
                else
                    table.insert(configs, "-DCMAKE_DISABLE_FIND_PACKAGE_" .. (opt.cmakedisable or opt.cmakewith) .. "=ON")
                end
            end
        end
        add_dep({conf = "bzip2", cmakewith = "BZIP2", cmakedisable = "BZip2", cmakeinclude = "BZIP2_INCLUDE_DIR", cmakelib = {"BZIP2_LIBRARIES", "BZIP2_LIBRARY"}}) -- there seem to be an error in FindBZip2.cmake
        add_dep({conf = "png", pkg = "libpng", cmakewith = "PNG", cmakeinclude = "PNG_PNG_INCLUDE_DIR", cmakelib = "PNG_LIBRARY"})
        add_dep({conf = "woff2", pkg = "brotli", cmakewith = "BROTLI", cmakedisable = "BrotliDec", cmakeinclude = "BROTLIDEC_INCLUDE_DIRS", cmakelib = "BROTLIDEC_LIBRARIES"})
        add_dep({conf = "zlib", cmakewith = "ZLIB", cmakeinclude = "ZLIB_INCLUDE_DIR", cmakelib = "ZLIB_LIBRARY"})
        add_dep({conf = "harfbuzz", pkg = "harfbuzz", cmakewith = "HARFBUZZ", cmakedisable = "HarfBuzz", cmakeinclude = "HarfBuzz_INCLUDE_DIR", cmakelib = "HarfBuzz_LIBRARY"})

        import("package.tools.cmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:has_cfuncs("FT_Init_FreeType", {includes = {"ft2build.h", "freetype/freetype.h"}}))
    end)
