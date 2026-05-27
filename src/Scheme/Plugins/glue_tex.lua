-------------------------------------------------------------------------------
--
-- MODULE      : glue_tex.lua
-- DESCRIPTION : Generating glue on src/Plugins/Tex
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
        initializer_name = "initialize_glue_tex",
        glues = {
            {
                scm_name = "parse-latex",
                cpp_name = "parse_latex",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "parse-latex-document",
                cpp_name = "parse_latex_document",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "latex->texmacs",
                cpp_name = "latex_to_tree",
                ret_type = "tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "cpp-latex-document->texmacs",
                cpp_name = "latex_document_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "latex-class-document->texmacs",
                cpp_name = "latex_class_document_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tracked-latex->texmacs",
                cpp_name = "tracked_latex_to_texmacs",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "conservative-texmacs->latex",
                cpp_name = "conservative_texmacs_to_latex",
                ret_type = "string",
                arg_list = {
                    "content",
                    "object"
                }
            },
            {
                scm_name = "tracked-texmacs->latex",
                cpp_name = "tracked_texmacs_to_latex",
                ret_type = "string",
                arg_list = {
                    "content",
                    "object"
                }
            },
            {
                scm_name = "conservative-latex->texmacs",
                cpp_name = "conservative_latex_to_texmacs",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "number-latex-pages",
                cpp_name = "number_latex_pages",
                ret_type = "int",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "latex-progress-start",
                cpp_name = "latex_progress_start",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "latex-progress-update",
                cpp_name = "latex_progress_update",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "latex-progress-end",
                cpp_name = "latex_progress_end",
                ret_type = "void"
            },
        }
    }
end

