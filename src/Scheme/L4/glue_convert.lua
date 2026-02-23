-------------------------------------------------------------------------------
--
-- MODULE      : glue_convert.lua
-- DESCRIPTION : Generating glue on src/Data/Convert
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
        initializer_name = "initialize_glue_convert",
        glues = {
            {
                scm_name = "parse-texmacs",
                cpp_name = "texmacs_document_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "serialize-texmacs",
                cpp_name = "tree_to_texmacs",
                ret_type = "string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "parse-texmacs-snippet",
                cpp_name = "texmacs_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "serialize-texmacs-snippet",
                cpp_name = "tree_to_texmacs",
                ret_type = "string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "parse-tmu",
                cpp_name = "tmu_document_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "serialize-tmu",
                cpp_name = "tree_to_tmu",
                ret_type = "string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "parse-tmu-snippet",
                cpp_name = "tmu_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "parse-tsu",
                cpp_name = "tsu_document_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "serialize-tsu",
                cpp_name = "tree_to_tsu",
                ret_type = "string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "parse-tsu-snippet",
                cpp_name = "tmu_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "texmacs->stm",
                cpp_name = "tree_to_scheme",
                ret_type = "string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "stm->texmacs",
                cpp_name = "scheme_document_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "stm-snippet->texmacs",
                cpp_name = "scheme_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tree->stree",
                cpp_name = "tree_to_scheme_tree",
                ret_type = "scheme_tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "stree->tree",
                cpp_name = "scheme_tree_to_tree",
                ret_type = "tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "cpp-texmacs->verbatim",
                cpp_name = "tree_to_verbatim",
                ret_type = "string",
                arg_list = {
                    "tree",
                    "bool",
                    "string"
                }
            },
            {
                scm_name = "cpp-verbatim-snippet->texmacs",
                cpp_name = "verbatim_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "bool",
                    "string"
                }
            },
            {
                scm_name = "cpp-verbatim->texmacs",
                cpp_name = "verbatim_document_to_tree",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "bool",
                    "string"
                }
            },
            {
                scm_name = "compute-keys-string",
                cpp_name = "compute_keys",
                ret_type = "array_string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "compute-keys-tree",
                cpp_name = "compute_keys",
                ret_type = "array_string",
                arg_list = {
                    "content",
                    "string"
                }
            },
            {
                scm_name = "compute-keys-url",
                cpp_name = "compute_keys",
                ret_type = "array_string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "compute-index-string",
                cpp_name = "compute_index",
                ret_type = "scheme_tree",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "compute-index-tree",
                cpp_name = "compute_index",
                ret_type = "scheme_tree",
                arg_list = {
                    "content",
                    "string"
                }
            },
            {
                scm_name = "compute-index-url",
                cpp_name = "compute_index",
                ret_type = "scheme_tree",
                arg_list = {
                    "url"
                }
            },
        }
    }
end
