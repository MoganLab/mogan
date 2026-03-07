/******************************************************************************
 * MODULE     : claw_ai_widget_test.cpp
 * DESCRIPTION: Unit tests for QTMClawAIWidget
 * COPYRIGHT  : (C) 2026 Liii Network
 ******************************************************************************/

#include "QTMClawAIWidget.hpp"
#include <QtTest/QtTest>
#include <QApplication>

class TestClawAIWidget : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testInitialState();
    void testAppendMessage_User();
    void testAppendMessage_Assistant();
    void testAppendMultipleMessages();
    void testUpdateLastMessage();
    void testClearChat();
    void testSetAndGetInputText();
    void testSetStreaming();
    void testGetMessage_InvalidIndex();
    void testAppendEmptyMessage();
    void testAppendLongMessage();
    void testAppendMessageWithSpecialChars();
    void testAppendMultilineMessage();

private:
    QTMClawAIWidget* widget;
};

void TestClawAIWidget::initTestCase() {
    widget = new QTMClawAIWidget(nullptr);
}

void TestClawAIWidget::cleanupTestCase() {
    delete widget;
    widget = nullptr;
}

void TestClawAIWidget::testInitialState() {
    QCOMPARE(widget->messageCount(), 0);
    QCOMPARE(widget->getInputText(), QString(""));
}

void TestClawAIWidget::testAppendMessage_User() {
    widget->clearChat();
    widget->appendMessage("user", "Hello, Claw AI!");
    
    QCOMPARE(widget->messageCount(), 1);
    
    auto msg = widget->getMessage(0);
    QCOMPARE(msg.role, QString("user"));
    QCOMPARE(msg.content, QString("Hello, Claw AI!"));
    QVERIFY(!msg.isStreaming);
}

void TestClawAIWidget::testAppendMessage_Assistant() {
    widget->clearChat();
    widget->appendMessage("assistant", "Hello! How can I help you?");
    
    QCOMPARE(widget->messageCount(), 1);
    
    auto msg = widget->getMessage(0);
    QCOMPARE(msg.role, QString("assistant"));
    QCOMPARE(msg.content, QString("Hello! How can I help you?"));
}

void TestClawAIWidget::testAppendMultipleMessages() {
    widget->clearChat();
    widget->appendMessage("user", "Message 1");
    widget->appendMessage("assistant", "Response 1");
    widget->appendMessage("user", "Message 2");
    
    QCOMPARE(widget->messageCount(), 3);
    
    QCOMPARE(widget->getMessage(0).content, QString("Message 1"));
    QCOMPARE(widget->getMessage(1).content, QString("Response 1"));
    QCOMPARE(widget->getMessage(2).content, QString("Message 2"));
}

void TestClawAIWidget::testUpdateLastMessage() {
    widget->clearChat();
    widget->appendMessage("assistant", "Hello");
    widget->setStreaming(true);
    widget->updateLastMessage("Hello, world!");
    
    QCOMPARE(widget->messageCount(), 1);
    QCOMPARE(widget->getMessage(0).content, QString("Hello, world!"));
    QVERIFY(widget->getMessage(0).isStreaming);
}

void TestClawAIWidget::testClearChat() {
    widget->clearChat();  // 先清空，确保初始状态干净
    
    widget->appendMessage("user", "Test message");
    widget->appendMessage("assistant", "Test response");
    
    QCOMPARE(widget->messageCount(), 2);
    
    widget->clearChat();
    
    QCOMPARE(widget->messageCount(), 0);
}

void TestClawAIWidget::testSetAndGetInputText() {
    widget->setInputText("Test input");
    QCOMPARE(widget->getInputText(), QString("Test input"));
    
    widget->setInputText("");
    QCOMPARE(widget->getInputText(), QString(""));
}

void TestClawAIWidget::testSetStreaming() {
    widget->setStreaming(true);
    // 流式输出时，输入框和发送按钮应被禁用
    // 注意：这里需要实际运行 GUI 才能验证
    
    widget->setStreaming(false);
    // 恢复正常状态
}

void TestClawAIWidget::testGetMessage_InvalidIndex() {
    widget->clearChat();
    auto msg = widget->getMessage(-1);
    QCOMPARE(msg.role, QString(""));
    QCOMPARE(msg.content, QString(""));
    
    msg = widget->getMessage(100);
    QCOMPARE(msg.role, QString(""));
    QCOMPARE(msg.content, QString(""));
    widget->clearChat();  // 清理
}

void TestClawAIWidget::testAppendEmptyMessage() {
    widget->clearChat();
    widget->appendMessage("user", "");
    QCOMPARE(widget->messageCount(), 1);
    QCOMPARE(widget->getMessage(0).content, QString(""));
    widget->clearChat();  // 清理
}

void TestClawAIWidget::testAppendLongMessage() {
    widget->clearChat();
    QString longContent(1000, 'a');
    widget->appendMessage("user", longContent);
    
    QCOMPARE(widget->messageCount(), 1);
    QCOMPARE(widget->getMessage(0).content.size(), 1000);
    widget->clearChat();  // 清理
}

void TestClawAIWidget::testAppendMessageWithSpecialChars() {
    widget->clearChat();
    widget->appendMessage("user", "Hello <world> & \"test\"");
    
    auto msg = widget->getMessage(0);
    QCOMPARE(msg.content, QString("Hello <world> & \"test\""));
    widget->clearChat();  // 清理
}

void TestClawAIWidget::testAppendMultilineMessage() {
    widget->clearChat();
    QString multiline = "Line 1\nLine 2\nLine 3";
    widget->appendMessage("user", multiline);
    
    QCOMPARE(widget->getMessage(0).content, multiline);
    widget->clearChat();  // 清理
}

QTEST_MAIN(TestClawAIWidget)
#include "claw_ai_widget_test.moc"
