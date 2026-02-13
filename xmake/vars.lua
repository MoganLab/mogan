-------------------------------------------------------------------------------
--
-- MODULE      : vars.lua
-- DESCRIPTION : Xmake config file for variables
-- COPYRIGHT   : (C) 2022-2024  jingkaimori
--                   2022-2024  Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

XMACS_VERSION="2026.2.0-rc6"
TEXMACS_VERSION = "2.1.2"
GOLDFISH_VERSION = "17.11.2"
DEVEL_VERSION = TEXMACS_VERSION
DEVEL_RELEASE = 1
STABLE_VERSION = TEXMACS_VERSION
STABLE_RELEASE = 1

-- LiiiLabs dependencies
TBOX_VERSION = "1.7.5"
LOLLY_VERSION = "1.4.27"

-- Third-party dependencies
CPPTRACE_VERSION = "v0.7.1"
S7_VERSION = "20240816"
TREESITTER_VERSION = "0.22.6"
TREESITTER_CPP_VERSION = "0.22.2"
TREESITTER_SCHEME_VERSION = "0.6.2"
QT6_VERSION = "6.5.3"
QTIFW_VERSION = "4.6.0"
CPR_VERSION = "1.10.5"
CURL_VERSION = "8.4.0"
PDFHUMMUS_VERSION = "4.6.2"
FREETYPE_VERSION = "2.13.3"
LIBPNG_VERSION = "1.6.37"
LIBJPEG_VERSION = "v9e"
LIBICONV_VERSION = "1.17"
LIBGIT2_VERSION = "1.7.1"

-- Linux system verison
CURRENT_DEBIAN_VERSION = 13
CURRENT_UBUNTU_VERSION = 25

TM_CONFIGURE_VARS = {
    TEXMACS_VERSION = TEXMACS_VERSION,
    GOLDFISH_VERSION = GOLDFISH_VERSION,
    XMACS_VERSION = XMACS_VERSION,
    tm_devel = "Texmacs-" .. DEVEL_VERSION,
    tm_devel_release = "Texmacs-" .. DEVEL_VERSION .. "-" .. DEVEL_RELEASE,
    tm_stable = "Texmacs-" .. STABLE_VERSION,
    tm_stable_release = "Texmacs-" .. STABLE_VERSION .. "-" .. STABLE_RELEASE,
    LOLLY_VERSION = LOLLY_VERSION,
}

RUN_ENVS = {TEXMACS_PATH=path.join(os.projectdir(), "TeXmacs")}

-- Source Code
libstem_headers = {
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
    "$(projectdir)/src/Mogan",
    "$(projectdir)/src/Plugins",
    "$(projectdir)/src/Scheme",
    "$(projectdir)/src/Scheme/S7",
    "$(projectdir)/src/Scheme/L2",
    "$(projectdir)/src/Scheme/L3",
    "$(projectdir)/src/Scheme/L4",
    "$(projectdir)/src/Scheme/L5",
    "$(projectdir)/src/Scheme/Plugins",
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
    "$(projectdir)/src/Texmacs/Server",
    "$(projectdir)/src/Texmacs/Window",
    "$(projectdir)/src/Typeset",
    "$(projectdir)/src/Typeset/Boxes",
    "$(projectdir)/src/Typeset/Bridge",
    "$(projectdir)/src/Typeset/Concat",
    "$(projectdir)/src/Typeset/Env",
    "$(projectdir)/src/Typeset/Format",
    "$(projectdir)/src/Typeset/Line",
    "$(projectdir)/src/Typeset/Page",
    "$(projectdir)/src/Typeset/Stack",
    "$(projectdir)/src/Typeset/Table",
    "$(buildir)/glue",
    "$(projectdir)/TeXmacs/include",
    "$(projectdir)/TeXmacs/plugins/goldfish/src",
}

libstem_srcs = {
    "$(projectdir)/src/Data/**.cpp",
    "$(projectdir)/src/Edit/**.cpp",
    "$(projectdir)/src/Graphics/**.cpp",
    "$(projectdir)/src/Kernel/**.cpp",
    "$(projectdir)/src/Mogan/**.cpp",
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
} 

moe_files = {
    "$(projectdir)/moebius/Data/History/**.cpp",
    "$(projectdir)/moebius/Data/String/**.cpp",
    "$(projectdir)/moebius/Data/Tree/**.cpp",
    "$(projectdir)/moebius/Kernel/Types/**.cpp",
    "$(projectdir)/moebius/Kernel/Abstractions/**.cpp",
    "$(projectdir)/moebius/Scheme/**.cpp",
    "$(projectdir)/moebius/moebius/**.cpp",
}

moe_includedirs = {
    "$(projectdir)/moebius/Data/History",
    "$(projectdir)/moebius/Data/String",
    "$(projectdir)/moebius/Data/Tree",
    "$(projectdir)/moebius/Kernel/Types",
    "$(projectdir)/moebius/Kernel/Abstractions",
    "$(projectdir)/moebius/Scheme",
    "$(projectdir)/moebius/Scheme/L1",
    "$(projectdir)/moebius/Scheme/L2",
    "$(projectdir)/moebius/Scheme/L3",
    "$(projectdir)/moebius/Scheme/S7",
    "$(projectdir)/moebius/Scheme/Scheme",
    "$(projectdir)/moebius/",
}

