
/******************************************************************************
 * MODULE     : template_utils_test.cpp
 * DESCRIPTION: Unit tests for qt_template_utils
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "base.hpp"
#include <Qt>
#include <QtTest/QtTest>

#include "qt_template_utils.hpp"

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTemporaryDir>

class TestTemplateUtils : public QObject {
  Q_OBJECT

private:
  QString tempDocsDir_;

private slots:
  void initTestCase () {
    QStandardPaths::setTestModeEnabled (true);
    tempDocsDir_=
        QStandardPaths::writableLocation (QStandardPaths::DocumentsLocation);
    QVERIFY (!tempDocsDir_.isEmpty ());
  }

  void init () {
    QDir dir (tempDocsDir_);
    for (const auto& entry :
         dir.entryInfoList (QStringList ("*.tmu"), QDir::Files)) {
      QFile::remove (entry.absoluteFilePath ());
    }
  }

  // 测试基本文件名生成：输入模板名应生成对应 .tmu 路径
  void test_generate_basic_name () {
    QString path= qt_generate_document_save_path ("MyTemplate");
    QVERIFY (!path.isEmpty ());
    QVERIFY (path.endsWith ("MyTemplate.tmu"));
  }

  // 测试非法字符过滤：Windows 保留字符应被替换为下划线
  void test_generate_filters_illegal_chars () {
    QString path= qt_generate_document_save_path ("a/b:c*d?e\"f<g>h|i");
    QVERIFY (!path.isEmpty ());
    QFileInfo fi (path);
    QString   fileName= fi.fileName ();
    QVERIFY (!fileName.contains ("/"));
    QVERIFY (!fileName.contains (":"));
    QVERIFY (!fileName.contains ("*"));
    QVERIFY (!fileName.contains ("?"));
    QVERIFY (!fileName.contains ("\""));
    QVERIFY (!fileName.contains ("<"));
    QVERIFY (!fileName.contains (">"));
    QVERIFY (!fileName.contains ("|"));
    QVERIFY (fileName == "a_b_c_d_e_f_g_h_i.tmu");
  }

  // 测试空名称回退：输入空字符串时应回退到 "template"
  void test_generate_empty_name_fallback () {
    QString path= qt_generate_document_save_path ("");
    QVERIFY (!path.isEmpty ());
    QVERIFY (path.endsWith ("template.tmu"));
  }

  // 测试自动编号去重：同名文件已存在时应生成 Name(1).tmu
  void test_generate_auto_numbering () {
    QString baseName= "AutoNumTest";
    QString path1   = qt_generate_document_save_path (baseName);
    QVERIFY (!path1.isEmpty ());
    QVERIFY (path1.endsWith ("AutoNumTest.tmu"));

    QFile f1 (path1);
    f1.open (QIODevice::WriteOnly);
    f1.close ();
    QString path2= qt_generate_document_save_path (baseName);
    QVERIFY (!path2.isEmpty ());
    QVERIFY (path2.endsWith ("AutoNumTest(1).tmu"));
    QVERIFY (path1 != path2);

    QFile f2 (path2);
    f2.open (QIODevice::WriteOnly);
    f2.close ();
    QString path3= qt_generate_document_save_path (baseName);
    QVERIFY (path3.endsWith ("AutoNumTest(2).tmu"));
  }

  // 测试大批量编号：创建 100 个文件后仍应能正常生成下一个可用文件名
  void test_generate_massive_numbering () {
    QString baseName= "MassiveTest";
    for (int i= 0; i < 100; ++i) {
      QString path= qt_generate_document_save_path (baseName);
      QVERIFY (!path.isEmpty ());
      QFile f (path);
      QVERIFY (f.open (QIODevice::WriteOnly));
      f.close ();
    }
    QString nextPath= qt_generate_document_save_path (baseName);
    QVERIFY (!nextPath.isEmpty ());
    QVERIFY (nextPath.endsWith ("MassiveTest(100).tmu"));
  }

  // 测试正常拷贝：源文件存在时应成功拷贝到 Documents
  void test_copy_success () {
    QString sourcePath= QDir (tempDocsDir_).filePath ("source_test.tmu");
    QFile   sourceFile (sourcePath);
    QVERIFY (sourceFile.open (QIODevice::WriteOnly));
    sourceFile.write ("test content");
    sourceFile.close ();

    QString result=
        qt_copy_template_to_documents (sourcePath, "CopySuccessTest");
    QVERIFY (!result.isEmpty ());
    QVERIFY (QFile::exists (result));
    QCOMPARE (QFile (result).size (), qint64 (12));

    QFile::remove (sourcePath);
    QFile::remove (result);
  }

  // 测试源文件缺失：源文件不存在时应返回空字符串
  void test_copy_missing_source () {
    QString missingPath=
        QDir (tempDocsDir_).filePath ("definitely_missing_file.tmu");
    QVERIFY (!QFile::exists (missingPath));

    // 抑制预期内的 qWarning 输出，避免 Qt Test 将其标记为 QWARN
    QtMessageHandler oldHandler= qInstallMessageHandler (
        [] (QtMsgType, const QMessageLogContext&, const QString&) {});

    QString result= qt_copy_template_to_documents (missingPath, "MissingTest");

    qInstallMessageHandler (oldHandler);

    QVERIFY (result.isEmpty ());
  }

  // 测试 Documents 目录不存在时的自动创建：验证 mkpath 回退逻辑
  void test_generate_without_documents_dir () {
    QString docsDir= tempDocsDir_;
    if (QDir (docsDir).exists ()) {
      QVERIFY (QDir (docsDir).removeRecursively ());
    }

    QString path= qt_generate_document_save_path ("NoDocsTest");
    QVERIFY (!path.isEmpty ());
    QVERIFY (path.endsWith ("NoDocsTest.tmu"));
    QVERIFY (path.startsWith (docsDir));
    QVERIFY (QDir (docsDir).exists ());

    QDir ().mkpath (docsDir);
  }

  // 测试中文路径：验证非 ASCII 字符在路径中的处理
  void test_generate_with_chinese_name () {
    QString path= qt_generate_document_save_path (u8"中文模板测试");
    QVERIFY (!path.isEmpty ());
    QVERIFY2 (path.endsWith (u8"中文模板测试.tmu"),
              qPrintable ("Unexpected path with auto-numbering: " + path));

    QFile f (path);
    QVERIFY (f.open (QIODevice::WriteOnly));
    f.close ();
    QFile::remove (path);
  }

  // 测试中文路径自动编号：同名中文文件已存在时应生成 中文模板测试(1).tmu
  void test_generate_chinese_auto_numbering () {
    QString baseName= u8"编号测试";
    QString path1   = qt_generate_document_save_path (baseName);
    QVERIFY (!path1.isEmpty ());
    QVERIFY (path1.endsWith (u8"编号测试.tmu"));

    QFile f1 (path1);
    f1.open (QIODevice::WriteOnly);
    f1.close ();

    QString path2= qt_generate_document_save_path (baseName);
    QVERIFY (!path2.isEmpty ());
    QVERIFY (path2.endsWith (u8"编号测试(1).tmu"));
    QVERIFY (path1 != path2);

    QFile::remove (path1);
    QFile::remove (path2);
  }
};

QTEST_MAIN (TestTemplateUtils)
#include "template_utils_test.moc"
