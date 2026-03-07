/******************************************************************************
 * MODULE     : QTMClawAIWidget.hpp
 * DESCRIPTION: Claw AI chat widget for Mogan
 * COPYRIGHT  : (C) 2026 Liii Network
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTM_CLAW_AI_WIDGET_HPP
#define QTM_CLAW_AI_WIDGET_HPP

#include "QTMAuxiliaryWidget.hpp"

#include <QDockWidget>
#include <QListView>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStringListModel>
#include <QTimer>

/**
 * @brief Claw AI 聊天消息结构
 */
struct ClawAIMessage {
    QString role;      // "user" 或 "assistant"
    QString content;   // 消息内容
    QString timestamp; // 时间戳
    bool isStreaming;  // 是否正在流式输出

    ClawAIMessage(const QString& r, const QString& c, 
                  const QString& ts = "", bool streaming = false)
        : role(r), content(c), timestamp(ts), isStreaming(streaming) {}
};

/**
 * @brief Claw AI 聊天窗口组件
 * 
 * 设计原则：
 * 1. C++ 层只负责 UI 渲染和事件处理
 * 2. 业务逻辑（消息历史、状态管理）交给 Scheme 层
 * 3. 提供简洁的接口供 Scheme 调用
 */
class QTMClawAIWidget : public QTMAuxiliaryWidget {
    Q_OBJECT

public:
    explicit QTMClawAIWidget(QWidget* parent = nullptr);
    ~QTMClawAIWidget();

    // ========== Scheme 接口 ==========
    
    /**
     * @brief 添加消息到聊天列表
     * @param role 角色 ("user" 或 "assistant")
     * @param content 消息内容
     */
    void appendMessage(const QString& role, const QString& content);
    
    /**
     * @brief 更新最后一条消息（用于流式输出）
     * @param content 更新的内容
     */
    void updateLastMessage(const QString& content);
    
    /**
     * @brief 清空聊天记录
     */
    void clearChat();
    
    /**
     * @brief 设置输入框内容
     */
    void setInputText(const QString& text);
    
    /**
     * @brief 获取输入框内容
     */
    QString getInputText() const;
    
    /**
     * @brief 设置是否正在流式输出
     */
    void setStreaming(bool streaming);
    
    /**
     * @brief 获取消息数量
     */
    int messageCount() const;
    
    /**
     * @brief 获取指定索引的消息
     */
    ClawAIMessage getMessage(int index) const;

signals:
    /**
     * @brief 用户发送消息信号
     * Scheme 层连接此信号处理发送逻辑
     */
    void messageSent(const QString& message);
    
    /**
     * @brief 用户请求清空聊天
     */
    void clearRequested();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onSendButtonClicked();
    void onInputReturnPressed();
    void onClearButtonClicked();

private:
    void setupUI();
    void setupConnections();
    QString formatMessage(const ClawAIMessage& msg);
    
    // UI 组件
    QListView* m_messageList;           // 消息列表视图
    QStringListModel* m_messageModel;   // 消息数据模型
    QTextEdit* m_messageDisplay;        // 消息显示区（富文本）
    QLineEdit* m_inputEdit;             // 输入框
    QPushButton* m_sendButton;          // 发送按钮
    QPushButton* m_clearButton;         // 清空按钮
    
    // 数据
    QList<ClawAIMessage> m_messages;    // 消息列表
    bool m_isStreaming;                 // 是否正在流式输出
    
    // 样式常量
    static const QString USER_STYLE;
    static const QString ASSISTANT_STYLE;
    static const QString STREAMING_STYLE;
};

#endif // QTM_CLAW_AI_WIDGET_HPP
