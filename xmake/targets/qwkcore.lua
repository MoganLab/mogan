-------------------------------------------------------------------------------
--
-- MODULE      : qwkcore.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

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

    on_load(function (target)
        -- Get build directory root (compatible with xmake v3.0.4+)
        -- $(builddir) is not resolved in on_load callbacks, so we derive it from targetdir
        local targetdir = target:targetdir()
        local buildir = path.directory(path.directory(path.directory(targetdir)))

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

        -- Create build directories
        os.mkdir(path.join(buildir, "include/QWKCore"))
        os.mkdir(path.join(buildir, "include/QWKCore/private"))

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
        local config_path = path.join(buildir, "include/QWKCore/qwkconfig.h")
        local existing_content = nil
        if os.isfile(config_path) then
            existing_content = io.readfile(config_path)
        end
        if existing_content ~= config_content then
            io.writefile(config_path, config_content)
        end

        -- Copy header files without bumping timestamps when unchanged
        local function safe_cp(src, dst)
            local src_content = io.readfile(src)
            local dst_content = nil
            if os.isfile(dst) then
                dst_content = io.readfile(dst)
            end
            if src_content ~= dst_content then
                os.cp(src, dst)
            end
        end
        local function safe_vcp(src_pattern, dst_dir)
            for _, filepath in ipairs(os.files(src_pattern)) do
                local dst = path.join(dst_dir, path.filename(filepath))
                safe_cp(filepath, dst)
            end
        end
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/*.h", path.join(buildir, "include/QWKCore/"))
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/*_p.h", path.join(buildir, "include/QWKCore/private/"))
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/*_p.h", path.join(buildir, "include/QWKCore/private/"))
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/*.h", path.join(buildir, "include/QWKCore/private/"))
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/kernel/*_p.h", path.join(buildir, "include/QWKCore/private/"))
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/shared/*_p.h", path.join(buildir, "include/QWKCore/private/"))

        if has_config("style_agent") then
            safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/style/*_p.h", path.join(buildir, "include/QWKCore/private/"))
            safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/core/style/styleagent.h", path.join(buildir, "include/QWKCore/styleagent.h"))
        end
    end)

    -- Include directories
    add_includedirs("$(builddir)/include", {public = true})
    add_includedirs("$(projectdir)/3rdparty/qwindowkitty/src/core", "$(projectdir)/3rdparty/qwindowkitty/src/core/kernel", "$(projectdir)/3rdparty/qwindowkitty/src/core/shared", "$(projectdir)/3rdparty/qwindowkitty/src/core/contexts", "$(projectdir)/3rdparty/qwindowkitty/src")

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
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/qwkglobal_p.h")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/qwkglobal.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/windowagentbase.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/windowitemdelegate.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/kernel/nativeeventfilter.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/kernel/sharedeventfilter.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/kernel/winidchangeeventfilter.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/abstractwindowcontext.cpp")
    if has_config("style_agent") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/style/styleagent.cpp")
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/style/styleagent_mac.mm")
    end
    if is_plat("windows") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/qwindowkit_windows.h")
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/qwindowkit_windows.cpp")
    end

    -- Add header files that need MOC processing (use add_files for Q_OBJECT headers)
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/windowagentbase.h")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/windowagentbase_p.h")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/abstractwindowcontext_p.h")

    if is_plat("macosx") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/cocoawindowcontext_p.h")
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/cocoawindowcontext.mm")
    end
    if is_plat("linux") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/qtwindowcontext_p.h")
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/qtwindowcontext.cpp")
    end
    if is_plat("windows") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/win32windowcontext_p.h")
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/contexts/win32windowcontext.cpp")
    end
    if is_plat("windows") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/shared/qwkwindowsextra_p.h")
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/shared/windows10borderhandler_p.h")
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/shared/systemwindow_p.h")
    end
   

    if has_config("style_agent") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/core/style/styleagent.h")
    end

    -- Set install headers
    add_headerfiles("$(builddir)/include/QWKCore/**.h", {prefixdir = "QWKCore"})
    add_headerfiles("$(builddir)/include/QWKCore/private/**.h", {prefixdir = "QWKCore/private"})

    on_install(function (target)
    end)
target_end()
