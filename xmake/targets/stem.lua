-------------------------------------------------------------------------------
--
-- MODULE      : stem.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

local stem_files = {
    "$(projectdir)/TeXmacs(/doc/**)",
    "$(projectdir)/TeXmacs(/langs/**)",
    "$(projectdir)/TeXmacs(/misc/**)",
    "$(projectdir)/TeXmacs(/packages/**)",
    "$(projectdir)/TeXmacs(/progs/**)",
    "$(projectdir)/TeXmacs(/styles/**)",
    "$(projectdir)/TeXmacs(/texts/**)",
    "$(projectdir)/TeXmacs(/templates/**)",
    "$(projectdir)/TeXmacs/COPYING", -- copying files are different
    "$(projectdir)/TeXmacs/INSTALL",
    "$(projectdir)/LICENSE", -- license files are same
    "$(projectdir)/TeXmacs/README",
    "$(projectdir)/TeXmacs/TEX_FONTS",
    "$(projectdir)/TeXmacs(/plugins/**)" -- plugin files
}

target("stem") do 
    if is_plat("windows") and is_mode("release") then
        add_deps("liii_windows_icon")
    end
    if is_plat("linux") then
        set_filename(stem_binary_linux)
    elseif is_plat("macosx") then
        set_filename(stem_binary_macos)
    elseif is_plat("wasm") then
        set_filename(stem_binary_wasm)
    else
        set_filename(stem_binary_windows)
    end

    local install_dir = "$(builddir)"
    if is_plat("windows") then
        install_dir = path.join("$(builddir)", "packages/stem/data/")
    elseif is_plat("macosx") then
        install_dir = path.join("$(builddir)", "macosx/$(arch)/$(mode)/" .. stem_binary_name .. ".app/Contents/Resources/")
    else
        if os.getenv("INSTALL_DIR") == nil then
            install_dir = path.join("$(builddir)", "packages/stem/")
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
    
    if is_plat("wasm") then
        add_cxxflags("-O3")
        add_cxxflags("--use-port=contrib.glfw3")
        add_ldflags("-O3")
        add_ldflags("--use-port=contrib.glfw3")
        add_ldflags("-sINITIAL_MEMORY=512MB")
        add_ldflags("-sALLOW_MEMORY_GROWTH=1")
        add_ldflags("-sSTACK_SIZE=32MB", {force = true})
        -- Export the C functions the React shell (web/) calls back into via
        -- Module.ccall, plus the runtime helpers (ccall/cwrap, UTF8_ToString).
        -- Only the three React-shell entry points are listed here; the IME,
        -- file-chooser and collab bridges keep themselves alive via
        -- EMSCRIPTEN_KEEPALIVE in their own TUs and are already exported that
        -- way (hardcoding them here would break builds with --loro=no etc.).
        add_ldflags("-sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString", {force = true})
        add_ldflags("-sEXPORTED_FUNCTIONS=" ..
            "_main," ..
            "_mogan_menu_invoke," ..
            "_mogan_menu_expand," ..
            "_mogan_menu_close_popup," ..
            "_mogan_set_chrome_metrics", {force = true})
        add_ldflags("--preload-file=" .. path.join(os.projectdir(), "TeXmacs/doc/about/mogan/stem.en.tmu") .. "@/TeXmacs/doc/about/mogan/stem.en.tmu")
        add_ldflags("--preload-file=" .. path.join(os.projectdir(), "TeXmacs/progs") .. "@/TeXmacs/progs")
        add_ldflags("--preload-file=" .. path.join(os.projectdir(), "TeXmacs/langs") .. "@/TeXmacs/langs")
        add_ldflags("--preload-file=" .. path.join(os.projectdir(), "TeXmacs/packages") .. "@/TeXmacs/packages")
        add_ldflags("--preload-file=" .. path.join(os.projectdir(), "TeXmacs/styles") .. "@/TeXmacs/styles")
        add_ldflags("--preload-file=" .. path.join(os.projectdir(), "TeXmacs/texts") .. "@/TeXmacs/texts")
        add_ldflags("--preload-file=" .. path.join(os.projectdir(), "TeXmacs/templates") .. "@/TeXmacs/templates")
        add_ldflags(
            "--preload-file=" .. path.join(os.projectdir(), "TeXmacs/fonts") .. "@/TeXmacs/fonts",
            "--exclude-file=*/fonts/truetype/Mogan-NotoColorEmoji.ttf",
            "--exclude-file=*/fonts/opentype/cm-unicode/*"
        )
        local plugins_dir = path.join(os.projectdir(), "TeXmacs/plugins")
        for _, dir in ipairs(os.dirs(path.join(plugins_dir, "*"))) do
            local name = path.filename(dir)
            if name ~= "goldfish" then
                add_ldflags("--preload-file=" .. dir .. "@/TeXmacs/plugins/" .. name)
            end
        end
        add_ldflags("--preload-file=" .. path.join(plugins_dir, "goldfish/goldfish") .. "@/TeXmacs/plugins/goldfish/goldfish")
    end

    if has_config("qt_frontend") then
        -- Qt frontend: build stem as a Qt app.
        if is_mode("debug", "releasedbg") and is_plat("windows") then
            add_rules("qt.console")
        else
            add_rules("qt.widgetapp")
        end
        add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtNetwork", "QtNetworkAuth")
        add_frameworks("QtQml", "QtQuick", "QtBodymovin")
    end

    add_packages("mupdf")
    if not has_config("qt_frontend") and not has_config("cli_frontend") and not is_plat("wasm") then -- WASM GLFW is in EMCC
        add_packages("glfw")
    end
    if has_config("goldfish") then
        add_deps("goldfish-bin")
    end
    add_deps("goldfish")
    add_deps("liblolly")
    add_deps("libmogan")
    add_deps("libmoebius")
    -- WASM: build the React shell (web/) before linking so the after_build
    -- hook can copy web/dist into the target dir. No-op on other platforms.
    if is_plat("wasm") then
        add_deps("web_shell")
    end
    if not is_plat("windows") then
        add_syslinks("pthread", "dl", "m")
    end
    if is_plat("linux") then
        add_syslinks("X11")
    end
    -- CLI 前端无 Qt/glfw 自带的 macOS 框架；Plugins/MacOS（HIDRemote/mac_utilities/
    -- mac_spellservice）仍被编译，需显式链接 Cocoa(=AppKit+Foundation) 与 IOKit。
    if is_plat("macosx") and has_config("cli_frontend") then
        add_frameworks("Cocoa", "IOKit")
    end

    add_includedirs("$(builddir)", {public = true})
    add_files("$(projectdir)/src/Mogan/Research/research.cpp")

    -- Velopack C++ runtime：启动钩子编译/链接 + 动态库随 bin/ 发布
    if is_plat("windows") and is_arch("x64") then
        add_velopack_runtime ()
        -- 导入库内嵌 DLL 名为 velopack_libc.dll，发布时改名，exe 才能加载
        add_installfiles ("$(projectdir)/3rdparty/velopack/lib/velopack_libc_win_x64_msvc.dll",
                          {prefixdir = "bin", filename = "velopack_libc.dll"})
    end

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
    if is_plat("windows") and has_config("qt_frontend")  then
        set_values("qt.deploy.flags", {"-printsupport", "--no-opengl-sw", "--no-translations", "--release"})
    end
    if is_plat("windows") and has_config("loro") then
        -- /WHOLEARCHIVE 会把 staticlib 中重复的 bcryptprimitives.dll 导入桩全部
        -- 拉入导致 LNK2005，/FORCE:MULTIPLE 取首个定义规避（rust#129218）
        add_ldflags("/WHOLEARCHIVE:mogan_loro_ffi.lib", "/FORCE:MULTIPLE", {force = true})
    end

    after_build(function (target)
        if is_plat("wasm") then
            local web_dist = path.join(os.projectdir(), "web", "dist")
            local legacy_shell = path.join(os.projectdir(), "tools", "wasm", "stem.html")
            local td = target:targetdir()
            if os.exists(path.join(web_dist, "index.html")) then
                -- React shell (Vite build of web/). Flatten web/dist into the
                -- target dir so stem.js / stem.wasm / stem.data and the React
                -- assets all sit side by side. The dev server (wasm_server.py)
                -- and the CD workflow both load stem.html as the entry point,
                -- so mirror the built index.html to stem.html.
                cprint("${yellow}installing React shell from web/dist${clear}")
                os.cp(path.join(web_dist, "*"), td)
                os.cp(path.join(td, "index.html"), path.join(td, "stem.html"))
            else
                -- Fallback: no JS build available (e.g. a C++-only CI branch).
                cprint("${yellow}web/dist not found; using legacy stem.html${clear}")
                cprint("${yellow}Run `npm run build` in web/ to enable the React shell.${clear}")
                os.cp(legacy_shell, td)
            end
        end
    end)

    on_run(function (target)
        import("core.base.option")

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

        -- Append user-provided arguments from `xmake run`
        local args = option.get("arguments")
        if args then
            for _, arg in ipairs(args) do
                table.insert(params, arg)
            end
        end

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
            elseif is_plat("wasm") then
                local build_dir = path.absolute(path.directory(binary))
                os.execv("python3", {
                    path.join(os.projectdir(), "bin/wasm_server.py"),
                    build_dir
                })
            else
                print("Unsupported platform: " .. tostring(os.host()))
            end
        end
    end)

    on_load(function (target)
        target:add("forceincludes", path.absolute("$(builddir)/config.h"))
        target:add("forceincludes", path.absolute("$(builddir)/tm_configure.hpp"))
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
        elseif is_plat("macosx") then
            local duplicate_binary = path.join(target:installdir(), "bin", target:filename())
            if os.isfile(duplicate_binary) then
                -- qt.widgetapp already places the real app executable in Contents/MacOS.
                -- Remove the extra installed copy from Contents/Resources/bin to avoid
                -- shipping two app binaries inside the bundle.
                os.rm(duplicate_binary)
                print("Removed duplicate app binary: " .. duplicate_binary)
            end
        end
    end)
end
