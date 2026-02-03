-------------------------------------------------------------------------------
--
-- MODULE      : goldfish.lua
-- DESCRIPTION : goldfish scheme
-- COPYRIGHT   : (C) 2025  Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

target ("goldfish") do
    set_languages("c++17")
    set_targetdir("$(projectdir)/TeXmacs/plugins/goldfish/bin/")
    add_files ("$(projectdir)/TeXmacs/plugins/goldfish/src/goldfish.cpp")
    add_packages("s7")
    add_packages("tbox")
    add_packages("cpr")
    add_packages("argh")
    on_install(function (target)
    end)
end
