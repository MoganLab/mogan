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
includes("xmake/stem.lua")

set_project(stem_project_name)
set_policy("run.autobuild", false)
set_languages("c++17")
set_encodings("utf-8")

add_requires("s7", {system=false})
add_requires("tbox", {system=false})
add_requires("cpr", {system=false})
includes("xmake/goldfish.lua")

option("mupdf")
    set_default(true)
    set_description("Enable MuPDF library")
option_end()

-- Temporary statement to move into MuPDF
set_config("mupdf", true)

-- Adjust community or commercial version
option("is_community")
    set_default(true)
    set_description("Adjust community or commercial version")
option_end()

set_config("is_community", is_community)

option("debug_with_timestamp")
    set_default(true)
    set_description("Enable timestamps in debug messages")
option_end()

set_config("debug_with_timestamp", true)

-- Generate build/config.h from template
add_configfiles("src/System/config.h.xmake", {
    filename = "config.h",
    variables = {
        SIZEOF_VOID_P = 8,
        VERSION = XMACS_VERSION,
        USE_FREETYPE = 1,
        USE_ICONV = true,
        USE_PLUGIN_GS = true,
        USE_PLUGIN_BIBTEX = true,
        USE_PLUGIN_LATEX_PREVIEW = true,
        USE_PLUGIN_TEX = true,
        USE_PLUGIN_ISPELL = true,
        USE_PLUGIN_PDF = true,
        USE_PLUGIN_SPARKLE = false,
        USE_PLUGIN_HTML = true,
        OS_MACOS = is_plat("macosx"),
        MACOSX_EXTENSIONS = is_plat("macosx"),
        QTTEXMACS = true,
        SANITY_CHECKS = true,
        OS_MINGW = is_plat("mingw"),
        OS_GNU_LINUX = is_plat("linux"),
        OS_WIN = is_plat("windows"),
        OS_WASM = is_plat("wasm"),
        NOMINMAX = true,
        QTPIPES = true,
        USE_QT_PRINTER = true,
        TM_DYNAMIC_LINKING = true,
        USE_FONTCONFIG = true,
        PDFHUMMUS_NO_TIFF = true,
        USE_MUPDF_RENDERER = has_config("mupdf"),
        IS_COMMUNITY = has_config("is_community"),
        DEBUG_WITH_TIMESTAMP = has_config("debug_with_timestamp"),
    }
})

-- because this cpp project use variant length arrays which is not supported by
-- msvc, this project will not support windows env.
-- because some package is not ported to cygwin env, this project will not
-- support cygwin env.
set_allowedplats("linux", "macosx", "windows")

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
if is_plat("windows") then
    set_configvar("OS_WIN", true)
else
    set_configvar("OS_WIN", false)
end

-- add releasedbg, debug and release modes for different platforms.
-- debug mode cannot run on mingw with qt precompiled binary
set_allowedmodes("releasedbg", "release", "debug")
add_rules("mode.releasedbg", "mode.release", "mode.debug")

add_repositories("liii-repo xmake")

TBOX_VERSION= "1.7.5"
LOLLY_VERSION= "1.4.26"
S7_VERSION = "20240816"

package("liii-libaesgm")
    set_homepage("https://github.com/xmake-mirror/libaesgm")
    set_description("https://repology.org/project/libaesgm/packages")

    set_sourcedir(path.join(os.scriptdir(), "3rdparty/libaesgm"))

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

PDFHUMMUS_VERSION = "4.6.2"
package("liii-pdfhummus")
    set_homepage("https://www.pdfhummus.com/")
    set_description("High performance library for creating, modiyfing and parsing PDF files in C++ ")
    set_license("Apache-2.0")

    set_sourcedir(path.join(os.scriptdir(), "3rdparty/pdfhummus"))

    add_deps("zlib", "liii-libaesgm")
    add_deps("freetype", {configs={png=true}})

    add_configs("libtiff", {description = "Supporting tiff image", default = false, type = "boolean"})
    add_configs("libjpeg", {description = "Support DCT encoding", default = false, type = "boolean"})
    add_configs("libpng", {description = "Support png image", default = false, type = "boolean"})

    if is_plat("linux") then
        add_syslinks("m")
    end

    on_load(function (package)
        for _, dep in ipairs({"libtiff", "libpng", "libjpeg"}) do
            if package:config(dep) then
                package:add("deps", dep)
            end
        end
    end)
    on_install("linux", "windows", "mingw", "macosx", function (package)
        local configs = {}
        if package:config("shared") then
            configs.kind = "shared"
        end
        for _, dep in ipairs({"libtiff", "libpng", "libjpeg"}) do
            if package:config(dep) then
                configs[dep] = true
            end
        end
        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include "PDFWriter/PDFWriter.h"
            #include <iostream>
            using namespace std;
            using namespace PDFHummus;
            void test() {
                PDFWriter pdfWriter;
                pdfWriter.Reset();
            }
        ]]}, {configs = {languages = "c++11"}}))
    end)
package_end()

includes("@builtin/check")
configvar_check_cxxtypes("HAVE_INTPTR_T", "intptr_t", {includes = {"memory"}})
configvar_check_cxxincludes("HAVE_INTTYPES_H", "inttypes.h")
configvar_check_cxxincludes("HAVE_STDINT_H", "stdint.h")

function using_apt ()
    return linuxos.name() == "debian"
           or linuxos.name() == "ubuntu"
           or linuxos.name() == "uos"
end

function using_legacy_apt ()
    return (linuxos.name() == "uos") or (linuxos.name () == "ubuntu" and linuxos.version():major() == 20)
end

local LIBICONV_VERSION = "1.17"

add_requires("lolly", {system=false})
-- QWK is built locally from 3rdparty/qwindowkitty, no external package needed
if is_plat ("windows") then
    add_requires("libiconv "..LIBICONV_VERSION, {system=false})
end

add_requires("libjpeg")
if is_plat("linux") then
    add_requires("apt::libpng-dev", {alias="libpng"})
    add_requires("apt::libcurl4-openssl-dev", {alias="libcurl"})
end

add_requires("liii-pdfhummus", {system=false,configs={libpng=true,libjpeg=true}})
add_requires("freetype "..FREETYPE_VERSION, {system=false, configs={png=true}})
add_requireconfs("liii-pdfhummus.freetype", {version = FREETYPE_VERSION, system = false, configs={png=true}, override=true})

add_requires("argh v1.3.2")

