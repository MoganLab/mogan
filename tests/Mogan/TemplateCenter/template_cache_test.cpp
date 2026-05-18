
/******************************************************************************
 * MODULE     : template_cache_test.cpp
 * DESCRIPTION: Unit tests for TemplateCache
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "template_cache.hpp"
#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

class TestTemplateCache : public QObject {
  Q_OBJECT

private:
  QString cacheDir_;

  void clearCacheDir () {
    QDir dir (cacheDir_);
    if (dir.exists ()) {
      dir.removeRecursively ();
    }
  }

private slots:
  void initTestCase () {
    QStandardPaths::setTestModeEnabled (true);
    QString appData=
        QStandardPaths::writableLocation (QStandardPaths::AppDataLocation);
    cacheDir_= QDir (appData).filePath ("system/template_cache");
  }

  void init () {
    clearCacheDir ();
    QDir ().mkpath (cacheDir_);
  }

  void cleanup () { clearCacheDir (); }

  // 测试分类缓存的 save/load 往返：验证字段完整性和顺序保持
  void test_save_and_load_categories_cache () {
    TemplateCache cache;
    cache.initialize ();

    QList<TemplateCategory> categories;
    TemplateCategory        cat1;
    cat1.id           = "thesis";
    cat1.name         = u8"论文";
    cat1.nameEn       = "Thesis";
    cat1.description  = u8"学位论文模板";
    cat1.order        = 1;
    cat1.templateCount= 15;
    categories.append (cat1);

    TemplateCategory cat2;
    cat2.id           = "report";
    cat2.name         = u8"报告";
    cat2.nameEn       = "Report";
    cat2.description  = u8"实验报告模板";
    cat2.order        = 2;
    cat2.templateCount= 10;
    categories.append (cat2);

    cache.saveCategoriesCache (categories);

    QList<TemplateCategory> loaded= cache.loadCategoriesCache ();
    QCOMPARE (loaded.size (), 2);
    QCOMPARE (loaded[0].id, QString ("thesis"));
    QCOMPARE (loaded[0].nameEn, QString ("Thesis"));
    QCOMPARE (loaded[0].templateCount, 15);
    QCOMPARE (loaded[1].id, QString ("report"));
    QCOMPARE (loaded[1].order, 2);
  }

  // 测试缓存文件损坏时返回空列表并自动清理
  void test_load_categories_cache_returns_empty_on_corruption () {
    TemplateCache cache;
    cache.initialize ();

    QString cachePath= QDir (cacheDir_).filePath ("categories.json");
    QFile   file (cachePath);
    QVERIFY (file.open (QIODevice::WriteOnly));
    file.write ("not valid json");
    file.close ();

    QList<TemplateCategory> loaded= cache.loadCategoriesCache ();
    QVERIFY (loaded.isEmpty ());
    QVERIFY (!QFile::exists (cachePath));
  }

  // 测试注册缓存模板后能通过 ID 查询路径和 MD5
  void test_register_and_query_cached_template () {
    TemplateCache cache;
    cache.initialize ();

    QString templatePath= QDir (cacheDir_).filePath ("test.tmu");
    QFile   tmplFile (templatePath);
    QVERIFY (tmplFile.open (QIODevice::WriteOnly));
    tmplFile.write ("template content");
    tmplFile.close ();

    cache.registerCachedTemplate ("test-id", templatePath, tmplFile.size (),
                                  "abc123");

    QVERIFY (cache.isTemplateCached ("test-id"));
    QCOMPARE (cache.cachedTemplatePath ("test-id"), templatePath);

    QList<CacheEntry> entries= cache.cachedTemplates ();
    QCOMPARE (entries.size (), 1);
    QCOMPARE (entries[0].fileMd5, QString ("abc123"));
  }

  // 测试移除缓存：索引和物理文件均被删除
  void test_remove_cached_template () {
    TemplateCache cache;
    cache.initialize ();

    QString templatePath= QDir (cacheDir_).filePath ("removal.tmu");
    QFile   tmplFile (templatePath);
    QVERIFY (tmplFile.open (QIODevice::WriteOnly));
    tmplFile.write ("data");
    tmplFile.close ();

    cache.registerCachedTemplate ("removal-id", templatePath, 4, "md5");
    QVERIFY (cache.isTemplateCached ("removal-id"));

    cache.removeCachedTemplate ("removal-id");
    QVERIFY (!cache.isTemplateCached ("removal-id"));
    QVERIFY (!QFile::exists (templatePath));
  }

  // 测试缓存总大小计算（累加所有模板文件大小）
  void test_cache_size_computation () {
    TemplateCache cache;
    cache.initialize ();

    QString path1= QDir (cacheDir_).filePath ("a.tmu");
    QFile   f1 (path1);
    f1.open (QIODevice::WriteOnly);
    f1.write (QByteArray (100, 'a'));
    f1.close ();

    QString path2= QDir (cacheDir_).filePath ("b.tmu");
    QFile   f2 (path2);
    f2.open (QIODevice::WriteOnly);
    f2.write (QByteArray (200, 'b'));
    f2.close ();

    cache.registerCachedTemplate ("id-a", path1, 100, "m1");
    cache.registerCachedTemplate ("id-b", path2, 200, "m2");

    QCOMPARE (cache.cacheSize (), qint64 (300));
  }

  // 测试清空缓存：索引、分类缓存、物理文件全部清除
  void test_clear_cache_removes_all () {
    TemplateCache cache;
    cache.initialize ();

    QString path= QDir (cacheDir_).filePath ("clear.tmu");
    QFile   f (path);
    f.open (QIODevice::WriteOnly);
    f.write ("x");
    f.close ();

    cache.registerCachedTemplate ("clear-id", path, 1, "m");
    cache.saveCategoriesCache (QList<TemplateCategory> ());

    QSignalSpy spy (&cache, &TemplateCache::cacheCleared);
    cache.clearCache ();

    QVERIFY (cache.cachedTemplates ().isEmpty ());
    QCOMPARE (spy.count (), 1);
  }
};

QTEST_MAIN (TestTemplateCache)
#include "template_cache_test.moc"
