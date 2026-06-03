-------------------------------------------------------------------------------
--
-- MODULE      : glue_server.lua
-- DESCRIPTION : Building basic glue for the server
-- COPYRIGHT   : (C) 1999-2023  Joris van der Hoeven
--                   2023       jingkaimori
--
-- This software falls under the GNU general public license version 3 or later.
-- It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
-- in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.

function main()
    return {
        binding_object = "get_server()->",
        initializer_name = "initialize_glue_server",
        glues = {
            {
                scm_name = "insert-kbd-wildcard",
                cpp_name = "insert_kbd_wildcard",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string",
                    "bool",
                    "bool",
                    "bool"
                }
            },
            {
                scm_name = "set-variant-keys",
                cpp_name = "set_variant_keys",
                ret_type = "void",
                arg_list = {
                    "string",
                    "string"
                }
            },
            {
                scm_name = "kbd-pre-rewrite",
                cpp_name = "kbd_pre_rewrite",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "kbd-post-rewrite",
                cpp_name = "kbd_post_rewrite",
                ret_type = "string",
                arg_list = {
                    "string",
                    "bool"
                }
            },
            {
                scm_name = "kbd-system-rewrite",
                cpp_name = "kbd_system_rewrite",
                ret_type = "tree",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-font-rules",
                cpp_name = "set_font_rules",
                ret_type = "void",
                arg_list = {
                    "scheme_tree"
                }
            },
            
            {
                scm_name = "window-get-serial",
                cpp_name = "get_window_serial",
                ret_type = "int"
            },
            {
                scm_name = "window-set-property",
                cpp_name = "set_window_property",
                ret_type = "void",
                arg_list = {
                    "scheme_tree",
                    "scheme_tree"
                }
            },
            {
                scm_name = "window-get-property",
                cpp_name = "get_window_property",
                ret_type = "scheme_tree",
                arg_list = {
                    "scheme_tree"
                }
            },
            {
                scm_name = "show-header",
                cpp_name = "show_header",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "show-icon-bar",
                cpp_name = "show_icon_bar",
                ret_type = "void",
                arg_list = {
                    "int",
                    "bool"
                }
            },
            {
                scm_name = "show-side-tools",
                cpp_name = "show_side_tools",
                ret_type = "void",
                arg_list = {
                    "int",
                    "bool"
                }
            },
            {
                -- 设置辅助组件可见性
                scm_name = "show-auxiliary-widget",
                cpp_name = "show_auxiliary_widget",
                ret_type = "void",
                arg_list = {
                    "bool", -- 是否可见
                }
            },
            {
                --将焦点由辅助窗口切换为主窗口
                scm_name = "change-auxiliary-widget-focus",
                cpp_name = "change_auxiliary_widget_focus",
                ret_type = "void",
                arg_list = {}
            },
            {
                --获取辅助窗口的parent url
                scm_name = "get-auxiliary-widget-parent-url",
                cpp_name = "get_auxiliary_widget_parent_url",
                ret_type = "url",
                arg_list = {}
            },
            {
                -- 设置辅助组件标题
                scm_name = "set-auxiliary-widget-title",
                cpp_name = "set_auxiliary_widget_title",
                ret_type = "void",
                arg_list = {
                    "string" -- 标题
                }
            },
            {
                -- 查询辅助组件可见性
                scm_name = "auxiliary-widget-visible?",
                cpp_name = "auxiliary_widget_visible",
                ret_type = "bool",
                arg_list = {}
            },
            {
                -- 设定辅助组件内容
                scm_name = "set-auxiliary-widget",
                cpp_name = "auxiliary_widget",
                ret_type = "void",
                arg_list = {
                    "widget", -- 组件
                    "string", -- 标题
                }
            },
            {
                scm_name = "show-bottom-tools",
                cpp_name = "show_bottom_tools",
                ret_type = "void",
                arg_list = {
                    "int",
                    "bool"
                }
            },
            {
                scm_name = "show-footer",
                cpp_name = "show_footer",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "visible-header?",
                cpp_name = "visible_header",
                ret_type = "bool"
            },
            {
                scm_name = "visible-icon-bar?",
                cpp_name = "visible_icon_bar",
                ret_type = "bool",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "visible-side-tools?",
                cpp_name = "visible_side_tools",
                ret_type = "bool",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "visible-bottom-tools?",
                cpp_name = "visible_bottom_tools",
                ret_type = "bool",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "visible-footer?",
                cpp_name = "visible_footer",
                ret_type = "bool"
            },
            {
                scm_name = "show-chat-sidebar",
                cpp_name = "show_chat_sidebar",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            {
                scm_name = "visible-chat-sidebar?",
                cpp_name = "visible_chat_sidebar",
                ret_type = "bool"
            },
            {
                scm_name = "full-screen-mode",
                cpp_name = "full_screen_mode",
                ret_type = "void",
                arg_list = {
                    "bool",
                    "bool"
                }
            },
            {
                scm_name = "full-screen?",
                cpp_name = "in_full_screen_mode",
                ret_type = "bool"
            },
            {
                scm_name = "full-screen-edit?",
                cpp_name = "in_full_screen_edit_mode",
                ret_type = "bool"
            },
            {
                scm_name = "set-window-zoom-factor",
                cpp_name = "set_window_zoom_factor",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "get-window-zoom-factor",
                cpp_name = "get_window_zoom_factor",
                ret_type = "double"
            },
            
            {
                scm_name = "shell",
                cpp_name = "shell",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "dialogue-end",
                cpp_name = "dialogue_end",
                ret_type = "void"
            },
            {
                scm_name = "cpp-choose-file",
                cpp_name = "choose_file",
                ret_type = "void",
                arg_list = {
                    "object",
                    "string",
                    "string",
                    "string",
                    "url"
                }
            },
            {
                scm_name = "tm-interactive",
                cpp_name = "interactive",
                ret_type = "void",
                arg_list = {
                    "object",
                    "scheme_tree"
                }
            },
            
            {
                scm_name = "cpp-style-clear-cache",
                cpp_name = "style_clear_cache",
                ret_type = "void"
            },
            {
                scm_name = "set-script-status",
                cpp_name = "set_script_status",
                ret_type = "void",
                arg_list = {
                    "int"
                }
            },
            {
                scm_name = "set-printing-command",
                cpp_name = "set_printing_command",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-printer-paper-type",
                cpp_name = "set_printer_page_type",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "get-printer-paper-type",
                cpp_name = "get_printer_page_type",
                ret_type = "string"
            },
            {
                scm_name = "set-printer-dpi",
                cpp_name = "set_printer_dpi",
                ret_type = "void",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "set-default-zoom-factor",
                cpp_name = "set_default_zoom_factor",
                ret_type = "void",
                arg_list = {
                    "double"
                }
            },
            {
                scm_name = "get-default-zoom-factor",
                cpp_name = "get_default_zoom_factor",
                ret_type = "double"
            },
            {
                scm_name = "inclusions-gc",
                cpp_name = "inclusions_gc",
                ret_type = "void"
            },
            {
                scm_name = "update-all-path",
                cpp_name = "typeset_update",
                ret_type = "void",
                arg_list = {
                    "path"
                }
            },
            {
                scm_name = "update-all-buffers",
                cpp_name = "typeset_update_all",
                ret_type = "void"
            },
            {
                scm_name = "set-message",
                cpp_name = "set_message",
                ret_type = "void",
                arg_list = {
                    "content",
                    "content"
                }
            },
            {
                scm_name = "set-message-temp",
                cpp_name = "set_message",
                ret_type = "void",
                arg_list = {
                    "content",
                    "content",
                    "bool"
                }
            },
            {
                scm_name = "recall-message",
                cpp_name = "recall_message",
                ret_type = "void"
            },
            {
                scm_name = "yes?",
                cpp_name = "is_yes",
                ret_type = "bool",
                arg_list = {
                    "string"
                }
            },
            {
                scm_name = "quit-TeXmacs",
                cpp_name = "quit",
                ret_type = "void"
            },
            {
                scm_name = "restart-TeXmacs",
                cpp_name = "restart",
                ret_type = "void"
            },
            {
                scm_name = "login",
                cpp_name = "login",
                ret_type = "void"
            },
            {
                scm_name = "logged-in?",
                cpp_name = "is_logged_in",
                ret_type = "bool"
            }
        }
    }
end