--- package: qt6widgets
QT6_VERSION="6.8.3"
add_requires("qt6widgets "..QT6_VERSION)

if has_config("mupdf") then
    if (linuxos.name() == "debian" and linuxos.version():major() >= CURRENT_DEBIAN_VERSION) or
       (linuxos.name() == "ubuntu" and linuxos.version():major() >= CURRENT_UBUNTU_VERSION)
    then
        add_requires("apt::libmupdf-dev", {alias="mupdf"})
    else
        add_requires("mupdf", {system=false})
    end
end

set_configvar("USE_FREETYPE", 1)

function build_glue_on_config()
    on_config(function (target)
        import("core.project.depend")
        -- use relative path here to avoid import failure on windows
        local scheme_path = path.join("src", "Scheme")
        local build_glue_path = path.join("src", "Scheme", "Glue")
        local build_glue = import("build_glue", {rootdir = build_glue_path})
        for _, filepath in ipairs(os.filedirs(path.join(scheme_path, "**/glue_*.lua"))) do
            depend.on_changed(function ()
                local glue_name = path.basename(filepath)
                local glue_dir = path.directory(filepath)
                local glue_table = import(glue_name, {rootdir = glue_dir})()
                io.writefile(
                    path.join("$(buildir)/glue", glue_name .. ".cpp"),
                    build_glue(glue_table, glue_name))
                cprint("generating scheme glue %s ... %s", glue_name, "${color.success}${text.success}")
            end, {
                values = {true},
                files = {filepath, path.join(build_glue_path, "build_glue.lua")},
                always_changed = false
            })
        end
        os.mkdir(path.join("$(buildir)/glue"))
    end)
end

target("libmoebius") do
    set_kind ("static")
    set_languages("c++17")
    set_encodings("utf-8")
    set_basename("moebius")

    add_includedirs(moe_includedirs)
    add_files(moe_files)

    add_packages("lolly")
    add_packages("s7")

    on_install(function (target)
    end)
end

target("libqhotkey") do
  set_kind("static")
  add_rules("qt.static")
  add_rules("qt.moc")
  add_packages("qt6widgets")

  -- 添加平台特定依赖
  if is_plat("linux") then
    add_packages("x11")
  end

  -- 添加头文件搜索路径，供依赖此target的模块使用
  add_includedirs("3rdparty", {public = true})

  -- 通用源文件 - 需要MOC处理的头文件必须作为源文件添加
  add_files("3rdparty/qhotkey/qhotkey.cpp")
  add_files("3rdparty/qhotkey/qhotkey.h")
  add_files("3rdparty/qhotkey/qhotkey_p.h")
  add_headerfiles("3rdparty/qhotkey/(qhotkey.h)")

  -- 平台特定源文件
  if is_plat("macosx") then
    add_files("3rdparty/qhotkey/qhotkey_mac.cpp")
  elseif is_plat("linux") then
    add_files("3rdparty/qhotkey/qhotkey_x11.cpp")
  elseif is_plat("windows") then
    add_files("3rdparty/qhotkey/qhotkey_win.cpp")
  end

  on_install(function (target)
  end)
end

-- Add options for different features
option("style_agent")
    set_default(false)
    set_description("Enable Style Agent")
option_end()

option("windows_system_borders")
    set_default(false)
    set_description("Enable Windows System Borders")
option_end()

