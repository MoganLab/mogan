-------------------------------------------------------------------------------
--
-- MODULE      : glue_editor.lua
-- DESCRIPTION : Building basic glue for the editor
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "get_current_editor()->",
        initializer_name = "initialize_glue_editor",
        glues = {
            -- important paths and trees
            {
                scm_name = "root-tree",
                cpp_name = "the_root",
                ret_type = "tree"
            },
            {
                scm_name = "buffer-path",
                cpp_name = "the_buffer_path",
                ret_type = "path"
            },
            {
                scm_name = "buffer-tree",
                cpp_name = "the_buffer",
                ret_type = "tree"
            },
            {
                scm_name = "paragraph-tree",
                cpp_name = "the_line",
                ret_type = "tree"
            },
            {
                scm_name = "cursor-path",
                cpp_name = "the_path",
                ret_type = "path"
            },
            {
                scm_name = "cursor-path*",
                cpp_name = "the_shifted_path",
                ret_type = "path"
            },
            {
                scm_name = "selection-tree",
                cpp_name = "selection_get",
                ret_type = "tree"
            },
            
            -- low-level modification routines
            {
                scm_name = "path-exists?",
                cpp_name = "test_subtree",
                ret_type = "bool",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "cpp-path->tree",
                cpp_name = "the_subtree",
                ret_type = "tree",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "path-correct-old",
                cpp_name = "correct",
                ret_type = "void",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "path-insert-with",
                cpp_name = "insert_with",
                ret_type = "void",
                arg_list = {
                    "path",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "path-remove-with",
                cpp_name = "remove_with",
                ret_type = "void",
                arg_list = {
                    "path",
                    "string"
                }
            },
            
            {
                scm_name = "position-new-path",
                cpp_name = "position_new",
                ret_type = "observer",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "position-delete",
                cpp_name = "position_delete",
                ret_type = "void",
                arg_list = {
                    "observer"
                }
            },
            {
                scm_name = "position-set",
                cpp_name = "position_set",
                ret_type = "void",
                arg_list = {
                    "observer",
                    "path"
                }
            },
            {
                scm_name = "position-get",
                cpp_name = "position_get",
                ret_type = "path",
                arg_list = {
                    "observer"
                }
            },
            
            -- general modification routines
            {
                scm_name = "inside?",
                cpp_name = "inside",
                ret_type = "bool",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "cpp-insert",
                cpp_name = "insert_tree",
                ret_type = "void",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "cpp-insert-go-to",
                cpp_name = "var_insert_tree",
                ret_type = "void",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "insert-raw-go-to",
                cpp_name = "insert_tree",
                ret_type = "void",
                arg_list = {
                    "content",
                    "path"
                }
            },
            {
                scm_name = "insert-raw-return",
                cpp_name = "insert_return",
                ret_type = "void"
            },
            {
                scm_name = "remove-text",
                cpp_name = "remove_text",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "remove-structure",
                cpp_name = "remove_structure",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "remove-structure-upwards",
                cpp_name = "remove_structure_upwards",
                ret_type = "void"
            },
            
            {
                scm_name = "cpp-make",
                cpp_name = "make_compound",
                ret_type = "void",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "cpp-make-arity",
                cpp_name = "make_compound",
                ret_type = "void",
                arg_list = {
                    "tree_label",
                    "int"
                }
            },
            {
                scm_name = "activate",
                cpp_name = "activate",
                ret_type = "void"
            },
            {
                scm_name = "insert-argument",
                cpp_name = "insert_argument",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "remove-argument",
                cpp_name = "remove_argument",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "insert-argument-at",
                cpp_name = "insert_argument",
                ret_type = "void",
                arg_list = {
                    "path",
                    "bool"
                }
            },
            {
                scm_name = "remove-argument-at",
                cpp_name = "remove_argument",
                ret_type = "void",
                arg_list = {
                    "path",
                    "bool"
                }
            },
            {
                scm_name = "cpp-make-with",
                cpp_name = "make_with",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "make-mod-active",
                cpp_name = "make_mod_active",
                ret_type = "void",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "make-style-with",
                cpp_name = "make_style_with",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "cpp-make-hybrid",
                cpp_name = "make_hybrid",
                ret_type = "void"
            },
            {
                scm_name = "activate-latex",
                cpp_name = "activate_latex",
                ret_type = "void"
            },
            {
                scm_name = "activate-hybrid",
                cpp_name = "activate_hybrid",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "activate-symbol",
                cpp_name = "activate_symbol",
                ret_type = "void"
            },
            {
                scm_name = "make-return-before",
                cpp_name = "make_return_before",
                ret_type = "void"
            },
            {
                scm_name = "make-return-after",
                cpp_name = "make_return_after",
                ret_type = "bool"
            },
            {
                scm_name = "temp-proof-fix",
                cpp_name = "temp_proof_fix",
                ret_type = "void"
            },
            
            -- document-wide modifications
            {
                scm_name = "get-full-env",
                cpp_name = "get_full_env",
                ret_type = "tree"
            },
            {
                scm_name = "get-all-inits",
                cpp_name = "get_init_all",
                ret_type = "tree"
            },
            {
                scm_name = "init-default-one",
                cpp_name = "init_default",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "init-env",
                cpp_name = "init_env",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "init-env-tree",
                cpp_name = "init_env",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "init-style",
                cpp_name = "init_style",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-style-tree",
                cpp_name = "get_style",
                ret_type = "tree"
            },
            {
                scm_name = "set-style-tree",
                cpp_name = "change_style",
                ret_type = "void",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "get-env",
                cpp_name = "get_env_string",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-env-tree",
                cpp_name = "get_env_value",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-env-tree-at",
                cpp_name = "get_env_value",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "path"
                }
            },
            {
                scm_name = "get-init",
                cpp_name = "get_init_string",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-init-tree",
                cpp_name = "get_init_value",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "context-has?",
                cpp_name = "defined_at_cursor",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "style-has?",
                cpp_name = "defined_at_init",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "init-has?",
                cpp_name = "defined_in_init",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-page-count",
                cpp_name = "get_page_count",
                ret_type = "int"
            },
            {
                scm_name = "get-page-width",
                cpp_name = "get_page_width",
                ret_type = "int",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "get-pages-width",
                cpp_name = "get_pages_width",
                ret_type = "int",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "get-page-height",
                cpp_name = "get_page_height",
                ret_type = "int",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "get-total-width",
                cpp_name = "get_total_width",
                ret_type = "int",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "get-total-height",
                cpp_name = "get_total_height",
                ret_type = "int",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "get-reference",
                cpp_name = "get_ref",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-reference",
                cpp_name = "set_ref",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "reset-reference",
                cpp_name = "reset_ref",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "find-references",
                cpp_name = "find_refs",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "list-references",
                cpp_name = "list_refs",
                ret_type = "array_string"
            },
            {
                scm_name = "list-references*",
                cpp_name = "list_refs",
                ret_type = "array_string",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "get-auxiliary",
                cpp_name = "get_aux",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-auxiliary",
                cpp_name = "set_aux",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "reset-auxiliary",
                cpp_name = "reset_aux",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "list-auxiliaries",
                cpp_name = "list_auxs",
                ret_type = "array_string"
            },
            {
                scm_name = "list-auxiliaries*",
                cpp_name = "list_auxs",
                ret_type = "array_string",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "get-attachment",
                cpp_name = "get_att",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-attachment",
                cpp_name = "set_att",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "reset-attachment",
                cpp_name = "reset_att",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "list-attachments",
                cpp_name = "list_atts",
                ret_type = "array_string"
            },
            {
                scm_name = "list-attachments*",
                cpp_name = "list_atts",
                ret_type = "array_string",
                arg_list = {
                    "bool"
                }
            },
            
            -- modify text
            {
                scm_name = "make-htab",
                cpp_name = "make_htab",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "make-space",
                cpp_name = "make_space",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "make-var-space",
                cpp_name = "make_space",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "make-hspace",
                cpp_name = "make_hspace",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "make-var-hspace",
                cpp_name = "make_hspace",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "make-vspace-before",
                cpp_name = "make_vspace_before",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "make-var-vspace-before",
                cpp_name = "make_vspace_before",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "make-vspace-after",
                cpp_name = "make_vspace_after",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "make-var-vspace-after",
                cpp_name = "make_vspace_after",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "make-image",
                cpp_name = "make_image",
                ret_type = "void",
                arg_list = {
                    "string",
                    "bool",
                    "string",
                    "string",
                    "string",
                    "string"
                }
            },
            
            {
                scm_name = "length-decode",
                cpp_name = "as_length",
                ret_type = "int",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "length-add",
                cpp_name = "add_lengths",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "length-sub",
                cpp_name = "sub_lengths",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "length-max",
                cpp_name = "max_lengths",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "length-min",
                cpp_name = "min_lengths",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "length-mult",
                cpp_name = "multiply_length",
                ret_type = "string",
                arg_list = {
                    "double",
                    "string"
                }
            },
            {
                scm_name = "length?",
                cpp_name = "is_length",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "length-divide",
                cpp_name = "divide_lengths",
                ret_type = "double",
                arg_list = {
                    "string",
                    "string"
                }
            },
            
            -- modify mathematics
            {
                scm_name = "cpp-make-rigid",
                cpp_name = "make_rigid",
                ret_type = "void"
            },
            {
                scm_name = "cpp-make-lprime",
                cpp_name = "make_lprime",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-make-rprime",
                cpp_name = "make_rprime",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-make-below",
                cpp_name = "make_below",
                ret_type = "void"
            },
            {
                scm_name = "cpp-make-above",
                cpp_name = "make_above",
                ret_type = "void"
            },
            {
                scm_name = "cpp-make-script",
                cpp_name = "make_script",
                ret_type = "void",
                arg_list = {
                    "bool",
                    "bool"
                }
            },
            {
                scm_name = "cpp-make-fraction",
                cpp_name = "make_fraction",
                ret_type = "void"
            },
            {
                scm_name = "cpp-make-sqrt",
                cpp_name = "make_sqrt",
                ret_type = "void"
            },
            {
                scm_name = "cpp-make-wide",
                cpp_name = "make_wide",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-make-wide-under",
                cpp_name = "make_wide_under",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-make-var-sqrt",
                cpp_name = "make_var_sqrt",
                ret_type = "void"
            },
            {
                scm_name = "cpp-make-neg",
                cpp_name = "make_neg",
                ret_type = "void"
            },
            {
                scm_name = "cpp-make-tree",
                cpp_name = "make_tree",
                ret_type = "void"
            },
            
            -- modify tables
            {
                scm_name = "make-subtable",
                cpp_name = "make_subtable",
                ret_type = "void"
            },
            {
                scm_name = "table-deactivate",
                cpp_name = "table_deactivate",
                ret_type = "void"
            },
            {
                scm_name = "table-extract-format",
                cpp_name = "table_extract_format",
                ret_type = "void"
            },
            {
                scm_name = "table-insert-row",
                cpp_name = "table_insert_row",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "table-insert-column",
                cpp_name = "table_insert_column",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "table-remove-row",
                cpp_name = "table_remove_row",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "table-remove-column",
                cpp_name = "table_remove_column",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "table-nr-rows",
                cpp_name = "table_nr_rows",
                ret_type = "int"
            },
            {
                scm_name = "table-nr-columns",
                cpp_name = "table_nr_columns",
                ret_type = "int"
            },
            {
                scm_name = "table-get-extents",
                cpp_name = "table_get_extents",
                ret_type = "array_int"
            },
            {
                scm_name = "table-set-extents",
                cpp_name = "table_set_extents",
                ret_type = "void",
                arg_list = {
                    "int",
                    "int"
                }
            },
            {
                scm_name = "table-which-row",
                cpp_name = "table_which_row",
                ret_type = "int"
            },
            {
                scm_name = "table-which-column",
                cpp_name = "table_which_column",
                ret_type = "int"
            },
            {
                scm_name = "table-which-cells",
                cpp_name = "table_which_cells",
                ret_type = "array_int"
            },
            
            {
                scm_name = "table-cell-path",
                cpp_name = "table_search_cell",
                ret_type = "path",
                arg_list = {
                    "int",
                    "int"
                }
            },
            {
                scm_name = "table-go-to",
                cpp_name = "table_go_to",
                ret_type = "void",
                arg_list = {
                    "int",
                    "int"
                }
            },
            {
                scm_name = "table-set-format",
                cpp_name = "table_set_format",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "table-get-format-all",
                cpp_name = "table_get_format",
                ret_type = "tree"
            },
            {
                scm_name = "table-get-format",
                cpp_name = "table_get_format",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "table-del-format",
                cpp_name = "table_del_format",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "table-row-decoration",
                cpp_name = "table_row_decoration",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "table-column-decoration",
                cpp_name = "table_column_decoration",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "table-format-center",
                cpp_name = "table_format_center",
                ret_type = "void"
            },
            {
                scm_name = "table-correct-block-content",
                cpp_name = "table_correct_block_content",
                ret_type = "void"
            },
            {
                scm_name = "set-cell-mode",
                cpp_name = "set_cell_mode",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-cell-mode",
                cpp_name = "get_cell_mode",
                ret_type = "string"
            },
            {
                scm_name = "cell-set-format",
                cpp_name = "cell_set_format",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "cell-get-format",
                cpp_name = "cell_get_format",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cell-del-format",
                cpp_name = "cell_del_format",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "table-test",
                cpp_name = "table_test",
                ret_type = "void"
            },
            
            -- keyboard and mouse handling
            {
                scm_name = "key-press",
                cpp_name = "key_press",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "raw-emulate-keyboard",
                cpp_name = "emulate_keyboard",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "complete-try?",
                cpp_name = "complete_try",
                ret_type = "bool"
            },
            {
                scm_name = "source-complete-try",
                cpp_name = "source_complete_try",
                ret_type = "void"
            },
            {
                scm_name = "get-input-mode",
                cpp_name = "get_input_mode",
                ret_type = "int"
            },
            {
                scm_name = "key-press-search",
                cpp_name = "search_keypress",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "key-press-replace",
                cpp_name = "replace_keypress",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "key-press-spell",
                cpp_name = "spell_keypress",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "key-press-complete",
                cpp_name = "complete_keypress",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "key-press-source-complete",
                cpp_name = "source_complete_keypress",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "mouse-any",
                cpp_name = "mouse_any",
                ret_type = "void",
                arg_list = {
                    "string",
                    "int",
                    "int",
                    "int",
                    "double",
                    "array_double"
                }
            },
            {
                scm_name = "get-mouse-position",
                cpp_name = "get_mouse_position",
                ret_type = "array_int"
            },
            {
                scm_name = "set-mouse-pointer",
                cpp_name = "set_pointer",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
              scm_name = "set-cursor-style-origin",
              cpp_name = "set_cursor_style",
              ret_type = "void",
              arg_list = {
                "string"
              }
            },
            {
                scm_name = "set-predef-mouse-pointer",
                cpp_name = "set_pointer",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            
            -- moving the cursor
            {
                scm_name = "go-to-path",
                cpp_name = "go_to",
                ret_type = "void",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "go-left",
                cpp_name = "go_left",
                ret_type = "void"
            },
            {
                scm_name = "go-right",
                cpp_name = "go_right",
                ret_type = "void"
            },
            {
                scm_name = "go-up",
                cpp_name = "go_up",
                ret_type = "void"
            },
            {
                scm_name = "go-down",
                cpp_name = "go_down",
                ret_type = "void"
            },
            {
                scm_name = "go-start",
                cpp_name = "go_start",
                ret_type = "void"
            },
            {
                scm_name = "go-end",
                cpp_name = "go_end",
                ret_type = "void"
            },
            {
                scm_name = "go-start-of",
                cpp_name = "go_start_of",
                ret_type = "void",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "go-end-of",
                cpp_name = "go_end_of",
                ret_type = "void",
                arg_list = {
                    "tree_label"
                }
            },
            {
                scm_name = "go-start-with",
                cpp_name = "go_start_with",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "go-end-with",
                cpp_name = "go_end_with",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "go-start-line",
                cpp_name = "go_start_line",
                ret_type = "void"
            },
            {
                scm_name = "go-end-line",
                cpp_name = "go_end_line",
                ret_type = "void"
            },
            {
                scm_name = "go-page-up",
                cpp_name = "go_page_up",
                ret_type = "void"
            },
            {
                scm_name = "go-page-down",
                cpp_name = "go_page_down",
                ret_type = "void"
            },
            {
                scm_name = "go-start-paragraph",
                cpp_name = "go_start_paragraph",
                ret_type = "void"
            },
            {
                scm_name = "go-end-paragraph",
                cpp_name = "go_end_paragraph",
                ret_type = "void"
            },
            {
                scm_name = "label->path",
                cpp_name = "search_label",
                ret_type = "path",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "go-to-label",
                cpp_name = "go_to_label",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cursor-accessible?",
                cpp_name = "cursor_is_accessible",
                ret_type = "bool"
            },
            {
                scm_name = "cursor-show-if-hidden",
                cpp_name = "show_cursor_if_hidden",
                ret_type = "void"
            },
            
            -- selections
            {
                scm_name = "select-all",
                cpp_name = "select_all",
                ret_type = "void"
            },
            {
                scm_name = "select-all-correct",
                cpp_name = "select_all_correct",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "select-line",
                cpp_name = "select_line",
                ret_type = "void"
            },
            {
                scm_name = "select-from-cursor",
                cpp_name = "select_from_cursor",
                ret_type = "void"
            },
            {
                scm_name = "select-from-cursor-if-active",
                cpp_name = "select_from_cursor_if_active",
                ret_type = "void"
            },
            {
                scm_name = "select-from-keyboard",
                cpp_name = "select_from_keyboard",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "select-from-shift-keyboard",
                cpp_name = "select_from_shift_keyboard",
                ret_type = "void"
            },
            {
                scm_name = "select-enlarge",
                cpp_name = "select_enlarge",
                ret_type = "void"
            },
            {
                scm_name = "select-enlarge-environmental",
                cpp_name = "select_enlarge_environmental",
                ret_type = "void"
            },
            {
                scm_name = "selection-active-any?",
                cpp_name = "selection_active_any",
                ret_type = "bool"
            },
            {
                scm_name = "selection-active-normal?",
                cpp_name = "selection_active_normal",
                ret_type = "bool"
            },
            {
                scm_name = "selection-active-table?",
                cpp_name = "selection_active_table",
                ret_type = "bool"
            },
            {
                scm_name = "selection-active-small?",
                cpp_name = "selection_active_small",
                ret_type = "bool"
            },
            {
                scm_name = "selection-active-enlarging?",
                cpp_name = "selection_active_enlarging",
                ret_type = "bool"
            },
            {
                scm_name = "selection-set-start",
                cpp_name = "selection_set_start",
                ret_type = "void"
            },
            {
                scm_name = "selection-set-end",
                cpp_name = "selection_set_end",
                ret_type = "void"
            },
            {
                scm_name = "selection-get-start",
                cpp_name = "selection_get_start",
                ret_type = "path"
            },
            {
                scm_name = "selection-get-end",
                cpp_name = "selection_get_end",
                ret_type = "path"
            },
            {
                scm_name = "selection-get-start*",
                cpp_name = "selection_var_get_start",
                ret_type = "path"
            },
            {
                scm_name = "selection-get-end*",
                cpp_name = "selection_var_get_end",
                ret_type = "path"
            },
            {
                scm_name = "selection-path",
                cpp_name = "selection_get_path",
                ret_type = "path"
            },
            {
                scm_name = "selection-set",
                cpp_name = "selection_set_paths",
                ret_type = "void",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "selection-set-range-set",
                cpp_name = "selection_set_range_set",
                ret_type = "void",
                arg_list = {
                    "array_path"
                }
            },
            {
                scm_name = "clipboard-set",
                cpp_name = "selection_set",
                ret_type = "void",
                arg_list = {
                    "string",
                    "content"
                }
            },
            {
                scm_name = "clipboard-get",
                cpp_name = "selection_get",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-clipboard-copy",
                cpp_name = "selection_copy",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-clipboard-cut",
                cpp_name = "selection_cut",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "clipboard-cut-at",
                cpp_name = "cut",
                ret_type = "void",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "clipboard-cut-between",
                cpp_name = "cut",
                ret_type = "void",
                arg_list = {
                    "path",
                    "path"
                }
            },
            {
                scm_name = "cpp-clipboard-paste",
                cpp_name = "selection_paste",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "selection-move",
                cpp_name = "selection_move",
                ret_type = "void"
            },
            {
                scm_name = "clipboard-clear",
                cpp_name = "selection_clear",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "selection-cancel",
                cpp_name = "selection_cancel",
                ret_type = "void"
            },
            {
                scm_name = "clipboard-set-import",
                cpp_name = "selection_set_import",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "clipboard-set-export",
                cpp_name = "selection_set_export",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "clipboard-get-import",
                cpp_name = "selection_get_import",
                ret_type = "string"
            },
            {
                scm_name = "clipboard-get-export",
                cpp_name = "selection_get_export",
                ret_type = "string"
            },
            {
                scm_name = "set-manual-focus-path",
                cpp_name = "manual_focus_set",
                ret_type = "void",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "get-manual-focus-path",
                cpp_name = "manual_focus_get",
                ret_type = "path"
            },
            {
                scm_name = "get-focus-path",
                cpp_name = "focus_get",
                ret_type = "path"
            },
            {
                scm_name = "set-alt-selection",
                cpp_name = "set_alt_selection",
                ret_type = "void",
                arg_list = {
                    "string",
                    "array_path"
                }
            },
            {
                scm_name = "get-alt-selection",
                cpp_name = "get_alt_selection",
                ret_type = "array_path",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-alt-selection-index",
                cpp_name = "get_alt_selection_index",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "cancel-alt-selection",
                cpp_name = "cancel_alt_selection",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cancel-alt-selections",
                cpp_name = "cancel_alt_selections",
                ret_type = "void"
            },
            
            -- undo and redo
            {
                scm_name = "clear-undo-history",
                cpp_name = "clear_undo_history",
                ret_type = "void"
            },
            {
                scm_name = "commit-changes",
                cpp_name = "end_editing",
                ret_type = "void"
            },
            {
                scm_name = "start-slave",
                cpp_name = "start_slave",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "mark-start",
                cpp_name = "mark_start",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "mark-end",
                cpp_name = "mark_end",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "mark-cancel",
                cpp_name = "mark_cancel",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "remove-undo-mark",
                cpp_name = "remove_undo_mark",
                ret_type = "void"
            },
            {
                scm_name = "add-undo-mark",
                cpp_name = "add_undo_mark",
                ret_type = "void"
            },
            {
                scm_name = "unredoable-undo",
                cpp_name = "unredoable_undo",
                ret_type = "void"
            },
            {
                scm_name = "undo-possibilities",
                cpp_name = "undo_possibilities",
                ret_type = "int"
            },
            {
                scm_name = "undo",
                cpp_name = "undo",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "redo-possibilities",
                cpp_name = "redo_possibilities",
                ret_type = "int"
            },
            {
                scm_name = "redo",
                cpp_name = "redo",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "show-history",
                cpp_name = "show_history",
                ret_type = "void"
            },
            {
                scm_name = "archive-state",
                cpp_name = "archive_state",
                ret_type = "void"
            },
            {
                scm_name = "start-editing",
                cpp_name = "start_editing",
                ret_type = "void"
            },
            {
                scm_name = "end-editing",
                cpp_name = "end_editing",
                ret_type = "void"
            },
            {
                scm_name = "cancel-editing",
                cpp_name = "cancel_editing",
                ret_type = "void"
            },
            
            -- graphics
            {
                scm_name = "in-graphics?",
                cpp_name = "inside_graphics",
                ret_type = "bool"
            },
            {
                scm_name = "get-graphical-x",
                cpp_name = "get_x",
                ret_type = "double"
            },
            {
                scm_name = "get-graphical-y",
                cpp_name = "get_y",
                ret_type = "double"
            },
            {
                scm_name = "get-graphical-pixel",
                cpp_name = "get_pixel",
                ret_type = "double"
            },
            {
                scm_name = "get-graphical-object",
                cpp_name = "get_graphical_object",
                ret_type = "tree"
            },
            {
                scm_name = "set-graphical-object",
                cpp_name = "set_graphical_object",
                ret_type = "void",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "invalidate-graphical-object",
                cpp_name = "invalidate_graphical_object",
                ret_type = "void"
            },
            {
                scm_name = "graphical-select",
                cpp_name = "graphical_select",
                ret_type = "tree",
                arg_list = {
                    "double",
                    "double"
                }
            },
            {
                scm_name = "graphical-select-area",
                cpp_name = "graphical_select",
                ret_type = "tree",
                arg_list = {
                    "double",
                    "double",
                    "double",
                    "double"
                }
            },
            
            -- search, replace and spell checking
            {
                scm_name = "in-normal-mode?",
                cpp_name = "in_normal_mode",
                ret_type = "bool"
            },
            {
                scm_name = "in-search-mode?",
                cpp_name = "in_search_mode",
                ret_type = "bool"
            },
            {
                scm_name = "in-replace-mode?",
                cpp_name = "in_replace_mode",
                ret_type = "bool"
            },
            {
                scm_name = "in-spell-mode?",
                cpp_name = "in_spell_mode",
                ret_type = "bool"
            },
            {
                scm_name = "search-start",
                cpp_name = "search_start",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "search-button-next",
                cpp_name = "search_button_next",
                ret_type = "void"
            },
            {
                scm_name = "replace-start",
                cpp_name = "replace_start",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "spell-start",
                cpp_name = "spell_start",
                ret_type = "void"
            },
            {
                scm_name = "spell-replace",
                cpp_name = "spell_replace",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            
            -- sessions
            {
                scm_name = "session-complete-command",
                cpp_name = "session_complete_command",
                ret_type = "string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "custom-complete",
                cpp_name = "custom_complete",
                ret_type = "void",
                arg_list = {
                    "tree"
                }
            },
            
            -- miscellaneous routines
            {
                scm_name = "keyboard-focus-on",
                cpp_name = "keyboard_focus_on",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-user-active",
                cpp_name = "set_user_active",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "view-set-property",
                cpp_name = "set_property",
                ret_type = "void",
                arg_list = {
                    "scheme_tree",
                    "scheme_tree"
                }
            },
            {
                scm_name = "view-get-property",
                cpp_name = "get_property",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "get-window-width",
                cpp_name = "get_window_width",
                ret_type = "int"
            },
            {
                scm_name = "get-window-height",
                cpp_name = "get_window_height",
                ret_type = "int"
            },
            {
                scm_name = "get-window-x",
                cpp_name = "get_window_x",
                ret_type = "int"
            },
            {
                scm_name = "get-window-y",
                cpp_name = "get_window_y",
                ret_type = "int"
            },
            {
                scm_name = "get-canvas-x",
                cpp_name = "get_canvas_x",
                ret_type = "int"
            },
            {
                scm_name = "get-canvas-y",
                cpp_name = "get_canvas_y",
                ret_type = "int"
            },
            {
                scm_name = "get-scroll-x",
                cpp_name = "get_scroll_x",
                ret_type = "int"
            },
            {
                scm_name = "get-scroll-y",
                cpp_name = "get_scroll_y",
                ret_type = "int"
            },
            {
                scm_name = "clear-buffer",
                cpp_name = "clear_buffer",
                ret_type = "void"
            },
            {
                scm_name = "tex-buffer",
                cpp_name = "tex_buffer",
                ret_type = "void"
            },
            {
                scm_name = "clear-local-info",
                cpp_name = "clear_local_info",
                ret_type = "void"
            },
            {
                scm_name = "refresh-window",
                cpp_name = "invalidate_all",
                ret_type = "void"
            },
            {
                scm_name = "update-forced",
                cpp_name = "typeset_forced",
                ret_type = "void"
            },
            {
                scm_name = "update-path",
                cpp_name = "typeset_invalidate",
                ret_type = "void",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "update-current-buffer",
                cpp_name = "typeset_invalidate_all",
                ret_type = "void"
            },
            {
                scm_name = "update-players",
                cpp_name = "typeset_invalidate_players",
                ret_type = "void",
                arg_list = {
                    "path",
                    "bool"
                }
            },
            {
                scm_name = "generate-all-aux",
                cpp_name = "generate_aux",
                ret_type = "void"
            },
            {
                scm_name = "generate-aux",
                cpp_name = "generate_aux",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "notify-page-change",
                cpp_name = "notify_page_change",
                ret_type = "void"
            },
            {
                scm_name = "notify-change",
                cpp_name = "notify_change",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "notified-change?",
                cpp_name = "has_changed",
                ret_type = "bool",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "get-metadata",
                cpp_name = "get_metadata",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-nr-pages",
                cpp_name = "nr_pages",
                ret_type = "int"
            },
            {
                scm_name = "print-to-file",
                cpp_name = "print_to_file",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "print-pages-to-file",
                cpp_name = "print_to_file",
                ret_type = "void",
                arg_list = {
                    "url",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "print",
                cpp_name = "print_buffer",
                ret_type = "void"
            },
            {
                scm_name = "print-pages",
                cpp_name = "print_buffer",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "print-snippet",
                cpp_name = "print_snippet",
                ret_type = "array_int",
                arg_list = {
                    "url",
                    "content",
                    "bool"
                }
            },
            {
                scm_name = "graphics-file-to-clipboard",
                cpp_name = "graphics_file_to_clipboard",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "export-postscript",
                cpp_name = "export_ps",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "export-pages-postscript",
                cpp_name = "export_ps",
                ret_type = "void",
                arg_list = {
                    "url",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "footer-eval",
                cpp_name = "footer_eval",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "texmacs-exec",
                cpp_name = "texmacs_exec",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "texmacs-exec*",
                cpp_name = "var_texmacs_exec",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "texmacs-expand",
                cpp_name = "exec_texmacs",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "verbatim-expand",
                cpp_name = "exec_verbatim",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "latex-expand",
                cpp_name = "exec_latex",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "html-expand",
                cpp_name = "exec_html",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "animate-checkout",
                cpp_name = "checkout_animation",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "animate-commit",
                cpp_name = "commit_animation",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "idle-time",
                cpp_name = "idle_time",
                ret_type = "int"
            },
            {
                scm_name = "change-time",
                cpp_name = "change_time",
                ret_type = "int"
            },
            {
                scm_name = "menu-before-action",
                cpp_name = "before_menu_action",
                ret_type = "void"
            },
            {
                scm_name = "menu-after-action",
                cpp_name = "after_menu_action",
                ret_type = "void"
            },
            {
                scm_name = "update-menus",
                cpp_name = "update_menus",
                ret_type = "void"
            },
            
            {
                scm_name = "show-tree",
                cpp_name = "show_tree",
                ret_type = "void"
            },
            {
                scm_name = "show-env",
                cpp_name = "show_env",
                ret_type = "void"
            },
            {
                scm_name = "show-path",
                cpp_name = "show_path",
                ret_type = "void"
            },
            {
                scm_name = "show-cursor",
                cpp_name = "show_cursor",
                ret_type = "void"
            },
            {
                scm_name = "show-selection",
                cpp_name = "show_selection",
                ret_type = "void"
            },
            {
                scm_name = "show-meminfo",
                cpp_name = "show_meminfo",
                ret_type = "void"
            },
            {
                scm_name = "edit-special",
                cpp_name = "edit_special",
                ret_type = "void"
            },
        
        }
    }
end