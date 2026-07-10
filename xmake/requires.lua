-------------------------------------------------------------------------------
--
-- MODULE      : requires.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

add_requires("goldfish", {system=false})
add_requires("liii-tbox", {system=false})
if not is_plat("wasm") then
    add_requires("cpr", {system=false})
end
-- QWK is built locally from 3rdparty/qwindowkitty, no external package needed
if is_plat ("windows") then
    add_requires("libiconv "..LIBICONV_VERSION, {system=false})
end

add_requires("libjpeg")
-- apt系统用系统包，其他发行版用源码构建
if is_plat("linux") and (linuxos.name() == "ubuntu" or linuxos.name() == "debian" or linuxos.name() == "uos") then
    add_requires("apt::libpng-dev", {alias="libpng"})
    add_requires("apt::libcurl4-openssl-dev", {alias="libcurl"})
elseif is_plat("linux") and (linuxos.name() == "fedora" or linuxos.name() == "rhel" or linuxos.name() == "centos" or linuxos.name() == "rocky" or linuxos.name() == "almalinux" or linuxos.name() == "ol") then
    add_requires("libpng", {system=true})
    add_requires("libcurl", {system=true})
else
    add_requires("libpng", {system=false})
    if not is_plat("wasm") then
        add_requires("libcurl", {system=false})
    end
end

if has_config("pdfhummus") then
    add_requires("liii-pdfhummus", {system=false,configs={libpng=true,libjpeg=true}})
    add_requires("freetype "..FREETYPE_VERSION, {system=false, configs={png=true}})
    add_requireconfs("liii-pdfhummus.freetype", {version = FREETYPE_VERSION, system = false, configs={png=true}, override=true})
end

add_requires("argh v1.3.2")

if has_config("qt_frontend") then
    QT6_VERSION="6.8.3"
    add_requires("qt6widgets "..QT6_VERSION)
elseif not is_plat("wasm") then -- WASM GLFW is in EMCC
    add_requires("glfw")
end

if has_config("mupdf") then
    if (linuxos.name() == "debian" and linuxos.version():major() >= CURRENT_DEBIAN_VERSION) or
       (linuxos.name() == "ubuntu" and linuxos.version():major() >= CURRENT_UBUNTU_VERSION)
    then
        add_requires("apt::libmupdf-dev", {alias="mupdf"})
    else
        add_requires("mupdf", {system=false})
    end
end