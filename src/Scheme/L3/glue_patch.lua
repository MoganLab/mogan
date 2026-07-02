-------------------------------------------------------------------------------
--
-- MODULE      : glue_patch.lua
-- DESCRIPTION : Generating glue on patch
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
        initializer_name = "initialize_glue_patch",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "object_l3.hpp",
            "scheme.hpp",
            "patch.hpp",
            "tree.hpp",
            "glue_patch_extra.hpp",
        },
        glues = {
            {
                scm_name = "patch-pair",
                cpp_name = "patch",
                ret_type = "patch",
                arg_list = {
                    "modification",
                    "modification"
                }
            },
            {
                scm_name = "patch-compound",
                cpp_name = "patch",
                ret_type = "patch",
                arg_list = {
                    "array_patch"
                }
            },
            {
                scm_name = "patch-branch",
                cpp_name = "branch_patch",
                ret_type = "patch",
                arg_list = {
                    "array_patch"
                }
            },
            {
                scm_name = "patch-birth",
                cpp_name = "patch",
                ret_type = "patch",
                arg_list = {
                    "double",
                    "bool"
                }
            },
            {
                scm_name = "patch-author",
                cpp_name = "patch",
                ret_type = "patch",
                arg_list = {
                    "double",
                    "patch"
                }
            },
            {
                scm_name = "patch-pair?",
                cpp_name = "is_modification",
                ret_type = "bool",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-compound?",
                cpp_name = "is_compound",
                ret_type = "bool",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-branch?",
                cpp_name = "is_branch",
                ret_type = "bool",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-birth?",
                cpp_name = "is_birth",
                ret_type = "bool",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-author?",
                cpp_name = "is_author",
                ret_type = "bool",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-arity",
                cpp_name = "N",
                ret_type = "int",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-ref",
                cpp_name = "access",
                ret_type = "patch",
                arg_list = {
                    "patch",
                    "int"
                }
            },
            {
                scm_name = "patch-direct",
                cpp_name = "get_modification",
                ret_type = "modification",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-inverse",
                cpp_name = "get_inverse",
                ret_type = "modification",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-get-birth",
                cpp_name = "get_birth",
                ret_type = "bool",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-get-author",
                cpp_name = "get_author",
                ret_type = "double",
                arg_list = {
                    "patch"
                }
            },
            
            {
                scm_name = "patch-copy",
                cpp_name = "copy",
                ret_type = "patch",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-applicable?",
                cpp_name = "is_applicable",
                ret_type = "bool",
                arg_list = {
                    "patch",
                    "content"
                }
            },
            {
                scm_name = "patch-compactify",
                cpp_name = "compactify",
                ret_type = "patch",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-cursor-hint",
                cpp_name = "cursor_hint",
                ret_type = "path",
                arg_list = {
                    "patch",
                    "content"
                }
            },
            {
                scm_name = "patch-invert",
                cpp_name = "invert",
                ret_type = "patch",
                arg_list = {
                    "patch",
                    "content"
                }
            },
            {
                scm_name = "patch-commute?",
                cpp_name = "commute",
                ret_type = "bool",
                arg_list = {
                    "patch",
                    "patch"
                }
            },
            {
                scm_name = "patch-can-pull?",
                cpp_name = "can_pull",
                ret_type = "bool",
                arg_list = {
                    "patch",
                    "patch"
                }
            },
            {
                scm_name = "patch-pull",
                cpp_name = "pull",
                ret_type = "patch",
                arg_list = {
                    "patch",
                    "patch"
                }
            },
            {
                scm_name = "patch-co-pull",
                cpp_name = "co_pull",
                ret_type = "patch",
                arg_list = {
                    "patch",
                    "patch"
                }
            },
            {
                scm_name = "patch-remove-set-cursor",
                cpp_name = "remove_set_cursor",
                ret_type = "patch",
                arg_list = {
                    "patch"
                }
            },
            {
                scm_name = "patch-modifies?",
                cpp_name = "does_modify",
                ret_type = "bool",
                arg_list = {
                    "patch"
                }
            },
        }
    }
end
