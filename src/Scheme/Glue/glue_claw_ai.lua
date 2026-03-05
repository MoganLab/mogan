-------------------------------------------------------------------------------
--
-- MODULE      : glue_claw_ai.lua
-- DESCRIPTION : Building glue for Claw AI integration
-- COPYRIGHT   : (C) 2026  Gatsby
--
-- This software falls under the GNU general public license version 3 or later.
-------------------------------------------------------------------------------

function main()
    return {
        binding_object = "get_server()->",
        initializer_name = "initialize_glue_claw_ai",
        glues = {
            -- 显示/隐藏 Claw AI 面板
            {
                scm_name = "show-claw-ai-panel",
                cpp_name = "show_claw_ai_panel",
                ret_type = "void",
                arg_list = {
                    "bool"
                }
            },
            -- 发送消息到 Claw AI（同步）
            {
                scm_name = "claw-ai-send",
                cpp_name = "claw_ai_send",
                ret_type = "string",
                arg_list = {
                    "string"
                }
            },
            -- 获取面板可见状态
            {
                scm_name = "claw-ai-panel-visible?",
                cpp_name = "claw_ai_panel_visible",
                ret_type = "bool"
            }
        }
    }
end
