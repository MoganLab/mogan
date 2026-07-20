-------------------------------------------------------------------------------
--
-- MODULE      : glue_qt.lua
-- DESCRIPTION : Generating glue on qt related routines
-- COPYRIGHT   : (C) 2026 JimZhouZZY
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        group_name = "glue_qt",
        binding_object = "",
        initializer_name = "initialize_glue_qt",
        standalone = true,
        includes = {
            "object_l1.hpp",
            "object_l2.hpp",
            "object_l3.hpp",
            "object_l5.hpp",
            "scheme.hpp",
            "glue_l5_extra.hpp",
        },
        glues = {
            {
                scm_name = "cpp-kill-tabpage",
                cpp_name = "cpp_kill_tabpage",
                ret_type = "void",
                arg_list = {
                    "url",
                    "url"
                }
            },
            {
                scm_name = "cpp-confirm-close",
                cpp_name = "cpp_confirm_close",
                ret_type = "string",
                arg_list = {
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "cpp-form-dialog",
                cpp_name = "cpp_form_dialog",
                ret_type = "tree",
                arg_list = {
                    "tree"
                }
            },
            {
                scm_name = "cpp-font-selector-dialog",
                cpp_name = "cpp_font_selector_dialog",
                ret_type = "tree",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "cpp-paragraph-format-dialog",
                cpp_name = "cpp_paragraph_format_dialog",
                ret_type = "tree",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "cpp-statistics-dialog",
                cpp_name = "cpp_statistics_dialog",
                ret_type = "void",
                arg_list = {
                    "string",
                    "tree"
                }
            },
            {
                scm_name = "cpp-rasterize-widget",
                cpp_name = "cpp_rasterize_widget",
                ret_type = "string",
                arg_list = {
                    "widget"
                }
            },
            {
                scm_name = "qt-clipboard-format",
                cpp_name = "qt_clipboard_format",
                ret_type = "string"
            },
            {
                scm_name = "qt-clipboard-text",
                cpp_name = "qt_clipboard_text",
                ret_type = "string"
            },
            {
                scm_name = "qt-clipboard-set-html",
                cpp_name = "qt_clipboard_set_html",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "open-pricing-url",
                cpp_name = "open_pricing_url",
                ret_type = "void"
            },
            {
                scm_name = "qt-chat-tab-set-state",
                cpp_name = "qt_chat_tab_set_state",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },

            {
                scm_name = "qt-chat-tab-restore-session",
                cpp_name = "qt_chat_tab_restore_session",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string",
                    "string",
                    "string",
                    "string",
                    "int",
                    "string",
                    "string"
                }
            },
            {
                scm_name = "qt-chat-tab-active-message-buffer-url",
                cpp_name = "qt_chat_tab_active_message_buffer_url",
                ret_type = "string",
                arg_list = {}
            },
            {
                scm_name = "qt-chat-notify-input-height",
                cpp_name = "qt_chat_notify_input_height",
                ret_type = "void",
                arg_list = {}
            },
 
            {
                scm_name = "qt-floating-search",
                cpp_name = "qt_floating_search",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "qt-floating-search-init",
                cpp_name = "qt_floating_search_init",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "qt-floating-search-set-match-info",
                cpp_name = "qt_floating_search_set_match_info",
                ret_type = "void",
                arg_list = {
                    "int",
                    "int"
                }
            },
            {
                scm_name = "qt-floating-search-set-callbacks",
                cpp_name = "qt_floating_search_set_callbacks",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "string"
                }
            }
        }
    }
end