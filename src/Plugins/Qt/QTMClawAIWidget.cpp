/******************************************************************************
 * MODULE     : QTMClawAIWidget.cpp
 * DESCRIPTION: Claw AI chat widget implementation
 * COPYRIGHT  : (C) 2026 Liii Network
 ******************************************************************************/

#include "QTMClawAIWidget.hpp"
#include "qt_gui.hpp"
#include "server.hpp"

#include <QKeyEvent>
#include <QScrollBar>
#include <QDateTime>
#include <QPainter>

// 样式常量定义
const QString QTMClawAIWidget::USER_STYLE = 
    "<div style='margin: 8px 0; padding: 10px; background: #e3f2fd; "
    "border-radius: 8px; border-left: 4px solid #2196f3;'>"
    "<b style='color: #1976d2;'>👤 You</b><br/>";

const QString QTMClawAIWidget::ASSISTANT_STYLE = 
    "<div style='margin: 8px 0; padding: 10px; background: #f3e5f5; "
    "border-radius: 8px; border-left: 4px solid #9c27b0;'>"
    "<b style='color: #7b1fa2;'>🤖 Claw AI</b><br/>";

const QString QTMClawAIWidget::STREAMING_STYLE = 
    "<span style='color: #666; font-style: italic;'>▌</span>";

QTMClawAIWidget::QTMClawAIWidget(QWidget* parent)
    : QTMAuxiliaryWidget("Claw AI", parent)
    , m_isStreaming(false) {
    
    setupUI();
    setupConnections();
    
    // 设置对象名，便于样式表定位
    setObjectName("claw_ai_widget");
}

QTMClawAIWidget::~QTMClawAIWidget() {
    // 清理工作
}

void QTMClawAIWidget::setupUI() {
    // 创建主布局
    QWidget* container = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);
    
    // 消息显示区（使用 QTextEdit 支持富文本）
    m_messageDisplay = new QTextEdit(this);
    m_messageDisplay->setReadOnly(true);
    m_messageDisplay->setFrameStyle(QFrame::NoFrame);
    m_messageDisplay->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_messageDisplay->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_messageDisplay->setPlaceholderText("Chat with Claw AI...");
    
    // 设置消息区样式
    m_messageDisplay->setStyleSheet(
        "QTextEdit {"
        "  background: #fafafa;"
        "  border: 1px solid #e0e0e0;"
        "  border-radius: 6px;"
        "  padding: 8px;"
        "  font-size: 13px;"
        "  line-height: 1.5;"
        "}"
    );
    
    mainLayout->addWidget(m_messageDisplay, 1); // 占据主要空间
    
    // 输入区域
    QWidget* inputArea = new QWidget(this);
    QHBoxLayout* inputLayout = new QHBoxLayout(inputArea);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(8);
    
    // 输入框
    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setPlaceholderText("Type your message... (Enter to send)");
    m_inputEdit->setStyleSheet(
        "QLineEdit {"
        "  padding: 8px 12px;"
        "  border: 1px solid #ccc;"
        "  border-radius: 6px;"
        "  font-size: 13px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #2196f3;"
        "}"
    );
    
    // 发送按钮
    m_sendButton = new QPushButton("Send", this);
    m_sendButton->setStyleSheet(
        "QPushButton {"
        "  padding: 8px 16px;"
        "  background: #2196f3;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 6px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: #1976d2;"
        "}"
        "QPushButton:pressed {"
        "  background: #1565c0;"
        "}"
        "QPushButton:disabled {"
        "  background: #ccc;"
        "}"
    );
    
    // 清空按钮
    m_clearButton = new QPushButton("Clear", this);
    m_clearButton->setStyleSheet(
        "QPushButton {"
        "  padding: 8px 12px;"
        "  background: transparent;"
        "  color: #666;"
        "  border: 1px solid #ccc;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background: #f5f5f5;"
        "  border-color: #999;"
        "}"
    );
    
    inputLayout->addWidget(m_inputEdit, 1);
    inputLayout->addWidget(m_clearButton);
    inputLayout->addWidget(m_sendButton);
    
    mainLayout->addWidget(inputArea);
    
    // 设置容器为 DockWidget 的内容
    setWidget(container);
    
    // 设置最小尺寸
    setMinimumWidth(320);
    setMinimumHeight(400);
}

