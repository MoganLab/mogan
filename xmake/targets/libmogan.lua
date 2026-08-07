-------------------------------------------------------------------------------
--
-- MODULE      : libmogan.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

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
    if is_plat("windows") and is_arch("x64") then
        -- Velopack C++ runtime：头文件路径供后续 tm_velopack 使用；链接项对静态库仅
        -- 在最终 exe 链接时生效，stem 已自带，此处双保险。
        add_velopack_runtime ()
    end
    set_languages("c++17")
    set_policy("check.auto_ignore_flags", false)
    set_encodings("utf-8")

    add_deps("libmoebius")
    add_deps("liblolly")
    add_deps("goldfish")
    if has_config("loro") then
        add_defines("LORO_ENABLED")
    end
    -- Loro 同步的 WS 传输层（src/Plugins/WebSocket）：native 用 libcurl 实现，
    -- WASM 用 emscripten WebSocket API 实现（emcc 内置库，无需包依赖）。
    if has_config("loro") then
        if not is_plat("wasm") then
            add_packages("libcurl", {public = true})
        end
    end
    if has_config("qt_frontend") then
        add_deps("QWKCore", "QWKWidgets")
        add_rules("qt.static")
        --add_packages("qt6base", "qt6core", "qt6gui", "qt6widgets")
        add_frameworks("QtGui", "QtWidgets", "QtCore", "QtPrintSupport", "QtSvg", "QtNetwork", "QtNetworkAuth")
        add_frameworks("QtQml", "QtQuick", "QtQuickWidgets", "QtBodymovin")
    elseif has_config("cli_frontend") then
        set_kind("static")
    else
        add_deps("imgui")
        set_kind("static")
    end
    if has_config("goldfish") and not is_plat("wasm") then
        add_defines("GOLDFISH_ENABLE_HTTP")
    end

    set_policy("check.auto_ignore_flags", false)
    on_install(function (target)
        print("No need to install libmogan")
    end)

    add_rules("mogan.glue")
    add_files("$(projectdir)/src/Scheme/L2/glue_lolly.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L3/glue_drd.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L3/glue_file.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L3/glue_misc.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L3/glue_modification.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L3/glue_moebius.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L3/glue_patch.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L3/glue_url.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L4/glue_convert.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L4/glue_tree.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/L5/glue_widget.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Glue/glue_basic.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Glue/glue_editor.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Glue/glue_font.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Glue/glue_server.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Plugins/glue_bibtex.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Plugins/glue_ghostscript.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Plugins/glue_html.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Plugins/glue_plugin.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Plugins/glue_tex.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Plugins/glue_updater.lua", {rule = "mogan.glue"})
    add_files("$(projectdir)/src/Scheme/Plugins/glue_xml.lua", {rule = "mogan.glue"})
    if has_config("pdfhummus") then
        add_files("$(projectdir)/src/Scheme/Plugins/glue_pdf.lua", {rule = "mogan.glue"})
    end
    if has_config("qt_frontend") then
        add_files("$(projectdir)/src/Scheme/L5/glue_qt.lua", {rule = "mogan.glue"})
        set_configvar("QTTEXMACS", 1)
        add_defines("QTTEXMACS")
        set_configvar("QTPIPES", 1)
        add_defines("QTPIPES")
        set_configvar("USE_QT_PRINTER", 1)
        add_defines("USE_QT_PRINTER")
    elseif has_config("cli_frontend") then
        set_configvar("MOGAN_CLI", 1)
        add_defines("MOGAN_CLI")
    elseif not is_plat("wasm") then -- WASM GLFW is in EMCC
        add_packages("glfw")
    end

    if has_config("pdfhummus") then
        add_packages("liii-pdfhummus")
    end
    add_packages("freetype")
    add_packages("liii-tbox")
    if not is_plat("wasm") then
        add_packages("cpr")
    end
    add_packages("argh", {public = true})
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
    set_configdir("$(builddir)")
    -- check for dl library
    -- configvar_check_cxxfuncs("TM_DYNAMIC_LINKING","dlopen")
    configvar_check_cxxincludes("HAVE_INTTYPES_H", "inttypes.h")

    set_configvar("STDC_HEADERS", true)

    set_configvar("GS_EXE", "/usr/bin/gs")

    set_configvar("PDFHUMMUS_NO_TIFF", true)

    if is_mode("debug") or is_mode("releasedbg") then
        set_configvar("LIII_DEBUG", 1)
    end

    add_configfiles(
        "$(projectdir)/src/System/config.h.xmake", {
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
                USE_PLUGIN_TEX = true,
                USE_PLUGIN_ISPELL = true,
                USE_PLUGIN_PDF = has_config("pdfhummus"),
                USE_PLUGIN_VELOPACK = is_plat("windows") and is_arch("x64"),
                USE_PLUGIN_HTML = true,
                USE_MUPDF_RENDERER = has_config("mupdf"),
                USE_STARTUP_TAB = has_config("startup_tab"),
                USE_TEXT_TOOLBAR = has_config("text_toolbar"),
                USE_TUTORIAL = not has_config("is_community"),
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

    if not is_plat("wasm") then
        configvar_check_cxxsnippets(
            "CONFIG_LARGE_POINTER", [[
                #include <stdlib.h>
                static_assert(sizeof(void*) == 8, "");]])
    else
        -- WASM use 32 bit pointers
        configvar_check_cxxsnippets(
            "CONFIG_LARGE_POINTER", [[
                #include <stdlib.h>
                static_assert(sizeof(void*) == 4, "");]])
    end
    add_configfiles(
        "$(projectdir)/src/System/tm_configure.hpp.xmake", {
            filename = "tm_configure.hpp",
            pattern = "@(.-)@",
            variables = {
                TEXMACS_VERSION = TEXMACS_VERSION,
                XMACS_VERSION = XMACS_VERSION,
                CACHE_NAME = stem_lab_big_name,
                STEM_NAME = stem_binary_name,
                STEM_INIT_FILE = stem_init_file or "init-research.scm",
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
    add_includedirs("$(builddir)", {public = true})
    add_includedirs({
            "$(projectdir)/src/Data/Convert",
            "$(projectdir)/src/Data/Document",
            "$(projectdir)/src/Data/History",
            "$(projectdir)/src/Data/Observers",
            "$(projectdir)/src/Data/Parser",
            "$(projectdir)/src/Data/String",
            "$(projectdir)/src/Data/Tree",
            "$(projectdir)/src/Edit",
            "$(projectdir)/src/Edit/Editor",
            "$(projectdir)/src/Edit/Interface",
            "$(projectdir)/src/Edit/Modify",
            "$(projectdir)/src/Edit/Process",
            "$(projectdir)/src/Edit/Replace",
            "$(projectdir)/src/Graphics/Bitmap_fonts",
            "$(projectdir)/src/Graphics/Colors",
            "$(projectdir)/src/Graphics/Fonts",
            "$(projectdir)/src/Graphics/Gui",
            "$(projectdir)/src/Graphics/Handwriting",
            "$(projectdir)/src/Graphics/Mathematics",
            "$(projectdir)/src/Graphics/Pictures",
            "$(projectdir)/src/Graphics/Renderer",
            "$(projectdir)/src/Graphics/Spacial",
            "$(projectdir)/src/Graphics/Types",
            "$(projectdir)/src/Kernel/Abstractions",
            "$(projectdir)/src/Kernel/Types",
            "$(projectdir)/src/Plugins",
            "$(projectdir)/src/Plugins/Pdf",
            "$(projectdir)/src/Plugins/Html",
            "$(projectdir)/src/Scheme",
            "$(projectdir)/src/Scheme/L2",
            "$(projectdir)/src/Scheme/L3",
            "$(projectdir)/src/Scheme/L4",
            "$(projectdir)/src/Scheme/L5",
            "$(projectdir)/src/Scheme/Plugins",
            "$(projectdir)/src/Scheme/S7",
            "$(projectdir)/src/Scheme/Scheme",
            "$(projectdir)/src/System",
            "$(projectdir)/src/System/Boot",
            "$(projectdir)/src/System/Classes",
            "$(projectdir)/src/System/Config",
            "$(projectdir)/src/System/Files",
            "$(projectdir)/src/System/Language",
            "$(projectdir)/src/System/Link",
            "$(projectdir)/src/System/Misc",
            "$(projectdir)/src/Texmacs",
            "$(projectdir)/src/Texmacs/Data",
            "$(projectdir)/src/Typeset",
            "$(projectdir)/src/Typeset/Bridge",
            "$(projectdir)/src/Typeset/Concat",
            "$(projectdir)/src/Typeset/Page",
            "$(projectdir)/src/Mogan/Cache",
            "$(projectdir)/src/Mogan/HashUtils",
            "$(projectdir)/src/Mogan/TemplateCenter",
            "$(projectdir)/src/Mogan/Telemetry",
            "$(projectdir)/TeXmacs/include",
            "$(builddir)/glue"
        }, {public = true})

    add_files({
            "$(projectdir)/src/Data/**.cpp",
            "$(projectdir)/src/Edit/**.cpp",
            "$(projectdir)/src/Graphics/**.cpp",
            "$(projectdir)/src/Kernel/**.cpp",
            "$(projectdir)/src/Scheme/Scheme/**.cpp",
            "$(projectdir)/src/Scheme/S7/**.cpp",
            "$(projectdir)/src/Scheme/L2/**.cpp",
            "$(projectdir)/src/Scheme/L3/**.cpp",
            "$(projectdir)/src/Scheme/L4/**.cpp",
            "$(projectdir)/src/Scheme/L5/**.cpp",
            "$(projectdir)/src/Scheme/Plugins/**.cpp",
            "$(projectdir)/src/System/**.cpp",
            "$(projectdir)/src/Texmacs/Data/**.cpp",
            "$(projectdir)/src/Texmacs/Server/**.cpp",
            "$(projectdir)/src/Texmacs/Window/**.cpp",
            "$(projectdir)/src/Typeset/**.cpp",
            "$(projectdir)/src/Plugins/Bibtex/**.cpp",
            "$(projectdir)/src/Plugins/Freetype/**.cpp",
            "$(projectdir)/src/Plugins/Ghostscript/**.cpp",
            "$(projectdir)/src/Plugins/Ispell/**.cpp",
            "$(projectdir)/src/Plugins/Metafont/**.cpp",
            "$(projectdir)/src/Plugins/Tex/**.cpp",
            "$(projectdir)/src/Plugins/Xml/**.cpp",
            "$(projectdir)/src/Plugins/Html/**.cpp",
            "$(projectdir)/src/Plugins/Updater/**.cpp",
            "$(projectdir)/src/Mogan/HashUtils/**.cpp"})

    if has_config("pdfhummus") then
        add_includedirs("$(projectdir)/src/Plugins/Pdf/**.hpp", {public=true})
        add_files("$(projectdir)/src/Plugins/Pdf/**.cpp")
    end

    if has_config("qt_frontend") then
        add_includedirs("$(projectdir)/src/Plugins/Qt", {public=true})
        add_files("$(projectdir)/src/Plugins/Qt/**.cpp", "$(projectdir)/src/Plugins/Qt/**.hpp")
        add_files("$(projectdir)/src/Mogan/Cache/**.cpp", "$(projectdir)/src/Mogan/Cache/**.hpp")
        add_files("$(projectdir)/src/Mogan/TemplateCenter/**.cpp", "$(projectdir)/src/Mogan/TemplateCenter/**.hpp")
        -- Qt resource files
        add_rules("qt.qrc")
        add_files("$(projectdir)/TeXmacs/misc/images/images.qrc")
        add_files("$(projectdir)/src/Plugins/Qt/moganqml.qrc")
    elseif has_config("cli_frontend") then
        add_files("$(projectdir)/src/Plugins/CLI/**.cpp")
    else
        add_files("$(projectdir)/src/Plugins/ImGui/**.cpp")
        if is_plat("macosx") then
            add_files("$(projectdir)/src/Plugins/ImGui/**.mm")
        end
    end

    if has_config("loro") then
        add_includedirs("$(projectdir)/src/Plugins/WebSocket", {public=true})
        if is_plat("wasm") then
            add_includedirs("$(projectdir)/src/Plugins/WebSocket/emscripten", {public=true})
            add_files("$(projectdir)/src/Plugins/WebSocket/emscripten/*.cpp")
        else
            add_includedirs("$(projectdir)/src/Plugins/WebSocket/libcurl", {public=true})
            add_files("$(projectdir)/src/Plugins/WebSocket/libcurl/*.cpp")
        end
        add_includedirs("$(projectdir)/src/Plugins/Collab", {public=true})
        add_files("$(projectdir)/src/Plugins/Collab/loro_collab.cpp")
        add_files("$(projectdir)/src/Plugins/Collab/loro_collab_ws.cpp")
        if is_plat("wasm") then
            add_files("$(projectdir)/src/Plugins/Collab/loro_collab_docs_wasm.cpp")
        else
            add_files("$(projectdir)/src/Plugins/Collab/loro_collab_docs_native.cpp")
        end
        add_files("$(projectdir)/src/Scheme/Plugins/glue_collab.lua", {rule = "mogan.glue"})
    end

    if is_plat("macosx") then
        plugin_macos_srcs = {
            "$(projectdir)/src/Plugins/MacOS/HIDRemote.m",
            "$(projectdir)/src/Plugins/MacOS/mac_spellservice.mm",
            "$(projectdir)/src/Plugins/MacOS/mac_utilities.mm",
            "$(projectdir)/src/Plugins/MacOS/mac_app.mm"
        }
        add_includedirs("$(projectdir)/src/Plugins/MacOS", {public = true})
        add_files(plugin_macos_srcs)
    end
    if is_plat("macosx", "linux", "windows") and has_config("qt_frontend") then
        add_includedirs("$(projectdir)/src/Plugins/QWindowKit", {public = true})
        add_files("$(projectdir)/src/Plugins/QWindowKit/**.cpp")
        add_files("$(projectdir)/src/Plugins/QWindowKit/**.hpp")
    end

    if has_config("mupdf") then
        add_includedirs({
            "$(projectdir)/src/Plugins/MuPDF"
        }, {public = true})
        add_files({
            "$(projectdir)/src/Plugins/MuPDF/*.cpp",
        })
        if has_config("qt_frontend") then
            add_files({
                "$(projectdir)/src/Plugins/MuPDF/Qt/*.cpp"
            })
        end
        add_packages("mupdf")
    end

    add_mxflags("-fno-objc-arc")
    if is_plat("wasm") then
        add_cxxflags("--use-port=contrib.glfw3")
        add_ldflags("--use-port=contrib.glfw3")
        add_ldflags("-lwebsocket.js")
    end
    on_load(function (target)
        target:add("forceincludes", path.absolute("$(builddir)/config.h"))
        target:add("forceincludes", path.absolute("$(builddir)/tm_configure.hpp"))
    end)
end 
