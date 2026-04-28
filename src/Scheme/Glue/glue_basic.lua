-------------------------------------------------------------------------------
--
-- MODULE      : glue_basic.lua
-- DESCRIPTION : Building basic glue
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "",
        initializer_name = "initialize_glue_basic",
        glues = {
            {
                scm_name = "texmacs-version-release",
                cpp_name = "texmacs_version",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "version-before?",
                cpp_name = "version_inf",
                ret_type = "bool",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "get-original-path",
                cpp_name = "get_original_path",
                ret_type = "string"
            },
            {
                scm_name = "has-printing-cmd?",
                cpp_name = "has_printing_cmd",
                ret_type = "bool"
            },
            {
                scm_name = "get-retina-factor",
                cpp_name = "get_retina_factor",
                ret_type = "int"
            },
            {
                scm_name = "get-retina-zoom",
                cpp_name = "get_retina_zoom",
                ret_type = "int"
            },
            {
                scm_name = "get-retina-icons",
                cpp_name = "get_retina_icons",
                ret_type = "int"
            },
            {
                scm_name = "get-retina-scale",
                cpp_name = "get_retina_scale",
                ret_type = "double"
            },
            {
                scm_name = "set-retina-factor",
                cpp_name = "set_retina_factor",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "set-retina-zoom",
                cpp_name = "set_retina_zoom",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "set-retina-icons",
                cpp_name = "set_retina_icons",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "set-retina-scale",
                cpp_name = "set_retina_scale",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "tm-output",
                cpp_name = "tm_output",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tm-errput",
                cpp_name = "tm_errput",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "win32-display",
                cpp_name = "win32_display",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-error",
                cpp_name = "cpp_error",
                ret_type = "void"
            },
            {
                scm_name = "rescue-mode?",
                cpp_name = "in_rescue_mode",
                ret_type = "bool"
            },
            {
                scm_name = "scheme-dialect",
                cpp_name = "scheme_dialect",
                ret_type = "string"
            },
            {
                scm_name = "plugin-list",
                cpp_name = "plugin_list",
                ret_type = "scheme_tree"
            },
            {
                scm_name = "set-fast-environments",
                cpp_name = "set_fast_environments",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "texmacs-time",
                cpp_name = "texmacs_time",
                ret_type = "int"
            },
            {
                scm_name = "pretty-time",
                cpp_name = "pretty_time",
                ret_type = "string",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "texmacs-memory",
                cpp_name = "mem_used",
                ret_type = "int"
            },
            {
                scm_name = "bench-print-all",
                cpp_name = "bench_print_all",
                ret_type = "void"
            },
            {
                scm_name = "system-wait",
                cpp_name = "system_wait",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "math-symbol-group",
                cpp_name = "math_symbol_group",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "math-group-members",
                cpp_name = "math_group_members",
                ret_type = "array_string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "math-symbol-type",
                cpp_name = "math_symbol_type",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "object->command",
                cpp_name = "as_command",
                ret_type = "command",
                arg_list = {
                    "object"
                }
            },
            {
                scm_name = "exec-delayed",
                cpp_name = "exec_delayed",
                ret_type = "void",
                arg_list = {
                    "object"
                }
            },
            {
                scm_name = "exec-delayed-pause",
                cpp_name = "exec_delayed_pause",
                ret_type = "void",
                arg_list = {
                    "object"
                }
            },
            {
                scm_name = "protected-call",
                cpp_name = "protected_call",
                ret_type = "void",
                arg_list = {
                    "object"
                }
            },
            {
                scm_name = "notify-preferences-booted",
                cpp_name = "notify_preferences_booted",
                ret_type = "void"
            },
            {
                scm_name = "cpp-has-preference?",
                cpp_name = "has_user_preference",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cpp-get-preference",
                cpp_name = "get_user_preference",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "cpp-set-preference",
                cpp_name = "set_user_preference",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "cpp-reset-preference",
                cpp_name = "reset_user_preference",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "save-preferences",
                cpp_name = "save_user_preferences",
                ret_type = "void"
            },
            {
                scm_name = "get-default-printing-command",
                cpp_name = "get_printing_default",
                ret_type = "string"
            },
            {
                scm_name = "set-input-language",
                cpp_name = "set_input_language",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-input-language",
                cpp_name = "get_input_language",
                ret_type = "string"
            },
            {
                scm_name = "set-output-language",
                cpp_name = "gui_set_output_language",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-output-language",
                cpp_name = "get_output_language",
                ret_type = "string"
            },
            {
                scm_name = "translate",
                cpp_name = "translate",
                ret_type = "string",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "string-translate",
                cpp_name = "translate_as_is",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "translate-from-to",
                cpp_name = "translate",
                ret_type = "string",
                arg_list = {
                    "content",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "tree-translate",
                cpp_name = "tree_translate",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-translate-from-to",
                cpp_name = "tree_translate",
                ret_type = "tree",
                arg_list = {
                    "content",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "force-load-translations",
                cpp_name = "force_load_dictionary",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "color",
                cpp_name = "named_color",
                ret_type = "int",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-hex-color",
                cpp_name = "get_hex_color",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "named-color->xcolormap",
                cpp_name = "named_color_to_xcolormap",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "rgba->named-color",
                cpp_name = "named_rgb_color",
                ret_type = "string",
                arg_list = {
                    "array_int"
                }
            },
            {
                scm_name = "named-color->rgba",
                cpp_name = "get_named_rgb_color",
                ret_type = "array_int",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "new-author",
                cpp_name = "new_author",
                ret_type = "double"
            },
            {
                scm_name = "set-author",
                cpp_name = "set_author",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "get-author",
                cpp_name = "get_author",
                ret_type = "double"
            },
            {
                scm_name = "debug-set",
                cpp_name = "debug_set",
                ret_type = "void",
                arg_list = {
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "debug-get",
                cpp_name = "debug_get",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "debug-message",
                cpp_name = "debug_message",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "get-debug-messages",
                cpp_name = "get_debug_messages",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "int"
                }
            },
            {
                scm_name = "clear-debug-messages",
                cpp_name = "clear_debug_messages",
                ret_type = "void"
            },
            {
                scm_name = "cout-buffer",
                cpp_name = "cout_buffer",
                ret_type = "void"
            },
            {
                scm_name = "cout-unbuffer",
                cpp_name = "cout_unbuffer",
                ret_type = "string"
            },
            {
                scm_name = "mark-new",
                cpp_name = "new_marker",
                ret_type = "double"
            },
            {
                scm_name = "glyph-register",
                cpp_name = "register_glyph",
                ret_type = "void",
                arg_list = {
                    "string",
                    "array_array_array_double"
                }
            },
            {
                scm_name = "glyph-recognize",
                cpp_name = "recognize_glyph",
                ret_type = "string",
                arg_list = {
                    "array_array_array_double"
                }
            },
            {
                scm_name = "tmtm-eqnumber->nonumber",
                cpp_name = "eqnumber_to_nonumber",
                ret_type = "tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "busy-versioning?",
                cpp_name = "is_busy_versioning",
                ret_type = "bool"
            },
            {
                scm_name = "players-set-elapsed",
                cpp_name = "players_set_elapsed",
                ret_type = "void",
                arg_list = {
                    "tree",
                    "double"
                }
            },
            {
                scm_name = "players-set-speed",
                cpp_name = "players_set_speed",
                ret_type = "void",
                arg_list = {
                    "tree",
                    "double"
                }
            },
            {
                scm_name = "apply-effect",
                cpp_name = "apply_effect",
                ret_type = "void",
                arg_list = {
                    "content",
                    "array_url",
                    "url",
                    "int",
                    "int"
                }
            },
              -- routines for images and animations
            {
                scm_name = "image->psdoc",
                cpp_name = "image_to_psdoc",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "anim-control-times",
                cpp_name = "get_control_times",
                ret_type = "array_double",
                arg_list = {
                    "content"
                }
            },
            
            {
                scm_name = "tree-extents",
                cpp_name = "tree_extents",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree-active?",
                cpp_name = "tree_active",
                ret_type = "bool",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-load-inclusion",
                cpp_name = "load_inclusion",
                ret_type = "tree",
                arg_list = {
                    "url"
                }
            },
            
            
              -- links
            {
                scm_name = "tree->ids",
                cpp_name = "get_ids",
                ret_type = "list_string",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "id->trees",
                cpp_name = "get_trees",
                ret_type = "list_tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "vertex->links",
                cpp_name = "get_links",
                ret_type = "list_tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "tree->tree-pointer",
                cpp_name = "tree_pointer_new",
                ret_type = "observer",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "tree-pointer-detach",
                cpp_name = "tree_pointer_delete",
                ret_type = "void",
                arg_list = {
                    "observer"
                }
            },
            {
                scm_name = "tree-pointer->tree",
                cpp_name = "obtain_tree",
                ret_type = "tree",
                arg_list = {
                    "observer"
                }
            },
            {
                scm_name = "current-link-types",
                cpp_name = "all_link_types",
                ret_type = "list_string"
            },
            {
                scm_name = "get-locus-rendering",
                cpp_name = "get_locus_rendering",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-locus-rendering",
                cpp_name = "set_locus_rendering",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "declare-visited",
                cpp_name = "declare_visited",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "has-been-visited?",
                cpp_name = "has_been_visited",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            
            {
                scm_name = "graphics-set",
                cpp_name = "set_graphical_value",
                ret_type = "void",
                arg_list = {
                    "content",
                    "content"
                }
            },
            {
                scm_name = "graphics-has?",
                cpp_name = "has_graphical_value",
                ret_type = "bool",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "graphics-ref",
                cpp_name = "get_graphical_value",
                ret_type = "tree",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "graphics-needs-update?",
                cpp_name = "graphics_needs_update",
                ret_type = "bool"
            },
            {
                scm_name = "graphics-notify-update",
                cpp_name = "graphics_notify_update",
                ret_type = "void",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "url-resolve-pattern",
                cpp_name = "resolve_pattern",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            
              -- routines for strings
            {
                scm_name = "utf8->html",
                cpp_name = "utf8_to_html",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "escape-to-ascii",
                cpp_name = "cork_to_ascii",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "downgrade-math-letters",
                cpp_name = "downgrade_math_letters",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "string-convert",
                cpp_name = "convert",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "sourcecode->cork",
                cpp_name = "sourcecode_to_cork",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "cork->sourcecode",
                cpp_name = "cork_to_sourcecode",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "guess-wencoding",
                cpp_name = "guess_wencoding",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
              
            -- Program bracket matching
            {
                scm_name = "find-left-bracket",
                cpp_name = "find_left_bracket",
                ret_type = "path",
                arg_list = {
                    "path",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "find-right-bracket",
                cpp_name = "find_right_bracket",
                ret_type = "path",
                arg_list = {
                    "path",
                    "string",
                    "string"
                }
            },
            
            {
                scm_name = "tmstring-translit",
                cpp_name = "uni_translit",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-locase-first",
                cpp_name = "uni_locase_first",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-upcase-first",
                cpp_name = "uni_upcase_first",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-locase-all",
                cpp_name = "uni_locase_all",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-upcase-all",
                cpp_name = "uni_upcase_all",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-unaccent-all",
                cpp_name = "uni_unaccent_all",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-letter?",
                cpp_name = "uni_is_letter",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "tmstring-before?",
                cpp_name = "uni_before",
                ret_type = "bool",
                arg_list = {
                    "string",
                    "string"
                }
            },
            
              -- Spell checking
            {
                scm_name = "multi-spell-start",
                cpp_name = "spell_start",
                ret_type = "void"
            },
            {
                scm_name = "multi-spell-done",
                cpp_name = "spell_done",
                ret_type = "void"
            },
            {
                scm_name = "single-spell-start",
                cpp_name = "spell_start",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "single-spell-done",
                cpp_name = "spell_done",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "spell-check",
                cpp_name = "spell_check",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "spell-check?",
                cpp_name = "check_word",
                ret_type = "bool",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "spell-accept",
                cpp_name = "spell_accept",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "spell-var-accept",
                cpp_name = "spell_accept",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "spell-insert",
                cpp_name = "spell_insert",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            
              -- Packrat grammar and parsing tools
            {
                scm_name = "packrat-define",
                cpp_name = "packrat_define",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "tree"
                }
            },
            {
                scm_name = "packrat-property",
                cpp_name = "packrat_property",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "packrat-inherit",
                cpp_name = "packrat_inherit",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "packrat-parse",
                cpp_name = "packrat_parse",
                ret_type = "path",
                arg_list = {
                    "string",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "packrat-correct?",
                cpp_name = "packrat_correct",
                ret_type = "bool",
                arg_list = {
                    "string",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "packrat-context",
                cpp_name = "packrat_context",
                ret_type = "object",
                arg_list = {
                    "string",
                    "string",
                    "content",
                    "path"
                }
            },
            {
                scm_name = "syntax-read-preferences",
                cpp_name = "initialize_color_decodings",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "url-cache-invalidate",
                cpp_name = "web_cache_invalidate",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "picture-cache-reset",
                cpp_name = "picture_cache_reset",
                ret_type = "void"
            },
            {
                scm_name = "set-file-focus",
                cpp_name = "set_file_focus",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "get-file-focus",
                cpp_name = "get_file_focus",
                ret_type = "url"
            },
            
            
              -- TeXmacs servers and clients
            {
                scm_name = "server-start",
                cpp_name = "server_start",
                ret_type = "void"
            },
            {
                scm_name = "server-stop",
                cpp_name = "server_stop",
                ret_type = "void"
            },
            {
                scm_name = "server-read",
                cpp_name = "server_read",
                ret_type = "string",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "server-write",
                cpp_name = "server_write",
                ret_type = "void",
                arg_list = {
                    "int",
                    "string"
                }
            },
            {
                scm_name = "server-started?",
                cpp_name = "server_started",
                ret_type = "bool"
            },
            {
                scm_name = "client-start",
                cpp_name = "client_start",
                ret_type = "int",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "client-stop",
                cpp_name = "client_stop",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "client-read",
                cpp_name = "client_read",
                ret_type = "string",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "client-write",
                cpp_name = "client_write",
                ret_type = "void",
                arg_list = {
                    "int",
                    "string"
                }
            },
            {
                scm_name = "enter-secure-mode",
                cpp_name = "enter_secure_mode",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            
              -- connections to extern systems
            {
                scm_name = "connection-start",
                cpp_name = "connection_start",
                ret_type = "string",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "connection-status",
                cpp_name = "connection_status",
                ret_type = "int",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "connection-write-string",
                cpp_name = "connection_write",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "connection-write",
                cpp_name = "connection_write",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "connection-cmd",
                cpp_name = "connection_cmd",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "connection-eval",
                cpp_name = "connection_eval",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "string",
                    "content"
                }
            },
            {
                scm_name = "connection-interrupt",
                cpp_name = "connection_interrupt",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "connection-stop",
                cpp_name = "connection_stop",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            
            {
                scm_name = "show-balloon",
                cpp_name = "show_help_balloon",
                ret_type = "void",
                arg_list = {
                    "widget",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "get-style-menu",
                cpp_name = "get_style_menu",
                ret_type = "object"
            },
            {
                scm_name = "hidden-package?",
                cpp_name = "hidden_package",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-add-package-menu",
                cpp_name = "get_add_package_menu",
                ret_type = "object"
            },
            {
                scm_name = "get-remove-package-menu",
                cpp_name = "get_remove_package_menu",
                ret_type = "object"
            },
            {
                scm_name = "get-toggle-package-menu",
                cpp_name = "get_toggle_package_menu",
                ret_type = "object"
            },
            {
                scm_name = "refresh-now",
                cpp_name = "windows_refresh",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-screen-size",
                cpp_name = "get_screen_size",
                ret_type = "array_int"
            },
            
              -- buffers
            {
                scm_name = "buffer-list",
                cpp_name = "get_all_buffers",
                ret_type = "array_url"
            },
            {
                scm_name = "current-buffer-url",
                cpp_name = "get_current_buffer_safe",
                ret_type = "url"
            },
            {
                scm_name = "path-to-buffer",
                cpp_name = "path_to_buffer",
                ret_type = "url",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "buffer-new",
                cpp_name = "make_new_buffer",
                ret_type = "url"
            },
            {
                scm_name = "buffer-rename",
                cpp_name = "rename_buffer",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "buffer-set",
                cpp_name = "set_buffer_tree",
                ret_type = "void",
                arg_list = {
                    "url",
                    "content"
                }
            },
            {
                scm_name = "buffer-get",
                cpp_name = "get_buffer_tree",
                ret_type = "tree",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-set-body",
                cpp_name = "set_buffer_body",
                ret_type = "void",
                arg_list = {
                    "url",
                    "content"
                }
            },
            {
                scm_name = "buffer-get-body",
                cpp_name = "get_buffer_body",
                ret_type = "tree",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-set-master",
                cpp_name = "set_master_buffer",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "buffer-get-master",
                cpp_name = "get_master_buffer",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-set-title",
                cpp_name = "set_title_buffer",
                ret_type = "void",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "buffer-get-title",
                cpp_name = "get_title_buffer",
                ret_type = "string",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-last-save",
                cpp_name = "get_last_save_buffer",
                ret_type = "int",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-last-visited",
                cpp_name = "last_visited",
                ret_type = "double",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-modified?",
                cpp_name = "buffer_modified",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-modified-since-autosave?",
                cpp_name = "buffer_modified_since_autosave",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-pretend-modified",
                cpp_name = "pretend_buffer_modified",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-pretend-saved",
                cpp_name = "pretend_buffer_saved",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-pretend-autosaved",
                cpp_name = "pretend_buffer_autosaved",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-attach-notifier",
                cpp_name = "attach_buffer_notifier",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-has-name?",
                cpp_name = "buffer_has_name",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-aux?",
                cpp_name = "is_aux_buffer",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-embedded?",
                cpp_name = "is_embedded_buffer",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-import",
                cpp_name = "buffer_import",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "url",
                    "string"
                }
            },
            {
                scm_name = "buffer-load",
                cpp_name = "buffer_load",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "buffer-export",
                cpp_name = "buffer_export",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "url",
                    "string"
                }
            },
            {
                scm_name = "buffer-save",
                cpp_name = "buffer_save",
                ret_type = "bool",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "tree-import-loaded",
                cpp_name = "import_loaded_tree",
                ret_type = "tree",
                arg_list = {
                    "string",
                    "url",
                    "string"
                }
            },
            {
                scm_name = "tree-import",
                cpp_name = "import_tree",
                ret_type = "tree",
                arg_list = {
                    "url",
                    "string"
                }
            },
            {
                scm_name = "tree-inclusion",
                cpp_name = "load_inclusion",
                ret_type = "tree",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "tree-export",
                cpp_name = "export_tree",
                ret_type = "bool",
                arg_list = {
                    "tree",
                    "url",
                    "string"
                }
            },
            {
                scm_name = "tree-load-style",
                cpp_name = "load_style_tree",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "buffer-focus",
                cpp_name = "focus_on_buffer",
                ret_type = "bool",
                arg_list = {
                    "url",
                    "bool"
                }
            },
            {
                scm_name = "move-buffer-index",
                cpp_name = "move_buffer_via_index",
                ret_type = "void",
                arg_list = {
                    "int",
                    "int"
                }
            },

            {
                scm_name = "view-list",
                cpp_name = "get_all_views",
                ret_type = "array_url"
            },
            {
                scm_name = "tabpage-list",
                cpp_name = "get_all_views_unsorted",
                ret_type = "array_url",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "buffer->views",
                cpp_name = "buffer_to_views",
                ret_type = "array_url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "current-view-url",
                cpp_name = "get_current_view_safe",
                ret_type = "url"
            },
            {
                scm_name = "window->view",
                cpp_name = "window_to_view",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "view->buffer",
                cpp_name = "view_to_buffer",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "view->window-url",
                cpp_name = "view_to_window",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "view->window-of-tabpage-url",
                cpp_name = "view_to_window_of_tabpage",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "move-tabpage",
                cpp_name = "move_tabpage",
                ret_type = "void",
                arg_list = {
                    "int",
                    "int"
                }
            },
            {
                scm_name = "view-new",
                cpp_name = "get_new_view",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "kill-tabpage",
                cpp_name = "kill_tabpage",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "view-passive",
                cpp_name = "get_passive_view",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "view-recent",
                cpp_name = "get_recent_view",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "view-delete",
                cpp_name = "delete_view",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "window-set-view",
                cpp_name = "window_set_view",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url",
                    "bool"
                }
            },
            {
                -- 设定标签页（view）所在的窗口
                scm_name = "view-set-window",
                cpp_name = "view_set_window",
                ret_type = "bool", -- 是否释放了目标 view 所在的 window
                arg_list = {
                    "url", -- 目标 view 的 url
                    "url", -- 目标 window 的 url。传入 url-none 时打开新窗口。
                    "bool" -- 是否设置为焦点
                           --    输入 true 时，将目标窗口所附着的的 view 切换为目标 view，
                           --    且将目标窗口提升到顶部。
                           --    输入 false 时，不改变目标窗口所附着的 view。且不将目标窗
                           --    口提升到顶部。
                }
            },
            {
                -- 将标签页（view）挪动到新窗口
                scm_name = "view-set-new-window",
                cpp_name = "view_set_new_window",
                ret_type = "void",
                arg_list = {
                    "url", -- 目标 view 的 url
                }
            },
            {
                scm_name = "switch-to-buffer",
                cpp_name = "switch_to_buffer",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "make-cursor-visible",
                cpp_name = "make_cursor_visible",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                -- 更新最近的编辑区视图
                scm_name = "invalidate-most-recent-view",
                cpp_name = "invalidate_most_recent_view",
                ret_type = "void",
                arg_list = {
                }
            },
            {
                scm_name = "set-drd",
                cpp_name = "set_current_drd",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                -- 检查一个 view 是否是一个特定类型的视图
                -- 返回值：
                --    如果 url 满足类型要求，则返回 true，否则返回 false
                --    （本质上是在做字符串的格式匹配）
                scm_name = "is-view-type?",
                cpp_name = "is_tmfs_view_type",
                ret_type = "bool",
                arg_list = {
                    "url",    -- view 的 url
                    "string"  -- 代表类型的字符串
                }
            },
            
            {
                scm_name = "window-list",
                cpp_name = "windows_list",
                ret_type = "array_url"
            },
            {
                scm_name = "windows-number",
                cpp_name = "get_nr_windows",
                ret_type = "int"
            },
            {
                scm_name = "current-window",
                cpp_name = "get_current_window",
                ret_type = "url"
            },
            {
                scm_name = "buffer->windows",
                cpp_name = "buffer_to_windows",
                ret_type = "array_url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "window-to-buffer",
                cpp_name = "window_to_buffer",
                ret_type = "url",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "window-set-buffer",
                cpp_name = "window_set_buffer",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "window-focus",
                cpp_name = "window_focus",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "switch-to-window",
                cpp_name = "switch_to_window",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                -- 切换编辑焦点到当前 view 所在的窗口的父窗口
                scm_name = "switch-to-parent-window",
                cpp_name = "switch_to_parent_window",
                ret_type = "void",
            },
            
            {
                scm_name = "new-buffer",
                cpp_name = "create_buffer",
                ret_type = "url"
            },
            {
                scm_name = "open-buffer-in-window",
                cpp_name = "new_buffer_in_new_window",
                ret_type = "url",
                arg_list = {
                    "url",
                    "content",
                    "content"
                }
            },
            {
                scm_name = "open-window",
                cpp_name = "open_window",
                ret_type = "url"
            },
            {
                scm_name = "open-window-geometry",
                cpp_name = "open_window",
                ret_type = "url",
                arg_list = {
                    "content"
                }
            },
            {
                scm_name = "clone-window",
                cpp_name = "clone_window",
                ret_type = "void"
            },
            {
                scm_name = "cpp-buffer-close",
                cpp_name = "kill_buffer",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "kill-window",
                cpp_name = "kill_window",
                ret_type = "void",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "kill-current-window-and-buffer",
                cpp_name = "kill_current_window_and_buffer",
                ret_type = "void"
            },
            
            {
                scm_name = "project-attach",
                cpp_name = "project_attach",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "project-detach",
                cpp_name = "project_attach",
                ret_type = "void"
            },
            {
                scm_name = "project-attached?",
                cpp_name = "project_attached",
                ret_type = "bool"
            },
            {
                scm_name = "project-get",
                cpp_name = "project_get",
                ret_type = "url"
            },
            
              -- transitional alternative windows; to be replaced by better solution
            {
                scm_name = "alt-window-handle",
                cpp_name = "window_handle",
                ret_type = "int"
            },
            {
                scm_name = "alt-window-create-quit",
                cpp_name = "window_create",
                ret_type = "void",
                arg_list = {
                    "int",
                    "widget",
                    "string",
                    "command"
                }
            },
            {
                scm_name = "alt-window-create-plain",
                cpp_name = "window_create_plain",
                ret_type = "void",
                arg_list = {
                    "int",
                    "widget",
                    "string"
                }
            },
            {
                scm_name = "alt-window-create-popup",
                cpp_name = "window_create_popup",
                ret_type = "void",
                arg_list = {
                    "int",
                    "widget",
                    "string"
                }
            },
            {
                scm_name = "alt-window-create-tooltip",
                cpp_name = "window_create_tooltip",
                ret_type = "void",
                arg_list = {
                    "int",
                    "widget",
                    "string"
                }
            },
            {
                scm_name = "alt-window-delete",
                cpp_name = "window_delete",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "alt-window-show",
                cpp_name = "window_show",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "alt-window-hide",
                cpp_name = "window_hide",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "alt-window-get-size",
                cpp_name = "window_get_size",
                ret_type = "scheme_tree",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "alt-window-set-size",
                cpp_name = "window_set_size",
                ret_type = "void",
                arg_list = {
                    "int",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "alt-window-get-position",
                cpp_name = "window_get_position",
                ret_type = "scheme_tree",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "alt-window-set-position",
                cpp_name = "window_set_position",
                ret_type = "void",
                arg_list = {
                    "int",
                    "int",
                    "int"
                }
            },
            {
                scm_name = "alt-window-search",
                cpp_name = "window_search",
                ret_type = "path",
                arg_list = {
                    "url"
                }
            },
            {
                scm_name = "stem-user-agent",
                cpp_name = "stem_user_agent",
                ret_type = "string"
            },
            {
                scm_name = "stem-device-id",
                cpp_name = "stem_device_id",
                ret_type = "string"
            },
        }
    }
end