void QTMClawAIWidget::setupConnections() {
    // 发送按钮
    connect(m_sendButton, &QPushButton::clicked, 
            this, &QTMClawAIWidget::onSendButtonClicked);
    
    // 输入框回车
    connect(m_inputEdit, &QLineEdit::returnPressed,
            this, &QTMClawAIWidget::onInputReturnPressed);
    
    // 清空按钮
    connect(m_clearButton, &QPushButton::clicked,
            this, &QTMClawAIWidget::onClearButtonClicked);
}

void QTMClawAIWidget::appendMessage(const QString& role, const QString& content) {
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    ClawAIMessage msg(role, content, timestamp, false);
    m_messages.append(msg);
    
    // 更新显示
    m_messageDisplay->append(formatMessage(msg));
    
    // 滚动到底部
    QScrollBar* scrollbar = m_messageDisplay->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void QTMClawAIWidget::updateLastMessage(const QString& content) {
    if (m_messages.isEmpty()) return;
    
    // 更新最后一条消息
    m_messages.last().content = content;
    m_messages.last().isStreaming = m_isStreaming;
    
    // 重新渲染所有消息
    m_messageDisplay->clear();
    for (const auto& msg : m_messages) {
        m_messageDisplay->append(formatMessage(msg));
    }
    
    // 滚动到底部
    QScrollBar* scrollbar = m_messageDisplay->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void QTMClawAIWidget::clearChat() {
    m_messages.clear();
    m_messageDisplay->clear();
    m_isStreaming = false;
    emit clearRequested();
}

void QTMClawAIWidget::setInputText(const QString& text) {
    m_inputEdit->setText(text);
}

QString QTMClawAIWidget::getInputText() const {
    return m_inputEdit->text();
}

void QTMClawAIWidget::setStreaming(bool streaming) {
    m_isStreaming = streaming;
    m_sendButton->setEnabled(!streaming);
    m_inputEdit->setEnabled(!streaming);
    
    if (streaming) {
        m_inputEdit->setPlaceholderText("Claw AI is thinking...");
    } else {
        m_inputEdit->setPlaceholderText("Type your message... (Enter to send)");
    }
}

int QTMClawAIWidget::messageCount() const {
    return m_messages.size();
}

ClawAIMessage QTMClawAIWidget::getMessage(int index) const {
    if (index >= 0 && index < m_messages.size()) {
        return m_messages.at(index);
    }
    return ClawAIMessage("", "");
}

QString QTMClawAIWidget::formatMessage(const ClawAIMessage& msg) {
    QString style = (msg.role == "user") ? USER_STYLE : ASSISTANT_STYLE;
    QString close = "</div>";
    
    QString content = msg.content;
    // 转义 HTML
    content.replace("&", "&amp;");
    content.replace("<", "&lt;");
    content.replace(">", "&gt;");
    // 保留换行
    content.replace("\n", "<br/>");
    
    // 流式输出指示器
    if (msg.isStreaming) {
        content += STREAMING_STYLE;
    }
    
    return style + content + close;
}

void QTMClawAIWidget::onSendButtonClicked() {
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty()) return;
    
    // 显示用户消息
    appendMessage("user", text);
    
    // 清空输入框
    m_inputEdit->clear();
    
    // 发送信号给 Scheme 层处理
    emit messageSent(text);
}

void QTMClawAIWidget::onInputReturnPressed() {
    onSendButtonClicked();
}

void QTMClawAIWidget::onClearButtonClicked() {
    clearChat();
}

void QTMClawAIWidget::closeEvent(QCloseEvent* event) {
    // 通过 scheme 层设置可见性
    exec_delayed(scheme_cmd(
        "(when (defined? 'claw-ai-hide) (claw-ai-hide))"));
    event->accept();
}

void QTMClawAIWidget::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Escape:
            // ESC 隐藏窗口
            this->close();
            break;
        default:
            QTMAuxiliaryWidget::keyPressEvent(event);
            break;
    }
}
