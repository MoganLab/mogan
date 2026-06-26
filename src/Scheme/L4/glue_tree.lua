-------------------------------------------------------------------------------
--
-- MODULE      : glue_string.lua
-- DESCRIPTION : Generating glue on src/Kernel/Types/tree.hpp and src/Data/Tree
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--                   2023       Darcy Shen
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_tree",
        binding_object = "",
        initializer_name = "initialize_glue_tree",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "scheme.hpp",
            "analyze.hpp",
            "observers.hpp",
            "path.hpp",
            "s7_tm.hpp",
            "tree.hpp",
            "tree_analyze.hpp",
            "tree_correct.hpp",
            "tree_helper.hpp",
            "tree_math_stats.hpp",
            "tree_modify.hpp",
            "tree_observer.hpp",
            "tree_search.hpp",
            "tree_traverse.hpp",
            "glue_tree_extra.hpp",
        },
        glues = {
            {
                scm_name = "tree->string",
                cpp_name = "coerce_tree_string",
                ret_type = "string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "string->tree",
                cpp_name = "coerce_string_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tm->tree",
                cpp_name = "tree",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-atomic?",
                cpp_name = "is_atomic",
                ret_type = "bool",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-compound?",
                cpp_name = "is_compound",
                ret_type = "bool",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-label",
                cpp_name = "L",
                ret_type = "tree_label",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-children",
                cpp_name = "A",
                ret_type = "array_tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-arity",
                cpp_name = "N",
                ret_type = "int",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-child-ref",
                cpp_name = "tree_ref",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int"
                }
            },
            {
                scm_name = "tree-child-set!",
                cpp_name = "tree_set",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "tree-child-insert",
                cpp_name = "tree_child_insert",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "int",
                    "content"
                }
            },
            {
                scm_name = "tree-ip",
                cpp_name = "obtain_ip",
                ret_type = "path",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-eq?",
                cpp_name = "strong_equal",
                ret_type = "bool",
                arg_list = {
                    "tree",
                    "tree"
                }
            },
            {
                scm_name = "subtree",
                cpp_name = "subtree",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "path"
                }
            },
            {
                scm_name = "tree-range",
                cpp_name = "tree_range",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "tree-copy",
                cpp_name = "copy",
                ret_type = "tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-append",
                cpp_name = "tree_append",
                ret_type = "tree",
                arg_list = {
                    "tree",
                    "tree"
                }
            },
            {
                scm_name = "tree-right-index",
                cpp_name = "right_index",
                ret_type = "int",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-label-extension?",
                cpp_name = "is_extension",
                ret_type = "bool",
                arg_list = {
                    "tree_label"
                }
            },
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
                scm_name = "tree-multi-paragraph?",
                cpp_name = "is_multi_paragraph",
                ret_type = "bool",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-simplify",
                cpp_name = "simplify_correct",
                ret_type = "tree",
                arg_list = {
                    "tree"
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
                scm_name = "tree-as-string",
                cpp_name = "tree_as_string",
                ret_type = "string",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-empty?",
                cpp_name = "is_empty",
                ret_type = "bool",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-multi-line?",
                cpp_name = "is_multi_line",
                ret_type = "bool",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-is-buffer?",
                cpp_name = "admits_edit_observer",
                ret_type = "bool",
                arg_list = {
                    "tree"
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
                scm_name = "tree-search-tree",
                cpp_name = "search",
                ret_type = "array_path",
                arg_list = {
                    "content",
                    "content",
                    "path",
                    "int"
                }
            },
            {
                scm_name = "tree-search-tree-at",
                cpp_name = "search",
                ret_type = "array_path",
                arg_list = {
                    "content",
                    "content",
                    "path",
                    "path",
                    "int"
                }
            },
            {
                scm_name = "tree-spell",
                cpp_name = "spell",
                ret_type = "array_path",
                arg_list = {
                    "string",
                    "content",
                    "path",
                    "int"
                }
            },
            {
                scm_name = "tree-spell-at",
                cpp_name = "spell",
                ret_type = "array_path",
                arg_list = {
                    "string",
                    "content",
                    "path",
                    "path",
                    "int"
                }
            },
            {
                scm_name = "tree-spell-selection",
                cpp_name = "spell",
                ret_type = "array_path",
                arg_list = {
                    "string",
                    "content",
                    "path",
                    "path",
                    "path",
                    "int"
                }
            },
            {
                scm_name = "previous-search-hit",
                cpp_name = "previous_search_hit",
                ret_type = "array_path",
                arg_list = {
                    "array_path",
                    "path",
                    "bool"
                }
            },
            {
                scm_name = "next-search-hit",
                cpp_name = "next_search_hit",
                ret_type = "array_path",
                arg_list = {
                    "array_path",
                    "path",
                    "bool"
                }
            },
            {
                scm_name = "navigate-search-hit",
                cpp_name = "navigate_search_hit",
                ret_type = "array_path",
                arg_list = {
                    "path",
                    "bool",
                    "bool",
                    "bool"
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
            {
                scm_name = "concat-tokenize-math",
                cpp_name = "concat_tokenize",
                ret_type = "array_tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "concat-decompose",
                cpp_name = "concat_decompose",
                ret_type = "array_tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "concat-recompose",
                cpp_name = "concat_recompose",
                ret_type = "tree",
                arg_list = {
                    "array_tree"
                }
            },
            {
                scm_name = "with-like?",
                cpp_name = "is_with_like",
                ret_type = "bool",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "with-same-type?",
                cpp_name = "with_same_type",
                ret_type = "bool",
                arg_list = {
                    "content",
                    "content"
                }
            },
            {
                scm_name = "with-similar-type?",
                cpp_name = "with_similar_type",
                ret_type = "bool",
                arg_list = {
                    "content",
                    "content"
                }
            },
            {
                scm_name = "with-correct",
                cpp_name = "with_correct",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "with-correct-superfluous",
                cpp_name = "superfluous_with_correct",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "invisible-correct-superfluous",
                cpp_name = "superfluous_invisible_correct",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "invisible-correct-missing",
                cpp_name = "missing_invisible_correct",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "int"
                }
            },
            {
                scm_name = "automatic-correct",
                cpp_name = "automatic_correct",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "string"
                }
            },
            {
                scm_name = "manual-correct",
                cpp_name = "manual_correct",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-upgrade-brackets",
                cpp_name = "upgrade_brackets",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "string"
                }
            },
            {
                scm_name = "tree-upgrade-big",
                cpp_name = "upgrade_big",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-downgrade-brackets",
                cpp_name = "downgrade_brackets",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "bool",
                    "bool"
                }
            },
            {
                scm_name = "tree-downgrade-big",
                cpp_name = "downgrade_big",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "math-status-print",
                cpp_name = "math_status_print",
                ret_type = "void"
            },
            {
                scm_name = "math-status-reset",
                cpp_name = "math_status_reset",
                ret_type = "void"
            },
            {
                scm_name = "math-stats-compile",
                cpp_name = "compile_stats",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content",
                    "string"
                }
            },
            {
                scm_name = "math-stats-occurrences",
                cpp_name = "number_occurrences",
                ret_type = "int",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "math-stats-number-in-role",
                cpp_name = "number_in_role",
                ret_type = "int",
                arg_list = {
                    "string",
                    "content"
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
        }
    }
end
