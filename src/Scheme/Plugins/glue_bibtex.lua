-------------------------------------------------------------------------------
--
-- MODULE      : glue_bibtex.lua
-- DESCRIPTION : Generating glue on src/Plugins/BibTex
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_bibtex",
        binding_object = "",
        initializer_name = "initialize_glue_bibtex",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "Bibtex/bibtex.hpp",
            "Bibtex/bibtex_functions.hpp",
        },
        glues = {
            {
                scm_name = "parse-bib",
                cpp_name = "parse_bib",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "conservative-bib-import",
                cpp_name = "conservative_bib_import",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "content",
                    "string"
                }
            },
            {
                scm_name = "conservative-bib-export",
                cpp_name = "conservative_bib_export",
                ret_type = "string",
                arg_list = {
                    "content",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "set-bibtex-command",
                cpp_name = "set_bibtex_command",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "supports-bibtex?",
                cpp_name = "bibtex_present",
                ret_type = "bool"
            },
            {
                scm_name = "bibtex-run",
                cpp_name = "bibtex_run",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "string",
                    "url",
                    "array_string"
                }
            },
            {
                scm_name = "bib-purify",
                cpp_name = "bib_purify",
                ret_type = "string",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-prefix",
                cpp_name = "bib_prefix",
                ret_type = "string",
                arg_list = {
                    "scheme_tree",
                    "int"
                }
            },
            {
                scm_name = "bib-locase",
                cpp_name = "bib_locase",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-upcase",
                cpp_name = "bib_upcase",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-default-upcase-first",
                cpp_name = "bib_default_upcase_first",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-default-preserve-case",
                cpp_name = "bib_default_preserve_case",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-text-length",
                cpp_name = "bib_text_length",
                ret_type = "int",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-empty?",
                cpp_name = "bib_empty",
                ret_type = "bool",
                arg_list = {
                    "scheme_tree",
                    "string"
                }
            },
            {
                scm_name = "bib-field",
                cpp_name = "bib_field",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree",
                    "string"
                }
            },
            {
                scm_name = "bib-add-period",
                cpp_name = "bib_add_period",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-locase-first",
                cpp_name = "bib_locase_first",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-upcase-first",
                cpp_name = "bib_upcase_first",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "bib-abbreviate",
                cpp_name = "bib_abbreviate",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree",
                    "scheme_tree",
                    "scheme_tree"
                }
            },
        }
    }
end
