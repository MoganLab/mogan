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
    add_files({
        "$(projectdir)/3rdparty/s7/s7.c",
        "$(projectdir)/3rdparty/s7/s7_scheme_base.c",
        "$(projectdir)/3rdparty/s7/s7_scheme_inexact.c",
        "$(projectdir)/3rdparty/s7/s7_scheme_complex.c",
        "$(projectdir)/3rdparty/s7/s7_scheme_char.c",
        "$(projectdir)/3rdparty/s7/s7_liii_bitwise.c",
        "$(projectdir)/3rdparty/s7/s7_liii_string.c",
        "$(projectdir)/3rdparty/s7/s7_liii_hash_table.c",
    }, {languages = "c11"})
    add_files({
        "$(projectdir)/3rdparty/json-schema-validator/src/smtp-address-validator.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-schema-draft7.json.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-uri.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-validator.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/json-patch.cpp",
        "$(projectdir)/3rdparty/json-schema-validator/src/string-format-check.cpp",
    })
    add_includedirs({
        "$(projectdir)/TeXmacs/plugins/goldfish/src",
        "$(projectdir)/3rdparty/s7",
        "$(projectdir)/3rdparty/nlohmann_json/include",
        "$(projectdir)/3rdparty/json-schema-validator/src",
    })

    add_defines("WITH_SYSTEM_EXTRAS=0")
    add_defines("HAVE_OVERFLOW_CHECKS=0")
    add_defines("WITH_WARNINGS")
    add_defines("WITH_R7RS=1")
    if is_mode("debug") then
        add_defines("S7_DEBUGGING")
    end

    if is_plat("linux") then
        add_syslinks("stdc++")
    end
    add_packages("tbox")
    add_packages("cpr")
    add_packages("argh")
    on_install(function (target)
    end)
end
