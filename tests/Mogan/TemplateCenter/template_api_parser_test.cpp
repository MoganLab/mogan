
/******************************************************************************
 * MODULE     : template_api_parser_test.cpp
 * DESCRIPTION: Unit tests for TemplateAPI response parsing
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#include "template_api.hpp"

#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

class TestTemplateAPI : public QObject {
  Q_OBJECT

private slots:
  // 测试分类列表解析：验证字段映射和按 order 排序
  void test_categories_response_parsing () {
    TemplateAPI api;

    QJsonArray cats;
    {
      QJsonObject o;
      o.insert ("categoryKey", "thesis");
      o.insert ("name", u8"论文");
      o.insert ("nameEn", "Thesis");
      o.insert ("description", u8"学位论文");
      o.insert ("order", 2);
      o.insert ("templateCount", 10);
      cats.append (o);
    }
    {
      QJsonObject o;
      o.insert ("categoryKey", "report");
      o.insert ("name", u8"报告");
      o.insert ("nameEn", "Report");
      o.insert ("description", u8"实验报告");
      o.insert ("order", 1);
      o.insert ("templateCount", 5);
      cats.append (o);
    }

    auto result= api.parseCategoriesResponse (QJsonValue (cats));
    QCOMPARE (result.size (), 2);
    QCOMPARE (result[0].id, QString ("report"));
    QCOMPARE (result[0].order, 1);
    QCOMPARE (result[1].id, QString ("thesis"));
    QCOMPARE (result[1].order, 2);
    QCOMPARE (result[1].templateCount, 10);
  }

  // 测试模板详情解析：验证所有字段映射（包括 category
  // 嵌套对象、tags、statistics）
  void test_templates_response_field_mapping () {
    TemplateAPI api;

    QJsonArray  items;
    QJsonObject tmpl;
    tmpl.insert ("templateKey", "nsfc-ysf-c");
    tmpl.insert ("name", u8"国自然青年C类");
    tmpl.insert ("description", u8"申请书模板");
    tmpl.insert ("author", "Liii Network");
    tmpl.insert ("version", "20260424");
    tmpl.insert ("license", "GPL-3.0");
    tmpl.insert ("thumbnailUrl", "https://cdn.liiistem.cn/images/thumb.png");
    tmpl.insert ("fileSize", 1024);
    tmpl.insert ("fileMd5", "53213b7dd8736afbf9a927cccac16533");
    tmpl.insert ("language", "zh-CN");

    QJsonObject categoryObj;
    categoryObj.insert ("categoryKey", "resume-report-application");
    categoryObj.insert ("name", u8"简历报告申请");
    tmpl.insert ("category", categoryObj);

    tmpl.insert ("url", "https://cdn.liiistem.cn/library/file.tmu");
    tmpl.insert ("pdfUrl", "https://cdn.liiistem.cn/library/file.pdf");
    tmpl.insert ("createTime", "2026-04-24T00:00:00Z");
    tmpl.insert ("updateTime", "2026-04-25T12:00:00Z");

    QJsonArray tags;
    tags.append ("NSFC");
    tags.append (u8"国自然");
    tmpl.insert ("tags", tags);

    QJsonObject compat;
    compat.insert ("mogan_min_version", "1.2.0");
    tmpl.insert ("compatibility", compat);

    QJsonObject stats;
    stats.insert ("downloads", 100);
    stats.insert ("rating", 4.5);
    tmpl.insert ("statistics", stats);

    items.append (tmpl);

    QJsonObject root;
    root.insert ("items", items);

    auto result= api.parseTemplatesResponse (QJsonValue (root));
    QCOMPARE (result.size (), 1);

    auto ptr= result.value ("nsfc-ysf-c");
    QVERIFY (!ptr.isNull ());
    QCOMPARE (ptr->id, QString ("nsfc-ysf-c"));
    QCOMPARE (ptr->name, QString (u8"国自然青年C类"));
    QCOMPARE (ptr->category, QString ("resume-report-application"));
    QCOMPARE (ptr->fileUrl,
              QString ("https://cdn.liiistem.cn/library/file.tmu"));
    QCOMPARE (ptr->previewUrl,
              QString ("https://cdn.liiistem.cn/library/file.pdf"));
    QCOMPARE (ptr->fileSize, qint64 (1024));
    QCOMPARE (ptr->fileMd5, QString ("53213b7dd8736afbf9a927cccac16533"));
    QCOMPARE (ptr->tags.size (), 2);
    QCOMPARE (ptr->downloadCount, 100);
    QCOMPARE (ptr->rating, 4.5);
    QCOMPARE (ptr->createdAt,
              QDateTime::fromString ("2026-04-24T00:00:00Z", Qt::ISODate));
    QCOMPARE (ptr->updatedAt,
              QDateTime::fromString ("2026-04-25T12:00:00Z", Qt::ISODate));
  }

  // 测试 createTime 缺失时回退到 created_at（兼容旧 API 格式）
  void test_created_at_fallback_when_createTime_missing () {
    TemplateAPI api;

    QJsonArray  items;
    QJsonObject tmpl;
    tmpl.insert ("templateKey", "legacy");
    tmpl.insert ("name", "Legacy");

    QJsonObject categoryObj;
    categoryObj.insert ("categoryKey", "test");
    tmpl.insert ("category", categoryObj);

    tmpl.insert ("url", "http://example.com/file.tmu");
    tmpl.insert ("created_at", "2026-01-01T00:00:00Z");
    tmpl.insert ("updated_at", "2026-06-01T00:00:00Z");
    items.append (tmpl);

    QJsonObject root;
    root.insert ("items", items);

    auto result= api.parseTemplatesResponse (QJsonValue (root));
    QCOMPARE (result.size (), 1);

    auto ptr= result.value ("legacy");
    QVERIFY (!ptr.isNull ());
    QCOMPARE (ptr->createdAt,
              QDateTime::fromString ("2026-01-01T00:00:00Z", Qt::ISODate));
    QCOMPARE (ptr->updatedAt,
              QDateTime::fromString ("2026-06-01T00:00:00Z", Qt::ISODate));
  }

  // 测试空数组返回空列表而非崩溃
  void test_empty_categories_returns_empty_list () {
    TemplateAPI api;
    auto result= api.parseCategoriesResponse (QJsonValue (QJsonArray ()));
    QVERIFY (result.isEmpty ());
  }

  // 测试非数组输入返回空列表（容错）
  void test_non_array_categories_returns_empty () {
    TemplateAPI api;
    auto result= api.parseCategoriesResponse (QJsonValue (QJsonObject ()));
    QVERIFY (result.isEmpty ());
  }
};

QTEST_MAIN (TestTemplateAPI)
#include "template_api_parser_test.moc"
