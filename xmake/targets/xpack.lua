-------------------------------------------------------------------------------
--
-- MODULE      : xpack.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

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
