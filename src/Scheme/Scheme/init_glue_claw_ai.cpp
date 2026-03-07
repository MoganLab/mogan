/******************************************************************************
 * MODULE     : init_glue_claw_ai.cpp
 * DESCRIPTION: Glue code for Claw AI Scheme/C++ binding
 * COPYRIGHT  : (C) 2026 Liii Network
 ******************************************************************************/

#include "scheme.hpp"
#include "object.hpp"
#include "QTMClawAIWidget.hpp"

#include <QMainWindow>

// 全局 Claw AI Widget 引用
static QTMClawAIWidget* g_claw_ai_widget = nullptr;

/******************************************************************************
 * Helper functions
 ******************************************************************************/

/**
 * @brief 创建或获取 Claw AI Widget
 */
static QTMClawAIWidget*
get_claw_ai_widget () {
    if (!g_claw_ai_widget) {
        g_claw_ai_widget = new QTMClawAIWidget (nullptr);
        g_claw_ai_widget->setAllowedAreas (Qt::RightDockWidgetArea);
        g_claw_ai_widget->setFloating (false);
        g_claw_ai_widget->hide ();
    }
    return g_claw_ai_widget;
}

/******************************************************************************
 * Scheme 可调用的 C++ 函数 (S7 风格)
 ******************************************************************************/

/**
 * @brief 显示 Claw AI 窗口
 */
tmscm
tm_claw_ai_widget_show (tmscm args) {
    QTMClawAIWidget* widget = get_claw_ai_widget ();
    if (widget) {
        widget->show ();
        widget->raise ();
    }
    return TMSCM_UNSPECIFIED;
}

/**
 * @brief 隐藏 Claw AI 窗口
 */
tmscm
tm_claw_ai_widget_hide (tmscm args) {
    if (g_claw_ai_widget) {
        g_claw_ai_widget->hide ();
    }
    return TMSCM_UNSPECIFIED;
}

/**
 * @brief 添加消息到 Claw AI 窗口
 */
tmscm
tm_claw_ai_widget_append (tmscm args) {
    tmscm role_scm = tmscm_car (args);
    tmscm content_scm = tmscm_cadr (args);
    
    string role = tmscm_to_string (role_scm);
    string content = tmscm_to_string (content_scm);
    
    QTMClawAIWidget* widget = get_claw_ai_widget ();
    if (widget) {
        widget->appendMessage (QString::fromUtf8 (as_charp (role)),
                               QString::fromUtf8 (as_charp (content)));
    }
    return TMSCM_UNSPECIFIED;
}

/**
 * @brief 更新最后一条消息（流式输出）
 */
tmscm
tm_claw_ai_widget_update_last (tmscm args) {
    tmscm content_scm = tmscm_car (args);
    string content = tmscm_to_string (content_scm);
    
    QTMClawAIWidget* widget = get_claw_ai_widget ();
    if (widget) {
        widget->updateLastMessage (QString::fromUtf8 (as_charp (content)));
    }
    return TMSCM_UNSPECIFIED;
}

/**
 * @brief 清空 Claw AI 消息
 */
tmscm
tm_claw_ai_widget_clear (tmscm args) {
    QTMClawAIWidget* widget = get_claw_ai_widget ();
    if (widget) {
        widget->clearChat ();
    }
    return TMSCM_UNSPECIFIED;
}

/**
 * @brief 设置流式输出状态
 */
tmscm
tm_claw_ai_widget_set_streaming (tmscm args) {
    tmscm streaming_scm = tmscm_car (args);
    bool streaming = tmscm_to_bool (streaming_scm);
    
    QTMClawAIWidget* widget = get_claw_ai_widget ();
    if (widget) {
        widget->setStreaming (streaming);
    }
    return TMSCM_UNSPECIFIED;
}

/**
 * @brief 获取消息数量
 */
tmscm
tm_claw_ai_widget_message_count (tmscm args) {
    QTMClawAIWidget* widget = get_claw_ai_widget ();
    if (widget) {
        return int_to_tmscm (widget->messageCount ());
    }
    return int_to_tmscm (0);
}

/******************************************************************************
 * Glue 注册
 ******************************************************************************/

void
initialize_claw_ai_glue () {
    tmscm_install_procedure ("claw-ai-widget-show", 
                             tm_claw_ai_widget_show, 
                             0, 0, 0);
    
    tmscm_install_procedure ("claw-ai-widget-hide", 
                             tm_claw_ai_widget_hide, 
                             0, 0, 0);
    
    tmscm_install_procedure ("claw-ai-widget-append", 
                             tm_claw_ai_widget_append, 
                             2, 0, 0);
    
    tmscm_install_procedure ("claw-ai-widget-update-last", 
                             tm_claw_ai_widget_update_last, 
                             1, 0, 0);
    
    tmscm_install_procedure ("claw-ai-widget-clear", 
                             tm_claw_ai_widget_clear, 
                             0, 0, 0);
    
    tmscm_install_procedure ("claw-ai-widget-set-streaming", 
                             tm_claw_ai_widget_set_streaming, 
                             1, 0, 0);
    
    tmscm_install_procedure ("claw-ai-widget-message-count", 
                             tm_claw_ai_widget_message_count, 
                             0, 0, 0);
}
