-------------------------------------------------------------------------------
--
-- MODULE      : glue_html.lua
-- DESCRIPTION : Generating glue on HTML/MathML
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "",
        initializer_name = "initialize_glue_html",
        glues = {
            {
                scm_name = "parse-html",
                cpp_name = "parse_html",
                ret_type = "scheme_tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "clean-html",
                cpp_name = "clean_html",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "upgrade-mathml",
                cpp_name = "upgrade_mathml",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "retrieve-mathjax",
                cpp_name = "retrieve_mathjax",
                ret_type = "tree",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "html-progress-start",
                cpp_name = "html_progress_start",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "html-progress-update",
                cpp_name = "html_progress_update",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "html-progress-end",
                cpp_name = "html_progress_end",
                ret_type = "void"
            },
        }
    }
end

