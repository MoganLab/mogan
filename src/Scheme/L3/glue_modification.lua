-------------------------------------------------------------------------------
--
-- MODULE      : glue_modification.lua
-- DESCRIPTION : Generating glue on src/Kernel/Types/modification.hpp
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_modification",
        binding_object = "",
        initializer_name = "initialize_glue_modification",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "modification.hpp",
            "path.hpp",
            "tree.hpp",
            "glue_modification_extra.hpp",
        },
        glues = {
            {
                scm_name = "make-modification",
                cpp_name = "make_modification",
                ret_type = "modification",
                arg_list = {
                    "string",
                    "path",
                    "content"
                }
            },
            {
                scm_name = "modification-assign",
                cpp_name = "mod_assign",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "content"
                }
            },
            {
                scm_name = "modification-insert",
                cpp_name = "mod_insert",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "modification-remove",
                cpp_name = "mod_remove",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "modification-split",
                cpp_name = "mod_split",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "modification-join",
                cpp_name = "mod_join",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "int"
                }
            },
            {
                scm_name = "modification-assign-node",
                cpp_name = "mod_assign_node",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "tree_label"
                }
            },
            {
                scm_name = "modification-insert-node",
                cpp_name = "mod_insert_node",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "modification-remove-node",
                cpp_name = "mod_remove_node",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "int"
                }
            },
            {
                scm_name = "modification-set-cursor",
                cpp_name = "mod_set_cursor",
                ret_type = "modification",
                arg_list = {
                    "path",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "modification-kind",
                cpp_name = "get_type",
                ret_type = "string",
                arg_list = {
                    "modification"
                }
            },
            {
                scm_name = "modification-path",
                cpp_name = "get_path",
                ret_type = "path",
                arg_list = {
                    "modification"
                }
            },
            {
                scm_name = "modification-tree",
                cpp_name = "get_tree",
                ret_type = "tree",
                arg_list = {
                    "modification"
                }
            },
            {
                scm_name = "modification-root",
                cpp_name = "root",
                ret_type = "path",
                arg_list = {
                    "modification"
                }
            },
            {
                scm_name = "modification-index",
                cpp_name = "index",
                ret_type = "int",
                arg_list = {
                    "modification"
                }
            },
            {
                scm_name = "modification-argument",
                cpp_name = "argument",
                ret_type = "int",
                arg_list = {
                    "modification"
                }
            },
            {
                scm_name = "modification-label",
                cpp_name = "L",
                ret_type = "tree_label",
                arg_list = {
                    "modification"
                }
            },
            
            {
                scm_name = "modification-copy",
                cpp_name = "copy",
                ret_type = "modification",
                arg_list = {
                    "modification"
                }
            },
            {
                scm_name = "modification-applicable?",
                cpp_name = "is_applicable",
                ret_type = "bool",
                arg_list = {
                    "content",
                    "modification"
                }
            },
            {
                scm_name = "modification-apply",
                cpp_name = "var_clean_apply",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "modification"
                }
            },
            {
                scm_name = "modification-inplace-apply",
                cpp_name = "var_apply",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "modification"
                }
            },
            {
                scm_name = "modification-invert",
                cpp_name = "invert",
                ret_type = "modification",
                arg_list = {
                    "modification",
                    "content"
                }
            },
            {
                scm_name = "modification-commute?",
                cpp_name = "commute",
                ret_type = "bool",
                arg_list = {
                    "modification",
                    "modification"
                }
            },
            {
                scm_name = "modification-can-pull?",
                cpp_name = "can_pull",
                ret_type = "bool",
                arg_list = {
                    "modification",
                    "modification"
                }
            },
            {
                scm_name = "modification-pull",
                cpp_name = "pull",
                ret_type = "modification",
                arg_list = {
                    "modification",
                    "modification"
                }
            },
            {
                scm_name = "modification-co-pull",
                cpp_name = "co_pull",
                ret_type = "modification",
                arg_list = {
                    "modification",
                    "modification"
                }
            },
        }
    }
end
