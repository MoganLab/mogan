
/******************************************************************************
 * MODULE     : startup_tab_widget_test.cpp
 * DESCRIPTION: Integration tests for QTMTemplatePage signal chain
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "base.hpp"
#include "QTMTemplatePage.hpp"
#include "template_manager.hpp"
#include <QtTest/QtTest>
#include <QLabel>
#include <QGridLayout>
#include <QSignalSpy>
#include <QStandardPaths>

class TestStartupTabWidget : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    QStandardPaths::setTestModeEnabled (true);
    init_lolly ();
  }

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

  // 手动发射 TemplateManager::templatesLoaded 后，网格应被刷新
  void test_templates_loaded_signal_refreshes_grid () {
    QTMTemplatePage page;
    page.initialize ();

    TemplateManager* mgr= TemplateManager::instance ();
    QVERIFY (mgr != nullptr);

    // 确保 TemplateManager 已初始化（无网络依赖，仅加载本地缓存）
    if (!mgr->isInitialized ()) {
      mgr->initialize ();
    }
    QVERIFY (mgr->isInitialized ());

    // 手动发射信号，触发 onTemplatesLoaded
    emit mgr->templatesLoaded ();

    // 处理事件以便槽函数执行
    QCoreApplication::processEvents ();

    // 由于无模板数据，网格应显示 "No templates available."
    QWidget* grid= page.findChild<QWidget*> ("startup-tab-grid");
    QVERIFY (grid != nullptr);

    QList<QLabel*> labels= grid->findChildren<QLabel*> ();
    bool foundNoTemplates= false;
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

  // 手动发射 categoriesLoaded 后，分类栏应被刷新
  void test_categories_loaded_signal_refreshes_bar () {
    QTMTemplatePage page;
    page.initialize ();

    TemplateManager* mgr= TemplateManager::instance ();
    if (!mgr->isInitialized ()) {
      mgr->initialize ();
    }

    emit mgr->categoriesLoaded ();
    QCoreApplication::processEvents ();

    // 分类栏存在但可能为空（无本地分类数据）
    QWidget* bar= page.findChild<QWidget*> ("startup-tab-category-bar");
    QVERIFY (bar != nullptr);
  }

  // resizeEvent 不应导致崩溃（不等待 debounce 定时器）
  void test_resize_event_does_not_crash () {
    QTMTemplatePage page;
    page.initialize ();
    page.resize (800, 600);
    page.resize (400, 300);
    // 如果到这里没有崩溃，测试通过
    QVERIFY (true);
  }
};

QTEST_MAIN (TestStartupTabWidget)
#include "startup_tab_widget_test.moc"
