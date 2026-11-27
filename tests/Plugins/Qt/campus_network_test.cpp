/******************************************************************************
 * MODULE     : campus_network_test.cpp
 * DESCRIPTION: Tests for campus network detection and display logic
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************/

#include "Qt/qt_tm_widget.hpp"
#include "base.hpp"
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtTest/QtTest>

class TestCampusNetwork : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_parse_campus_network_in_campus_with_institution () {
    const char* jsonStr= R"({
      "id": 10086,
      "username": "张三",
      "email": "zhangsan@gmail.com",
      "memberType": "Regular User",
      "periodLabel": "Non-member",
      "isInstitutionVip": false,
      "institutionName": null,
      "campus_network": {
        "in_campus": true,
        "institution_name": "清华大学"
      }
    })";

    QJsonDocument doc = QJsonDocument::fromJson (jsonStr);
    QJsonObject   data= doc.object ();

    CampusNetworkInfo info= qt_tm_widget_rep::parseCampusNetwork (data);
    QVERIFY (info.inCampus);
    QCOMPARE (info.institutionName, QString::fromUtf8 ("清华大学"));
  }

  void test_parse_campus_network_in_campus_without_institution () {
    const char* jsonStr= R"({
      "campus_network": {
        "in_campus": true,
        "institution_name": null
      }
    })";

    QJsonDocument doc = QJsonDocument::fromJson (jsonStr);
    QJsonObject   data= doc.object ();

    CampusNetworkInfo info= qt_tm_widget_rep::parseCampusNetwork (data);
    QVERIFY (info.inCampus);
    QVERIFY (info.institutionName.isEmpty ());
  }

  void test_parse_campus_network_not_in_campus () {
    const char* jsonStr= R"({
      "campus_network": {
        "in_campus": false,
        "institution_name": null
      }
    })";

    QJsonDocument doc = QJsonDocument::fromJson (jsonStr);
    QJsonObject   data= doc.object ();

    CampusNetworkInfo info= qt_tm_widget_rep::parseCampusNetwork (data);
    QVERIFY (!info.inCampus);
  }

  void test_parse_campus_network_missing_node () {
    QJsonObject       data;
    CampusNetworkInfo info= qt_tm_widget_rep::parseCampusNetwork (data);
    QVERIFY (!info.inCampus);
    QVERIFY (info.institutionName.isEmpty ());
  }

  void test_format_campus_network_text () {
    // 离开校园网：不显示任何特殊文本
    QCOMPARE (qt_tm_widget_rep::formatCampusNetworkText (false, "清华大学"),
              QString (""));

    // 处于校园网且包含高校名
    QString textWithInst= qt_tm_widget_rep::formatCampusNetworkText (
        true, QString::fromUtf8 ("清华大学"));
    QVERIFY (!textWithInst.isEmpty ());
    QVERIFY (textWithInst.contains (QString::fromUtf8 ("清华大学")));

    // 处于校园网但无高校名
    QString textWithoutInst=
        qt_tm_widget_rep::formatCampusNetworkText (true, "");
    QVERIFY (!textWithoutInst.isEmpty ());
  }
};

QTEST_MAIN (TestCampusNetwork)
#include "campus_network_test.moc"