target("QWKCore")
    -- Add library export define for shared library
    set_kind("$(kind)")
    if is_kind("shared") then
        add_defines("QWK_CORE_LIBRARY")
    elseif is_kind("static") then
        add_defines("QWK_CORE_STATIC")
    end

    if is_plat("windows") then
        add_cxxflags("/Zc:__cplusplus", "/permissive-")
        add_syslinks("mpr", "userenv", "kernel32", "user32", "gdi32", "winspool", "shell32", "ole32", "oleaut32", "uuid", "comdlg32", "advapi32")
    else
        add_cxxflags("-fPIC", "-fvisibility=hidden", "-fvisibility-inlines-hidden")
    end
    add_packages("qt6base", "qt6core", "qt6gui", "qt6widgets")
    if is_plat("macosx") then
        add_mxflags("-fno-objc-arc")
        add_frameworks("Foundation", "Cocoa", "AppKit")
        add_frameworks("QtCore", "QtGui", "QtWidgets")
    end

    -- Generate config header before build
    before_build(function (target)
        -- Create build directories
        os.mkdir("$(buildir)/include/QWKCore")
        os.mkdir("$(buildir)/include/QWKCore/private")
        
        -- Generate qwkconfig.h
        local config_content = [[
#ifndef QWKCONFIG_H
#define QWKCONFIG_H

#define QWINDOWKIT_ENABLE_QT_WINDOW_CONTEXT ]] .. 
        "-1" .. [[

#define QWINDOWKIT_ENABLE_STYLE_AGENT ]] ..
        (has_config("style_agent") and "1" or "-1") .. [[

#define QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS ]] ..
        (has_config("windows_system_borders") and "1" or "-1") .. [[


#endif // QWKCONFIG_H
]]
        local config_path = "$(buildir)/include/QWKCore/qwkconfig.h"
        local existing_content = nil
        if os.isfile(config_path) then
            existing_content = io.readfile(config_path)
        end
        if existing_content ~= config_content then
            io.writefile(config_path, config_content)
        end
        
        -- Copy header files without bumping timestamps when unchanged
        os.vcp("3rdparty/qwindowkitty/src/core/*.h", "$(buildir)/include/QWKCore/")
        os.vcp("3rdparty/qwindowkitty/src/core/*_p.h", "$(buildir)/include/QWKCore/private/")
        os.vcp("3rdparty/qwindowkitty/src/core/contexts/*_p.h", "$(buildir)/include/QWKCore/private/")
        os.vcp("3rdparty/qwindowkitty/src/core/contexts/*.h", "$(buildir)/include/QWKCore/private/")
        os.vcp("3rdparty/qwindowkitty/src/core/kernel/*_p.h", "$(buildir)/include/QWKCore/private/")
        os.vcp("3rdparty/qwindowkitty/src/core/shared/*_p.h", "$(buildir)/include/QWKCore/private/")
        
        if has_config("style_agent") then
            os.vcp("3rdparty/qwindowkitty/src/core/style/*_p.h", "$(buildir)/include/QWKCore/private/")
            os.vcp("3rdparty/qwindowkitty/src/core/style/styleagent.h", "$(buildir)/include/QWKCore/styleagent.h")
        end

        local private_paths = {}
        local qt_package = get_config("qt")
        local qt_version = get_config("qt_sdkver")

        local modules = {"QtCore", "QtGui"}
        for _, module in ipairs(modules) do
            local headers_path = ""
            if is_plat("macosx") then
                headers_path= path.join(qt_package, "lib", module .. ".framework", "Headers")
                table.insert(private_paths, path.join(headers_path, qt_version, module, "private"))
                table.insert(private_paths, path.join(headers_path, qt_version, module))
                table.insert(private_paths, path.join(headers_path, qt_version))
            else
                headers_path= path.join(qt_package, "include")
                table.insert(private_paths, path.join(headers_path, module, qt_version, module, "private"))
                table.insert(private_paths, path.join(headers_path, module, qt_version, module))
                table.insert(private_paths, path.join(headers_path, module, qt_version))
            end
        end
        if is_plat("windows") then
            table.insert(private_paths, path.join(qt_package, "mkspecs", "win32-msvc"))
        end
        target:add("includedirs", private_paths, {public = true})
    end)

    -- Include directories
    add_includedirs("$(buildir)/include", {public = true})
    add_includedirs("3rdparty/qwindowkitty/src/core", "3rdparty/qwindowkitty/src/core/kernel", "3rdparty/qwindowkitty/src/core/shared", "3rdparty/qwindowkitty/src/core/contexts", "3rdparty/qwindowkitty/src")

    -- Defines
    add_defines("QWINDOWKIT_ENABLE_QT_WINDOW_CONTEXT=-1")
    
    if has_config("style_agent") then
        add_defines("QWINDOWKIT_ENABLE_STYLE_AGENT=1")
    else
        add_defines("QWINDOWKIT_ENABLE_STYLE_AGENT=-1")
    end
    
    if has_config("windows_system_borders") then
        add_defines("QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS=1")
    else
        add_defines("QWINDOWKIT_ENABLE_WINDOWS_SYSTEM_BORDERS=-1")
    end

    -- Enable MOC generation for Qt
    add_rules("qt.moc")


    -- Core source files
    add_files("3rdparty/qwindowkitty/src/core/qwkglobal_p.h")
    add_files("3rdparty/qwindowkitty/src/core/qwkglobal.cpp")
    add_files("3rdparty/qwindowkitty/src/core/windowagentbase.cpp")
    add_files("3rdparty/qwindowkitty/src/core/windowitemdelegate.cpp")
    add_files("3rdparty/qwindowkitty/src/core/kernel/nativeeventfilter.cpp")
    add_files("3rdparty/qwindowkitty/src/core/kernel/sharedeventfilter.cpp")
    add_files("3rdparty/qwindowkitty/src/core/kernel/winidchangeeventfilter.cpp")
    add_files("3rdparty/qwindowkitty/src/core/contexts/abstractwindowcontext.cpp")
    if has_config("style_agent") then
        add_files("3rdparty/qwindowkitty/src/core/style/styleagent.cpp")
        add_files("3rdparty/qwindowkitty/src/core/style/styleagent_mac.mm")
    end
    if is_plat("windows") then
        add_files("3rdparty/qwindowkitty/src/core/qwindowkit_windows.h")
        add_files("3rdparty/qwindowkitty/src/core/qwindowkit_windows.cpp")
    end

    -- Add header files that need MOC processing (use add_files for Q_OBJECT headers)
    add_files("3rdparty/qwindowkitty/src/core/windowagentbase.h")
    add_files("3rdparty/qwindowkitty/src/core/windowagentbase_p.h")
    add_files("3rdparty/qwindowkitty/src/core/contexts/abstractwindowcontext_p.h")

    if is_plat("macosx") then
        add_files("3rdparty/qwindowkitty/src/core/contexts/cocoawindowcontext_p.h")
        add_files("3rdparty/qwindowkitty/src/core/contexts/cocoawindowcontext.mm")
    end
    if is_plat("linux") then
        add_files("3rdparty/qwindowkitty/src/core/contexts/qtwindowcontext_p.h")
        add_files("3rdparty/qwindowkitty/src/core/contexts/qtwindowcontext.cpp")
    end
    if is_plat("windows") then
        add_files("3rdparty/qwindowkitty/src/core/contexts/win32windowcontext_p.h")
        add_files("3rdparty/qwindowkitty/src/core/contexts/win32windowcontext.cpp")
    end
    if is_plat("windows") then
        add_files("3rdparty/qwindowkitty/src/core/shared/qwkwindowsextra_p.h")
        add_files("3rdparty/qwindowkitty/src/core/shared/windows10borderhandler_p.h")
        add_files("3rdparty/qwindowkitty/src/core/shared/systemwindow_p.h")
    end
   

    if has_config("style_agent") then
        add_files("3rdparty/qwindowkitty/src/core/style/styleagent.h")
    end

    -- Set install headers
    add_headerfiles("$(buildir)/include/QWKCore/**.h", {prefixdir = "QWKCore"})
    add_headerfiles("$(buildir)/include/QWKCore/private/**.h", {prefixdir = "QWKCore/private"})

    on_install(function (target)
    end)
target_end()

target("QWKWidgets")
    set_kind("$(kind)")
    -- Add library export define for shared library
    if is_kind("shared") then
        add_defines("QWK_WIDGETS_LIBRARY")
    elseif is_kind("static") then
        add_defines("QWK_CORE_STATIC")
        add_defines("QWK_WIDGETS_STATIC")
    end

    if is_plat("windows") then
        add_cxxflags("/Zc:__cplusplus", "/permissive-")
    else
        add_cxxflags("-fPIC", "-fvisibility=hidden", "-fvisibility-inlines-hidden")
    end
    add_deps("QWKCore")
    add_packages("qt6core", "qt6gui", "qt6widgets")
    if is_plat("macosx") then
        add_mxflags("-fno-objc-arc")
        add_frameworks("Foundation", "Cocoa", "AppKit")
        add_frameworks("QtCore", "QtGui", "QtWidgets")
    end

    -- Enable MOC generation for Qt
    add_rules("qt.moc")

    -- Enable RCC generation for Qt resources used by this target
    add_rules("qt.qrc")

    -- Generate config header and copy headers before build
    before_build(function (target)
        os.mkdir("$(buildir)/include/QWKWidgets")
        os.mkdir("$(buildir)/include/QWKWidgets/ui/widgetframe")
        os.vcp("3rdparty/qwindowkitty/src/widgets/*.h", "$(buildir)/include/QWKWidgets/")
        os.vcp("3rdparty/qwindowkitty/src/ui/widgetframe/*.h", "$(buildir)/include/QWKWidgets/ui/widgetframe/")

        local private_paths = {}
        local qt_package = get_config("qt")
        local qt_version = get_config("qt_sdkver")

        local modules = {"QtCore", "QtGui"}
        for _, module in ipairs(modules) do
            local headers_path = ""
            if is_plat("macosx") then
                headers_path= path.join(qt_package, "lib", module .. ".framework", "Headers")
                table.insert(private_paths, path.join(headers_path, qt_version, module, "private"))
                table.insert(private_paths, path.join(headers_path, qt_version, module))
                table.insert(private_paths, path.join(headers_path, qt_version))
            else
                headers_path= path.join(qt_package, "include")
                table.insert(private_paths, path.join(headers_path, module, qt_version, module, "private"))
                table.insert(private_paths, path.join(headers_path, module, qt_version, module))
                table.insert(private_paths, path.join(headers_path, module, qt_version))
            end
        end
        if is_plat("windows") then
            table.insert(private_paths, path.join(qt_package, "mkspecs", "win32-msvc"))
        end
        target:add("includedirs", private_paths, {public = true})
    end)

    -- Include directories
    add_includedirs("$(buildir)/include", {public = true})
    add_includedirs("3rdparty/qwindowkitty/src/widgets", "3rdparty/qwindowkitty/src")

    -- Source files
    add_files("3rdparty/qwindowkitty/src/widgets/widgetitemdelegate.cpp")
    add_files("3rdparty/qwindowkitty/src/widgets/widgetwindowagent_p.h")
    add_files("3rdparty/qwindowkitty/src/widgets/widgetwindowagent.h")
    add_files("3rdparty/qwindowkitty/src/widgets/widgetwindowagent.cpp")
    add_files("3rdparty/qwindowkitty/src/ui/widgetframe/windowbar.cpp")
    add_files("3rdparty/qwindowkitty/src/ui/widgetframe/windowbar.h")
    add_files("3rdparty/qwindowkitty/src/ui/widgetframe/windowbar_p.h")
    add_files("3rdparty/qwindowkitty/src/ui/widgetframe/windowbutton.cpp")
    add_files("3rdparty/qwindowkitty/src/ui/widgetframe/windowbutton.h")
    if is_plat("macosx") then
        add_files("3rdparty/qwindowkitty/src/widgets/widgetwindowagent_mac.cpp")
    end
    if is_plat("windows") then
        add_files("3rdparty/qwindowkitty/src/widgets/widgetwindowagent_win.cpp")
    end
    add_files("3rdparty/qwindowkitty/src/styles/styles.qrc")

    -- Set install headers
    add_headerfiles("$(buildir)/include/QWKWidgets/**.h", {prefixdir = "QWKWidgets"})

    on_install(function (target)
    end)
target_end()

target("libmogan") do
    set_basename("mogan")
    local TEXMACS_VERSION = "2.1.4"
    local DEVEL_VERSION = TEXMACS_VERSION
    local DEVEL_RELEASE = 1
    local STABLE_VERSION = TEXMACS_VERSION
    local STABLE_RELEASE = 1
    set_version(TEXMACS_VERSION)

    if is_plat("windows") then
        set_runtimes("MT")
        add_defines("_USE_MATH_DEFINES")
    end
    set_languages("c++17")
    set_policy("check.auto_ignore_flags", false)
    set_encodings("utf-8")

    add_deps("libmoebius")
    add_deps("libqhotkey")
    add_deps("QWKCore", "QWKWidgets")

    -- 添加 3rdparty 头文件搜索路径（用于 qhotkey）
    add_includedirs("3rdparty", {public = true})
    
    set_policy("check.auto_ignore_flags", false)
    add_rules("qt.static")
    on_install(function (target)
        print("No need to install libmogan")
    end)
    add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtNetwork", "QtNetworkAuth")

    build_glue_on_config()
    set_configvar("QTTEXMACS", 1)
    add_defines("QTTEXMACS")
    set_configvar("QTPIPES", 1)
    add_defines("QTPIPES")
    set_configvar("USE_QT_PRINTER", 1)
    add_defines("USE_QT_PRINTER")

    add_packages("lolly")
    add_packages("liii-pdfhummus")
    add_packages("freetype")
    add_packages("s7")
    add_packages("argh")
    if not is_plat("macosx") then
        add_packages("libiconv")
    end

    if is_plat("windows") then
        add_syslinks("secur32", "shell32", "winhttp", "rpcrt4", {public = true})
    elseif is_plat("macosx") then
        add_syslinks("iconv")
    end

    ---------------------------------------------------------------------------
    -- generate config files. see also:
    --    * https://github.com/xmake-io/xmake/issues/320
    --    * https://github.com/xmake-io/xmake/issues/342
    ---------------------------------------------------------------------------
    set_configdir("src/System")
    -- check for dl library
    -- configvar_check_cxxfuncs("TM_DYNAMIC_LINKING","dlopen")
    configvar_check_cxxincludes("HAVE_INTTYPES_H", "inttypes.h")

    set_configvar("STDC_HEADERS", true)

    set_configvar("GS_EXE", "/usr/bin/gs")

    set_configvar("PDFHUMMUS_NO_TIFF", true)
    add_configfiles(
        "src/System/config.h.xmake", {
            filename = "config.h",
            variables = {
                GS_FONTS = "../share/ghostscript/fonts:/usr/share/fonts:",
                GS_LIB = "../share/ghostscript/9.06/lib:",
                OS_MACOS = is_plat("macosx"),
                MACOSX_EXTENSIONS = is_plat("macosx"),
                SIZEOF_VOID_P = 8,
                USE_ICONV = true,
                USE_PLUGIN_GS = true,
                USE_PLUGIN_BIBTEX = true,
                USE_PLUGIN_LATEX_PREVIEW = true,
                USE_PLUGIN_TEX = true,
                USE_PLUGIN_ISPELL = true,
                USE_PLUGIN_PDF = true,
                USE_PLUGIN_SPARKLE = false,
                USE_PLUGIN_HTML = true,
                USE_MUPDF_RENDERER = has_config("mupdf"),
                IS_COMMUNITY = has_config("is_community"),
                DEBUG_WITH_TIMESTAMP = has_config("debug_with_timestamp"),
                }})

    if is_plat("linux") then 
        set_configvar("CONFIG_OS", "GNU_LINUX")
    elseif is_subhost("cygwin") then
        set_configvar("CONFIG_OS", "CYGWIN")
    else 
        set_configvar("CONFIG_OS", "")
    end

    configvar_check_cxxsnippets(
        "CONFIG_LARGE_POINTER", [[
            #include <stdlib.h>
            static_assert(sizeof(void*) == 8, "");]])
    add_configfiles(
        "src/System/tm_configure.hpp.xmake", {
            filename = "tm_configure.hpp",
            pattern = "@(.-)@",
            variables = {
                TEXMACS_VERSION = TEXMACS_VERSION,
                XMACS_VERSION = XMACS_VERSION,
                CACHE_NAME = stem_lab_big_name,
                STEM_NAME = stem_binary_name,
                CONFIG_USER = os.getenv("USER") or "unknown",
                CONFIG_DATE = os.time(),
                CONFIG_STD_SETENV = "#define STD_SETENV",
                tm_devel = "Texmacs-" .. DEVEL_VERSION,
                tm_devel_release = "Texmacs-" .. DEVEL_VERSION .. "-" .. DEVEL_RELEASE,
                tm_stable = "Texmacs-" .. STABLE_VERSION,
                tm_stable_release = "Texmacs-" .. STABLE_VERSION .. "-" .. STABLE_RELEASE,
                tm_prefix_dir = stem_lab_name,
                PDFHUMMUS_VERSION = PDFHUMMUS_VERSION,
                LOLLY_VERSION = LOLLY_VERSION,
                }})

    ---------------------------------------------------------------------------
    -- add source and header files
    ---------------------------------------------------------------------------
    add_includedirs(moe_includedirs)
    add_includedirs({
            "src/Data/Convert",
            "src/Data/Document",
            "src/Data/History",
            "src/Data/Observers",
            "src/Data/Parser",
            "src/Data/String",
            "src/Data/Tree",
            "src/Edit",
            "src/Edit/Editor",
            "src/Edit/Interface",
            "src/Edit/Modify",
            "src/Edit/Process",
            "src/Edit/Replace",
            "src/Graphics/Bitmap_fonts",
            "src/Graphics/Colors",
            "src/Graphics/Fonts",
            "src/Graphics/Gui",
            "src/Graphics/Handwriting",
            "src/Graphics/Mathematics",
            "src/Graphics/Pictures",
            "src/Graphics/Renderer",
            "src/Graphics/Spacial",
            "src/Graphics/Types",
            "src/Kernel/Abstractions",
            "src/Kernel/Types",
            "src/Plugins",
            "src/Plugins/Pdf",
            "src/Plugins/Qt",
            "src/Plugins/Html",
            "src/Scheme",
            "src/Scheme/L2",
            "src/Scheme/L3",
            "src/Scheme/L4",
            "src/Scheme/L5",
            "src/Scheme/Plugins",
            "src/Scheme/S7",
            "src/Scheme/Scheme",
            "src/System",
            "src/System/Boot",
            "src/System/Classes",
            "src/System/Config",
            "src/System/Files",
            "src/System/Language",
            "src/System/Link",
            "src/System/Misc",
            "src/Texmacs",
            "src/Texmacs/Data",
            "src/Typeset",
            "src/Typeset/Bridge",
            "src/Typeset/Concat",
            "src/Typeset/Page",
            "TeXmacs/include",
            "$(buildir)/glue",
            "$(projectdir)/TeXmacs/plugins/goldfish/src/"
        }, {public = true})

    add_files({
            "src/Data/**.cpp",
            "src/Edit/**.cpp",
            "src/Graphics/**.cpp",
            "src/Kernel/**.cpp",
            "src/Scheme/Scheme/**.cpp",
            "src/Scheme/S7/**.cpp",
            "src/Scheme/L2/**.cpp",
            "src/Scheme/L3/**.cpp",
            "src/Scheme/L4/**.cpp",
            "src/Scheme/L5/**.cpp",
            "src/Scheme/Plugins/**.cpp",
            "src/System/**.cpp",
            "src/Texmacs/Data/**.cpp",
            "src/Texmacs/Server/**.cpp",
            "src/Texmacs/Window/**.cpp",
            "src/Typeset/**.cpp",
            "src/Plugins/Bibtex/**.cpp",
            "src/Plugins/Freetype/**.cpp",
            "src/Plugins/Pdf/**.cpp",
            "src/Plugins/Ghostscript/**.cpp",
            "src/Plugins/Ispell/**.cpp",
            "src/Plugins/Metafont/**.cpp",
            "src/Plugins/LaTeX_Preview/**.cpp",
            "src/Plugins/Openssl/**.cpp",
            "src/Plugins/Tex/**.cpp",
            "src/Plugins/Xml/**.cpp",
            "src/Plugins/Html/**.cpp",
            "src/Plugins/Updater/**.cpp",
            "$(projectdir)/TeXmacs/plugins/goldfish/src/**.cpp"})

    add_files("src/Plugins/Qt/**.cpp", "src/Plugins/Qt/**.hpp")

    -- Add Qt resource file
    add_rules("qt.qrc")
    add_files("TeXmacs/misc/images/images.qrc")

    if is_plat("macosx") then
        plugin_macos_srcs = {
            "$(projectdir)/src/Plugins/MacOS/HIDRemote.m",
            "$(projectdir)/src/Plugins/MacOS/mac_spellservice.mm",
            "$(projectdir)/src/Plugins/MacOS/mac_utilities.mm",
            "$(projectdir)/src/Plugins/MacOS/mac_app.mm"
        }
        add_includedirs("src/Plugins/MacOS", {public = true})
        add_files(plugin_macos_srcs)
    end
    if is_plat("macosx", "linux", "windows") then
        add_includedirs("src/Plugins/QWindowKit", {public = true})
        add_files("src/Plugins/QWindowKit/**.cpp")
        add_files("src/Plugins/QWindowKit/**.hpp")
    end

    if has_config("mupdf") then
        add_includedirs({
            "src/Plugins/MuPDF"
        }, {public = true})
        add_files({
            "src/Plugins/MuPDF/**.cpp"
        })
        add_packages("mupdf")
    end

    add_mxflags("-fno-objc-arc")
    before_build(function (target)
        target:add("forceincludes", path.absolute("src/System/config.h"))
        target:add("forceincludes", path.absolute("src/System/tm_configure.hpp"))
    end)
end 

local stem_files = {
    "$(projectdir)/TeXmacs(/doc/**)",
    "$(projectdir)/TeXmacs(/langs/**)",
    "$(projectdir)/TeXmacs(/misc/**)",
    "$(projectdir)/TeXmacs(/packages/**)",
    "$(projectdir)/TeXmacs(/progs/**)",
    "$(projectdir)/TeXmacs(/styles/**)",
    "$(projectdir)/TeXmacs(/texts/**)",
    "$(projectdir)/TeXmacs/COPYING", -- copying files are different
    "$(projectdir)/TeXmacs/INSTALL",
    "$(projectdir)/LICENSE", -- license files are same
    "$(projectdir)/TeXmacs/README",
    "$(projectdir)/TeXmacs/TEX_FONTS",
    "$(projectdir)/TeXmacs(/plugins/**)" -- plugin files
}

if is_plat("windows") then
    target("liii_windows_icon") do
        set_version(XMACS_VERSION)
        set_kind("object")
        add_configfiles("$(projectdir)/packages/windows/resource.rc.in", {
            filename = "resource.rc"
        })
        add_configfiles("$(projectdir)/packages/windows/Xmacs.ico", {
            onlycopy = true
        })
        add_configfiles("$(projectdir)/packages/windows/TeXmacs.ico", {
            onlycopy = true
        })
        add_files("$(buildir)/resource.rc")
    end
end

target("stem") do 
    add_deps("goldfish")
    if is_plat("windows") and is_mode("release") then
        add_deps("liii_windows_icon")
    end
    if is_plat("linux") then
        set_filename(stem_binary_linux)
    elseif is_plat("macosx") then
        set_filename(stem_binary_macos)
    else
        set_filename(stem_binary_windows)
    end

    local install_dir = "$(buildir)"
    if is_plat("windows") then
        install_dir = path.join("$(buildir)", "packages/stem/data/")
    elseif is_plat("macosx") then
        install_dir = path.join("$(buildir)", "macosx/$(arch)/$(mode)/" .. stem_binary_name .. ".app/Contents/Resources/")
    else
        if os.getenv("INSTALL_DIR") == nil then
            install_dir = path.join("$(buildir)", "packages/stem/")
        else
            install_dir = os.getenv("INSTALL_DIR")
        end
    end
    set_installdir(install_dir)

    if is_plat("windows") then
        add_installfiles(stem_files)
        add_installfiles(path.join(os.projectdir(), "packages/windows/TeXmacs.ico"), {prefixdir = ""})
    else
        add_installfiles(stem_files, {prefixdir=stem_prefix_dir})
    end

    if is_plat("windows") then
        add_installfiles("$(projectdir)/TeXmacs(/fonts/**)")
    else
        add_installfiles("$(projectdir)/TeXmacs(/fonts/**)", {prefixdir=stem_prefix_dir})
    end

    if is_plat("linux") then
        add_installfiles("$(projectdir)/TeXmacs/misc/mime/" .. stem_binary_name .. ".desktop", {prefixdir="share/applications"})
        add_installfiles("$(projectdir)/TeXmacs/misc/images/" .. stem_binary_name .. ".png", {prefixdir="share/icons/hicolor/512x512/apps"})

        add_installfiles("$(projectdir)/TeXmacs/misc/images/texmacs-document.svg", {prefixdir="share/icons/hicolor/scalable/mimetypes"})

        local mime_icon_sizes = {"16", "20", "22", "24", "32", "36", "40", "48", "64", "72", "96", "128", "192", "256", "512"}
        for _, size in ipairs(mime_icon_sizes) do
            add_installfiles("$(projectdir)/TeXmacs/misc/images/texmacs-document-" .. size .. ".png",
                            {prefixdir="share/icons/hicolor/" .. size .. "x" .. size .. "/mimetypes"})
        end

        add_installfiles("$(projectdir)/TeXmacs/misc/mime/texmacs.xml", {prefixdir="share/mime/packages"})
    end


    -- package metadata
    if is_plat("macosx") then
        add_installfiles({
            "$(projectdir)/packages/macos/stem.icns",
            "$(projectdir)/packages/macos/TeXmacs-document.icns",
            "$(projectdir)/src/Plugins/Cocoa/(en.lproj/**)",
            "$(projectdir)/src/Plugins/Cocoa/(zh-Hans.lproj/**)"
        })
    end

    if is_plat("windows") then
        set_optimize("smallest")
        set_runtimes("MT")
        add_ldflags("/STACK:16777216")
    end

    if is_mode("debug", "releasedbg") and is_plat("windows") then
        add_rules("qt.console")
    else
        add_rules("qt.widgetapp")
    end

    add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtNetwork", "QtNetworkAuth")
    add_packages("s7")
    add_packages("lolly")
    add_deps("libmogan")
    if not is_plat("windows") then
        add_syslinks("pthread", "dl", "m")
    end
    if is_plat("linux") then
        add_syslinks("X11")
    end

    add_includedirs(moe_includedirs)
    add_files("src/Mogan/Research/research.cpp")

    -- install tm files for testing purpose
    if is_mode("releasedbg") then
        if is_plat("mingw", "windows") then
            add_installfiles({
                "$(projectdir)/TeXmacs(/tests/tm/*.tm)",
                "$(projectdir)/TeXmacs(/tests/tex/*.tex)",
                "$(projectdir)/TeXmacs(/tests/bib/*.bib)",
            })
        else
            add_installfiles({
                "$(projectdir)/TeXmacs(/tests/*.tm)",
                "$(projectdir)/TeXmacs(/tests/*.bib)",
            }, {prefixdir=stem_prefix_dir})
        end
    end

    -- deploy necessary dll
    if is_plat("windows") then
        set_values("qt.deploy.flags", {"-printsupport", "--no-opengl-sw", "--no-translations", "--release"})
    end

    on_run(function (target)
        local name = target:name()
        -- path to the binary: for Windows we use the install dir's bin/, otherwise the build artifact
        local binary
        if is_plat("windows") then
            binary = path.join(target:installdir(), "bin", target:filename())
        else
            binary = target:targetfile()
        end

        -- Default program parameters (kept to preserve old behaviour)
        local params = {"-d", "-debug-bench"}

        -- Allow overriding debug-run behavior by setting DEBUG or XMAKE_DEBUGGER env var.
        -- If set, we will launch an interactive debugger (gdb on linux, lldb on macos) instead
        local run_debugger = os.getenv("DEBUG") or os.getenv("XMAKE_DEBUGGER")
        if run_debugger and run_debugger ~= "" then
            -- If not compiled in debug mode, warn user that symbols may be missing
            if not is_mode("debug") then
                cprint("${color.warning}Warning: running under debugger but build mode is not debug. Symbols may be missing.${text.reset}")
            end

            if is_plat("linux") then
                if os.exists("/usr/bin/gdb") or os.exists("/bin/gdb") or os.exec("which gdb >/dev/null 2>&1 || true") == 0 then
                    print("Launching gdb for: " .. binary)
                    os.execv("gdb", {"--args", binary, table.unpack(params)})
                else
                    print("gdb not found; running binary directly.")
                    os.execv(binary, params, {envs={TEXMACS_PATH= path.join(os.projectdir(), "TeXmacs")}})
                end
            elseif is_plat("macosx") then
                if os.exists("/usr/bin/lldb") or os.exec("which lldb >/dev/null 2>&1 || true") == 0 then
                    print("Launching lldb for: " .. binary)
                    os.execv("lldb", {"--", binary, table.unpack(params)}, {envs={TEXMACS_PATH= path.join(os.projectdir(), "TeXmacs")}})
                else
                    print("lldb not found; running binary directly.")
                    os.execv(binary, params, {envs={TEXMACS_PATH= path.join(os.projectdir(), "TeXmacs")}})
                end
            elseif is_plat("windows") then
                -- On Windows, prefer to launch cdb / windbg if present; fallback to running directly
                local windbg = os.exec("which windbg >/dev/null 2>&1 || true")
                if windbg == 0 then
                    print("Launching windbg for: " .. binary)
                    os.execv("windbg", {binary})
                else
                    local cdb = os.exec("which cdb >/dev/null 2>&1 || true")
                    if cdb == 0 then
                        print("Launching cdb for: " .. binary)
                        os.execv("cdb", {binary})
                    else
                        print("No suitable debugger found on PATH; running binary directly.")
                        os.execv(binary, params)
                    end
                end
            else
                print("Unsupported platform: " .. tostring(os.host()))
            end
        else
            -- Normal run: pass TEXMACS_PATH env var on POSIX platforms
            if is_plat("linux", "macosx") then
                print("Launching " .. binary)
                os.execv(binary, params, {envs={TEXMACS_PATH= path.join(os.projectdir(), "TeXmacs")}})
            elseif is_plat("windows") then
                os.execv(binary, params)
            else
                print("Unsupported platform: " .. tostring(os.host()))
            end
        end
    end)

    before_build(function (target)
        target:add("forceincludes", path.absolute("src/System/config.h"))
        target:add("forceincludes", path.absolute("src/System/tm_configure.hpp"))
    end)

    -- After install callback for Linux to rename MIME icon files
    after_install(function (target, opt)
        if is_plat("linux") then
            local install_dir = target:installdir()
            local mime_icon_sizes = {"16", "20", "22", "24", "32", "36", "40", "48", "64", "72", "96", "128", "192", "256", "512"}

            -- Rename texmacs-document-{size}.png to texmacs-document.png in each size directory
            for _, size in ipairs(mime_icon_sizes) do
                local src_file = path.join(install_dir, "share/icons/hicolor", size .. "x" .. size, "mimetypes", "texmacs-document-" .. size .. ".png")
                local dst_file = path.join(install_dir, "share/icons/hicolor", size .. "x" .. size, "mimetypes", "texmacs-document.png")

                if os.isfile(src_file) then
                    os.cp(src_file, dst_file)
                    os.rm(src_file)
                    print("Renamed MIME icon: " .. src_file .. " -> " .. dst_file)
                end
            end

            -- Also rename the SVG file if needed (should already be correct name)
            local svg_src = path.join(install_dir, "share/icons/hicolor/scalable/mimetypes", "texmacs-document.svg")
            if os.isfile(svg_src) then
                -- SVG file should already have correct name
                print("SVG MIME icon installed at: " .. svg_src)
            end
        end
    end)
end

function add_target_integration_test(filepath, INSTALL_DIR, RUN_ENVS)
    local testname = path.basename(filepath)
    target(testname) do
        set_enabled(not is_plat("wasm"))
        set_kind("phony")
        set_group("integration_tests")
        add_deps("stem")
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
                binary = path.join(INSTALL_DIR,"bin", stem_binary_windows)
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

-- Integration tests
RUN_ENVS = {TEXMACS_PATH=path.join(os.projectdir(), "TeXmacs")}
for _, filepath in ipairs(os.files("TeXmacs/tests/*.scm")) do
    add_target_integration_test(filepath, INSTALL_DIR, RUN_ENVS)
end

target("stem_packager") do
    set_enabled(is_plat("macosx") and is_mode("release"))
    set_kind("phony")

    add_deps("stem")

    -- 重新声明变量以解决作用域问题
    local stem_project_name_local = stem_project_name
    local stem_binary_name_local = stem_binary_name
	local stem_dmg_bg_name_local = stem_dmg_bg_image

    set_configvar("XMACS_VERSION", XMACS_VERSION)
    set_configvar("APPCAST", "")
    set_configvar("OSXVERMIN", "")
    set_configvar("STEM_NAME", stem_binary_name_local)
    add_configfiles("$(projectdir)/packages/macos/Info.plist.in", {
        filename = "Info.plist",
        pattern = "@(.-)@",
    })

    set_installdir(path.join("$(buildir)", "macosx/$(arch)/$(mode)/" .. stem_binary_name_local .. ".app/Contents/Resources/"))

    local dmg_name= stem_binary_name_local .. "-v" .. XMACS_VERSION .. ".dmg"
    if is_arch("arm64") then
        dmg_name= stem_binary_name_local .. "-v" .. XMACS_VERSION .. "-arm.dmg"
    elseif is_arch("x86_64") then
        dmg_name= stem_binary_name_local .. "-v" .. XMACS_VERSION .. "-x64.dmg"
    end
	
	-- print("DMG name will be: " .. dmg_name)
	-- print("Build dir is: " .. path.absolute("$(buildir)"))
	-- print("App dir is: " .. path.absolute(path.join("$(buildir)", "macosx/$(arch)/$(mode)/" .. stem_binary_name_local .. ".app")))

    after_install(function (target, opt)
        local app_dir = target:installdir() .. "/../../"
        local build_dir = path.absolute("$(buildir)")
        local project_dir = os.projectdir()
        
        print("Packaging app at: " .. app_dir)
		os.cp(path.join(build_dir, "Info.plist"), app_dir .. "/Contents")
        
        -- 复制图标文件
        local resources_dir = app_dir .. "/Contents/Resources"
        os.cp(path.join(project_dir, "packages", "macos", "stem.icns"), resources_dir)
        os.cp(path.join(project_dir, "packages", "macos", "TeXmacs-document.icns"), resources_dir)
        -- 复制DMG相关的图标到build目录，供create-dmg使用
        os.cp(path.join(project_dir, "packages", "macos", "driver.icns"), build_dir)
        print("Copied icon files to: " .. resources_dir)
        
        os.execv("codesign", {"--force", "--deep", "--sign", "-", app_dir})

        -- 构建DMG路径
        local dmg_path = path.join(build_dir, dmg_name)
        local app_path = path.absolute(app_dir)
        
        -- 清理可能存在的旧DMG文件和临时文件
        if os.isfile(dmg_path) then
            print("Removing existing DMG: " .. dmg_path)
            os.rm(dmg_path)
        end
        os.exec("rm -rf /tmp/create-dmg.* 2>/dev/null || true")
        os.exec("rm -rf /tmp/dmg.* 2>/dev/null || true")

        -- 尝试 create-dmg，使用更安全的参数格式
        -- Helper: run a command with retry on failure
        -- xmake sandbox disables pcall; use try/catch wrapper
        local function retry_execv(cmd, args, attempts, delay_seconds)
            attempts = attempts or 10
            delay_seconds = delay_seconds or 2
            local last_err = nil
            for i = 1, attempts do
                local ok = true
                try {
                    function ()
                        os.execv(cmd, args)
                    end,
                    catch {
                        function (errors)
                            ok = false
                            last_err = errors
                        end
                    }
                }
                if ok then
                    return true
                end
                print(string.format("Attempt %d/%d failed: %s", i, attempts, tostring(last_err)))
                if i < attempts then
                    print(string.format("Retrying in %d seconds...", delay_seconds))
                    os.sleep(delay_seconds * 1000)
                end
            end
            return false
        end

        try {
            function ()
				print("Creating DMG at: " .. dmg_path)
				print("Using app path: " .. app_path)
				
				-- 切换到 build 目录执行 create-dmg
				local old_dir = os.curdir()
				os.cd(build_dir)
				
                -- 检查背景图片
                local background_image = path.join(project_dir, "packages", "macos", stem_dmg_bg_name_local)
                local args_with_bg = {
                    "--volicon", "driver.icns",
                    "--background", background_image,
                    "--volname", stem_project_name_local,
                    "--window-pos", "200", "120",
                    "--window-size", "720", "480",
                    "--icon-size", "120",
                    "--icon", stem_binary_name_local .. ".app", "200", "190",
                    "--app-drop-link", "540", "190",
                    dmg_name,
                    app_path
                }
                local args_no_bg = {
                    "--volicon", "driver.icns",
                    "--volname", stem_project_name_local,
                    "--window-pos", "200", "120",
                    "--window-size", "720", "480",
                    "--icon-size", "120",
                    "--icon", stem_binary_name_local .. ".app", "200", "190",
                    "--app-drop-link", "540", "190",
                    dmg_name,
                    app_path
                }

                local ok
                if os.isfile(background_image) then
                    print("Background image found; using themed DMG background")
                    ok = retry_execv("create-dmg", args_with_bg, 10, 3)
                    if not ok then
                        print("create-dmg failed with background; retrying without background...")
                        ok = retry_execv("create-dmg", args_no_bg, 10, 3)
                    end
                else
                    print("Background image not found; creating DMG without background")
                    ok = retry_execv("create-dmg", args_no_bg, 10, 3)
                end

                if not ok then
                    raise("create-dmg failed after retries")
                end
                
                -- 恢复原目录
                os.cd(old_dir)
            end,
            catch {
                function (errors)
                    print("create-dmg failed: " .. tostring(errors))
                    -- Propagate failure to CI pipeline
                    raise(errors)
                end
            }
        }
    end)
end

if is_mode("release") then
includes("@builtin/xpack")
xpack("stem") do
    set_formats("nsis")
    set_author("Darcy Shen <da@liii.pro>")
    set_license("GPLv3")
    set_licensefile(path.join(os.projectdir(), "LICENSE"))
    set_title(stem_project_name)
    set_description("A one-stop solution that meets all your STEM writing needs")
    set_homepage(stem_homepage)

    _, pos = string.find(XMACS_VERSION, "-")
    local XMACS_VERSION_XYZ= XMACS_VERSION
    if not (pos == nil) then
        XMACS_VERSION_XYZ= string.sub(XMACS_VERSION, 1, pos-1)
    end
    set_version(XMACS_VERSION_XYZ..".0")

    if is_plat ("windows") then
        set_specfile(path.join(os.projectdir(), "packages/windows/research.nsis"))
        set_specvar("PACKAGE_INSTALL_DIR", stem_lab_big_name .. "\\" .. stem_binary_name .. "-" .. XMACS_VERSION)
        set_specvar("PACKAGE_NAME", stem_binary_name)
        set_specvar("PACKAGE_SHORTCUT_NAME", stem_project_name)
        set_iconfile(path.join(os.projectdir(), "packages/windows/Xmacs.ico"))
        set_bindir("bin")
        add_installfiles(path.join(os.projectdir(), "build/packages/stem/data/bin/(**)|" .. stem_binary_windows), {prefixdir = "bin"})
        add_installfiles(path.join(os.projectdir(), "packages/windows/TeXmacs.ico"), {prefixdir = "."})
    end


    set_basename(stem_binary_name)
    add_targets("stem")

    if is_plat("windows") then
        on_load(function (package)
            local format = package:format()
            local base_name = package:basename()
            if format == "nsis" then
                package:set("basename", base_name .. "-v" .. package:version() .. "-64bit-installer")
            else
                package:set("basename", base_name .. "-v" .. package:version() .. "-64bit-portable")
            end
        end)
    end
end
end

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