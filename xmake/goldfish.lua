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
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_hashlib.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_http.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_njson.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_os.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_path.cpp",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/liii_subprocess.cpp",
    })
    add_files({
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_continuation.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_ctables.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_dtoa.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_module.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_op_names.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_base.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_char.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_complex.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_format.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_inexact.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_predicate.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_symbol.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_scheme_write.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_bitwise.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_hash_table.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_list.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_string.c",
        "$(projectdir)/TeXmacs/plugins/goldfish/src/s7_liii_vector.c",
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
