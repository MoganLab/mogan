-------------------------------------------------------------------------------
--
-- MODULE      : qwkwidgets.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

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

        os.mkdir(path.join(buildir, "include/QWKWidgets"))
        os.mkdir(path.join(buildir, "include/QWKWidgets/ui/widgetframe"))
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
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/widgets/*.h", path.join(buildir, "include/QWKWidgets/"))
        safe_vcp("$(projectdir)/3rdparty/qwindowkitty/src/ui/widgetframe/*.h", path.join(buildir, "include/QWKWidgets/ui/widgetframe/"))
    end)

    -- Include directories
    add_includedirs("$(builddir)/include", {public = true})
    add_includedirs("$(projectdir)/3rdparty/qwindowkitty/src/widgets", "3rdparty/qwindowkitty/src")

    -- Source files
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/widgets/widgetitemdelegate.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/widgets/widgetwindowagent_p.h")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/widgets/widgetwindowagent.h")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/widgets/widgetwindowagent.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/ui/widgetframe/windowbar.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/ui/widgetframe/windowbar.h")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/ui/widgetframe/windowbar_p.h")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/ui/widgetframe/windowbutton.cpp")
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/ui/widgetframe/windowbutton.h")
    if is_plat("macosx") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/widgets/widgetwindowagent_mac.cpp")
    end
    if is_plat("windows") then
        add_files("$(projectdir)/3rdparty/qwindowkitty/src/widgets/widgetwindowagent_win.cpp")
    end
    add_files("$(projectdir)/3rdparty/qwindowkitty/src/styles/styles.qrc")

    -- Set install headers
    add_headerfiles("$(builddir)/include/QWKWidgets/**.h", {prefixdir = "QWKWidgets"})

    on_install(function (target)
    end)
target_end()
