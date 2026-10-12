/******************************************************************************
 * MODULE     : qt_guide_window_test.cpp
 * DESCRIPTION: Unit tests for StartupLoginDialog language and translations
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_guide_window.hpp"
#include "base.hpp"
#include "dictionary.hpp"
#include "preferences.hpp"
#include "qt_utilities.hpp"
#include "tm_sys_utils.hpp"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QtTest/QtTest>

class TestStartupLoginDialog : public QObject {
  Q_OBJECT

  string orig_lang_pref;
  bool   had_lang_pref;

private slots:
  void init () {
    init_lolly ();
    had_lang_pref= has_user_preference ("language");
    if (had_lang_pref) orig_lang_pref= get_user_preference ("language");
  }

  void cleanup () {
    if (had_lang_pref) set_user_preference ("language", orig_lang_pref);
    else reset_user_preference ("language");
    cleanup_qt_top_level_widgets ();
  }

  void test_is_chinese_language_preference ();
  void test_dialog_ui_language_switch ();
  void test_dictionary_translations ();
};

void
TestStartupLoginDialog::test_is_chinese_language_preference () {
  // 当首选项设为 chinese 时，返回 true
  set_user_preference ("language", "chinese");
  QCOMPARE (QWK::StartupLoginDialog::isChineseLanguage (), true);

  // 当首选项设为 english 时，返回 false
  set_user_preference ("language", "english");
  QCOMPARE (QWK::StartupLoginDialog::isChineseLanguage (), false);

  // 当首选项无 language 字段时，跟随编译条件
  reset_user_preference ("language");
#ifdef IS_OVERSEAS
  QCOMPARE (QWK::StartupLoginDialog::isChineseLanguage (), false);
#else
  QCOMPARE (QWK::StartupLoginDialog::isChineseLanguage (), true);
#endif
}

void
TestStartupLoginDialog::test_dialog_ui_language_switch () {
  // 中文模式
  set_user_preference ("language", "chinese");
  {
    QWK::StartupLoginDialog dlg;
    auto*                   title= dlg.findChild<QLabel*> ("titleLabel");
    auto*                   login= dlg.findChild<QPushButton*> ("loginButton");
    auto*                   skip = dlg.findChild<QPushButton*> ("skipButton");
    auto* backup= dlg.findChild<QCheckBox*> ("autoBackupCheckBox");

    QVERIFY (title != nullptr);
    QVERIFY (login != nullptr);
    QVERIFY (skip != nullptr);
    QVERIFY (backup != nullptr);

    QCOMPARE (title->text (), QStringLiteral ("欢迎使用 Liii STEM"));
    QCOMPARE (login->text (), QStringLiteral ("登录"));
    QCOMPARE (skip->text (), QStringLiteral ("跳过登录"));
    QCOMPARE (backup->text (), QStringLiteral ("开启自动备份"));
  }

  // 英文模式
  set_user_preference ("language", "english");
  {
    QWK::StartupLoginDialog dlg;
    auto*                   title= dlg.findChild<QLabel*> ("titleLabel");
    auto*                   login= dlg.findChild<QPushButton*> ("loginButton");
    auto*                   skip = dlg.findChild<QPushButton*> ("skipButton");
    auto* backup= dlg.findChild<QCheckBox*> ("autoBackupCheckBox");

    QVERIFY (title != nullptr);
    QVERIFY (login != nullptr);
    QVERIFY (skip != nullptr);
    QVERIFY (backup != nullptr);

    QCOMPARE (title->text (), QStringLiteral ("Welcome to Liii STEM"));
    QCOMPARE (login->text (), QStringLiteral ("Log In"));
    QCOMPARE (skip->text (), QStringLiteral ("Skip Login"));
    QCOMPARE (backup->text (), QStringLiteral ("Enable auto backup"));
  }
}

void
TestStartupLoginDialog::test_dictionary_translations () {
  // 验证 zh_CN 词典对新文本的翻译支持
  QCOMPARE (to_qstring (translate (string ("Previous"), "english", "chinese")),
            QStringLiteral ("上一步"));
  QCOMPARE (to_qstring (translate (string ("Next"), "english", "chinese")),
            QStringLiteral ("下一步"));
  QCOMPARE (to_qstring (translate (string ("Finish"), "english", "chinese")),
            QStringLiteral ("完成"));
  QCOMPARE (to_qstring (translate (string ("Accept"), "english", "chinese")),
            QStringLiteral ("接受"));
  QCOMPARE (to_qstring (translate (string ("Reject"), "english", "chinese")),
            QStringLiteral ("拒绝"));
  QCOMPARE (to_qstring (translate (string ("Cloud"), "english", "chinese")),
            QStringLiteral ("云端"));
  QCOMPARE (to_qstring (
                translate (string ("Login Successful"), "english", "chinese")),
            QStringLiteral ("登录成功"));
  QCOMPARE (to_qstring (translate (string ("Outline"), "english", "chinese")),
            QStringLiteral ("大纲"));
  QCOMPARE (to_qstring (translate (
                string ("OCR error, please contact customer support!"),
                "english", "chinese")),
            QStringLiteral ("识别异常，请联系客服！"));
  QCOMPARE (
      to_qstring (translate (
          string ("OCR result is empty, please contact customer support!"),
          "english", "chinese")),
      QStringLiteral ("识别结果为空，请联系客服！"));
}

#ifdef QTTEXMACS
QTEST_MAIN (TestStartupLoginDialog)
#else
int
main () {
  return 0;
}
#endif

#include "qt_guide_window_test.moc"
