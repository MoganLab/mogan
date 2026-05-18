
/******************************************************************************
 * MODULE     : startup_tab_widget_test.cpp
 * DESCRIPTION: Integration tests for QTMTemplatePage signal chain
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "QTMTemplatePage.hpp"
#include "base.hpp"
#include "template_manager.hpp"
#include <QGridLayout>
#include <QLabel>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QtTest/QtTest>

class TestStartupTabWidget : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    QStandardPaths::setTestModeEnabled (true);
    init_lolly ();

    // 预初始化 TemplateManager singleton，抑制 Scheme 解释器未就绪的预期警告
    QtMessageHandler oldHandler= qInstallMessageHandler (
        [] (QtMsgType, const QMessageLogContext&, const QString&) {});
    TemplateManager::instance ()->initialize ();
    qInstallMessageHandler (oldHandler);
  }

  // --- 构造与初始化 ---

  // QTMTemplatePage 应能正常构造和初始化
  void test_template_page_construct_and_initialize () {
    QTMTemplatePage page;
    // setupUI 在构造函数中调用，grid widget 应立即存在
    QWidget* grid= page.findChild<QWidget*> ("startup-tab-grid");
    QVERIFY (grid != nullptr);

    page.initialize ();
    // initialize 连接信号后不应改变基本结构
    QVERIFY (page.findChild<QWidget*> ("startup-tab-grid") != nullptr);
  }

  // --- 信号响应 ---

  // 手动发射 TemplateManager::templatesLoaded 后，网格应被刷新
  void test_templates_loaded_signal_refreshes_grid () {
    QTMTemplatePage page;
    page.initialize ();

    TemplateManager* mgr= TemplateManager::instance ();
    QVERIFY (mgr != nullptr);
    QVERIFY (mgr->isInitialized ());

    // 手动发射信号，触发 onTemplatesLoaded
    emit mgr->templatesLoaded ();

    // 处理事件以便槽函数执行
    QCoreApplication::processEvents ();

    // 由于无模板数据，网格应显示 "No templates available."
    QWidget* grid= page.findChild<QWidget*> ("startup-tab-grid");
    QVERIFY (grid != nullptr);

    QList<QLabel*> labels          = grid->findChildren<QLabel*> ();
    bool           foundNoTemplates= false;
    for (QLabel* label : labels) {
      if (label->text ().contains ("No templates available")) {
        foundNoTemplates= true;
        break;
      }
    }
    QVERIFY2 (foundNoTemplates,
              "Grid should display 'No templates available.' after "
              "templatesLoaded signal with empty template list");
  }

  // --- 分类操作 ---

  // setCategory 不应导致崩溃
  void test_set_category_does_not_crash () {
    QTMTemplatePage page;
    page.initialize ();

    page.setCategory ("test-category");
    QCoreApplication::processEvents ();

    QVERIFY (page.currentCategory () == "test-category");
  }

  void test_refresh_grid_does_not_crash () {
    QTMTemplatePage page;
    page.initialize ();
    page.setCategory ("cat1");
    page.refreshGrid ();
    QCoreApplication::processEvents ();
    QVERIFY (true);
  }

  void test_set_category_with_display_name () {
    QTMTemplatePage page;
    page.initialize ();
    page.setCategory ("thesis", "Thesis");
    QCoreApplication::processEvents ();

    QVERIFY (page.currentCategory () == "thesis");
  }

  // --- 事件处理 ---

  void test_resize_event_does_not_crash () {
    QTMTemplatePage page;
    page.initialize ();
    page.resize (800, 600);
    page.resize (400, 300);
    QVERIFY (true);
  }
};

QTEST_MAIN (TestStartupTabWidget)
#include "startup_tab_widget_test.moc"
