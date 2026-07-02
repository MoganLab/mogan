-------------------------------------------------------------------------------
--
-- MODULE      : glue_moebius.lua
-- DESCRIPTION : Generating glue on routines in moebius
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023-2024  Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "",
        initializer_name = "initialize_glue_moebius",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "object_l3.hpp",
            "scheme.hpp",
            "converter.hpp",
            "cork.hpp",
            "observers.hpp",
            "path.hpp",
            "tree.hpp",
            "tree_cursor.hpp",
            "tree_helper.hpp",
            "tree_modify.hpp",
            "tree_observer.hpp",
            "tree_patch.hpp",
            "tree_traverse.hpp",
            "glue_moebius_extra.hpp",
        },
        glues = {
            {
                scm_name = "string-quote",
                cpp_name = "scm_quote",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-unquote",
                cpp_name = "scm_unquote",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "utf8->cork",
                cpp_name = "utf8_to_cork",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cork->utf8",
                cpp_name = "cork_to_utf8",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "strict-cork->utf8",
                cpp_name = "strict_cork_to_utf8",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
              -- routines for strings in the TeXmacs encoding
            {
                scm_name = "string->tmstring",
                cpp_name = "tm_encode",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring->string",
                cpp_name = "tm_decode",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-length",
                cpp_name = "tm_string_length",
                ret_type = "int",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-ref",
                cpp_name = "tm_forward_access",
                ret_type = "string",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "tmstring-reverse-ref",
                cpp_name = "tm_backward_access",
                ret_type = "string",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "tmstring->list",
                cpp_name = "tm_tokenize",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-next",
                cpp_name = "tm_char_next",
                ret_type = "int",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "string-previous",
                cpp_name = "tm_char_previous",
                ret_type = "int",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "tmstring-split",
                cpp_name = "tm_string_split",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "list->tmstring",
                cpp_name = "tm_recompose",
                ret_type = "string",
                arg_list = {
                    "array_string"
                }
            },
            {
                scm_name = "path-strip",
                cpp_name = "strip",
                ret_type = "path",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-inf?",
                cpp_name = "path_inf",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-inf-eq?",
                cpp_name = "path_inf_eq",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-less?",
                cpp_name = "path_less",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-less-eq?",
                cpp_name = "path_less_eq",
                ret_type = "bool",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "path-start",
                cpp_name = "start",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-end",
                cpp_name = "end",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            -- routines for tree observers
            {
                scm_name = "tree-ip",
                cpp_name = "obtain_ip",
                ret_type = "path",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-assign",
                cpp_name = "tree_assign",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "content"
                }
            },
            {
                scm_name = "tree-var-insert",
                cpp_name = "tree_insert",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "tree-remove",
                cpp_name = "tree_remove",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "tree-split",
                cpp_name = "tree_split",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "tree-join",
                cpp_name = "tree_join",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int"
                }
            },
            {
                scm_name = "tree-assign-node",
                cpp_name = "tree_assign_node",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "tree_label"
                }
            },
            {
                scm_name = "tree-insert-node",
                cpp_name = "tree_insert_node",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "tree-remove-node",
                cpp_name = "tree_remove_node",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int"
                }
            },
            -- routines for tree modification
            {
                scm_name = "tree-simplify",
                cpp_name = "simplify_correct",
                ret_type = "tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "cpp-tree-correct-node",
                cpp_name = "correct_node",
                ret_type = "void",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "cpp-tree-correct-downwards",
                cpp_name = "correct_downwards",
                ret_type = "void",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "cpp-tree-correct-upwards",
                cpp_name = "correct_upwards",
                ret_type = "void",
                arg_list = {
                    "tree"
                }
            },
            -- routines for tree traversal
            {
                scm_name = "tree-label-macro?",
                cpp_name = "is_macro",
                ret_type = "bool",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "tree-label-parameter?",
                cpp_name = "is_parameter",
                ret_type = "bool",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "tree-label-type",
                cpp_name = "get_tag_type",
                ret_type = "string",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "tree-primitives",
                cpp_name = "get_all_primitives",
                ret_type = "array_string",
                arg_list = {
                }
            },
            {
                scm_name = "tree-minimal-arity",
                cpp_name = "minimal_arity",
                ret_type = "int",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-maximal-arity",
                cpp_name = "maximal_arity",
                ret_type = "int",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-possible-arity?",
                cpp_name = "correct_arity",
                ret_type = "bool",
                arg_list = {
                    "tree",
                    "int"
                }
            },
            {
                scm_name = "tree-insert_point",
                cpp_name = "insert_point",
                ret_type = "int",
                arg_list = {
                    "tree",
                    "int"
                }
            },
            {
                scm_name = "tree-is-dynamic?",
                cpp_name = "is_dynamic",
                ret_type = "bool",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-accessible-child?",
                cpp_name = "is_accessible_child",
                ret_type = "bool",
                arg_list = {
                    "tree",
                    "int"
                }
            },
            {
                scm_name = "tree-accessible-children",
                cpp_name = "accessible_children",
                ret_type = "array_tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-all-accessible?",
                cpp_name = "all_accessible",
                ret_type = "bool",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-none-accessible?",
                cpp_name = "none_accessible",
                ret_type = "bool",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-name",
                cpp_name = "get_name",
                ret_type = "string",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-long-name",
                cpp_name = "get_long_name",
                ret_type = "string",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-child-name",
                cpp_name = "get_child_name",
                ret_type = "string",
                arg_list = {
                    "content",
                    "int"
                }
            },
            {
                scm_name = "tree-child-long-name",
                cpp_name = "get_child_long_name",
                ret_type = "string",
                arg_list = {
                    "content",
                    "int"
                }
            },
            {
                scm_name = "tree-child-type",
                cpp_name = "get_child_type",
                ret_type = "string",
                arg_list = {
                    "content",
                    "int"
                }
            },
            {
                scm_name = "tree-child-env*",
                cpp_name = "get_env_child",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "tree-child-env",
                cpp_name = "get_env_child",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "int",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "tree-descendant-env*",
                cpp_name = "get_env_descendant",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "path",
                    "content"
                }
            },
            {
                scm_name = "tree-descendant-env",
                cpp_name = "get_env_descendant",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "path",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "tag-minimal-arity",
                cpp_name = "minimal_arity",
                ret_type = "int",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "tag-maximal-arity",
                cpp_name = "maximal_arity",
                ret_type = "int",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "tag-possible-arity?",
                cpp_name = "correct_arity",
                ret_type = "bool",
                arg_list = {
                    "tree_label",
                    "int"
                }
            },
            {
                scm_name = "tree-search-sections",
                cpp_name = "search_sections",
                ret_type = "array_tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "path-next",
                cpp_name = "next_valid",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-previous",
                cpp_name = "previous_valid",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-next-word",
                cpp_name = "next_word",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-previous-word",
                cpp_name = "previous_word",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-next-node",
                cpp_name = "next_node",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-previous-node",
                cpp_name = "previous_node",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-next-tag",
                cpp_name = "next_tag",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path",
                    "scheme_tree"
                }
            },
            {
                scm_name = "path-previous-tag",
                cpp_name = "previous_tag",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path",
                    "scheme_tree"
                }
            },
            {
                scm_name = "path-next-tag-same-argument",
                cpp_name = "next_tag_same_argument",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path",
                    "scheme_tree"
                }
            },
            {
                scm_name = "path-previous-tag-same-argument",
                cpp_name = "previous_tag_same_argument",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path",
                    "scheme_tree"
                }
            },
            {
                scm_name = "path-next-argument",
                cpp_name = "next_argument",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-previous-argument",
                cpp_name = "previous_argument",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "path-previous-section",
                cpp_name = "previous_section",
                ret_type = "path",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "herk-tree->utf8-tree",
                cpp_name = "tree_herk_to_utf8",
                ret_type = "tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "utf8-tree->herk-tree",
                cpp_name = "tree_utf8_to_herk",
                ret_type = "tree",
                arg_list = {
                    "tree"
                }
            },
            -- routines for patch application
            {
                scm_name = "patch-apply",
                cpp_name = "var_clean_apply",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "patch"
                }
            },
            {
                scm_name = "patch-inplace-apply",
                cpp_name = "var_apply",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "patch"
                }
            },
        }
    }
end

