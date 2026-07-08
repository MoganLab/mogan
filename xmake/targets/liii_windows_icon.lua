-------------------------------------------------------------------------------
--
-- MODULE      : liii_windows_icon.lua
-- DESCRIPTION : variables for STEM
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

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
    add_files("$(builddir)/resource.rc")
end