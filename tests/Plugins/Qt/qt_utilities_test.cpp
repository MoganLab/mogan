
/******************************************************************************
 * MODULE     : qt_utilities_test.cpp
 * COPYRIGHT  : (C) 2019  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Qt/qt_utilities.hpp"
#include "base.hpp"
#include "sys_utils.hpp"
#include <QTemporaryFile>
#include <Qt>
#include <QtTest/QtTest>

using namespace moebius;

class TestQtUtilities : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void test_qt_supports ();
  void test_from_modifiers ();
  void test_from_key_press_event ();
  void test_to_qstring_utf8 ();
  void test_from_qstring_utf8_roundtrip ();
  void test_title_encoding_roundtrip ();
  void test_qt_embed_tree_images_data_uri ();
  void test_qt_embed_tree_images_file_url ();
  void test_qt_embed_tree_images_already_embedded ();
  void test_qt_embed_tree_images_nested_in_document ();
  void test_qt_embed_tree_images_invalid_url_fallback ();
};

void
TestQtUtilities::test_qt_supports () {
#ifdef QTTEXMACS
  QVERIFY (qt_supports (url ("x.svg")));
  QVERIFY (qt_supports (url ("x.png")));
  QVERIFY (!qt_supports (url ("x.eps")));
  QVERIFY (!qt_supports (url ("x.ps")));
  QVERIFY (!qt_supports (url ("x.pdf")));
#endif
}

void
TestQtUtilities::test_from_modifiers () {
  qcompare (from_modifiers (Qt::NoModifier), "");
  qcompare (from_modifiers (Qt::ShiftModifier), "S-");
  qcompare (from_modifiers (Qt::AltModifier), "A-");
  qcompare (from_modifiers (Qt::AltModifier | Qt::ShiftModifier), "A-S-");
  if (os_macos ()) {
    qcompare (from_modifiers (Qt::MetaModifier), "C-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::AltModifier), "C-A-");
    qcompare (
        from_modifiers (Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier),
        "C-A-S-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::ShiftModifier), "C-S-");
    qcompare (from_modifiers (Qt::ControlModifier), "M-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier), "M-A-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier |
                              Qt::ShiftModifier),
              "M-A-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::ShiftModifier), "M-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::MetaModifier), "M-C-");
  }
  else {
    qcompare (from_modifiers (Qt::MetaModifier), "M-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::AltModifier), "M-A-");
    qcompare (
        from_modifiers (Qt::MetaModifier | Qt::AltModifier | Qt::ShiftModifier),
        "M-A-S-");
    qcompare (from_modifiers (Qt::MetaModifier | Qt::ShiftModifier), "M-S-");
    qcompare (from_modifiers (Qt::ControlModifier), "C-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier), "C-A-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::AltModifier |
                              Qt::ShiftModifier),
              "C-A-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::ShiftModifier), "C-S-");
    qcompare (from_modifiers (Qt::ControlModifier | Qt::MetaModifier), "M-C-");
  }
}

void
TestQtUtilities::test_from_key_press_event () {
  if (os_macos ()) {
    auto ctrl_plus= QKeyEvent (QEvent::KeyPress, (int) '=',
                               Qt::ControlModifier | Qt::ShiftModifier, "=");
    qcompare (from_key_press_event (&ctrl_plus), "M-S-=");

    // A-<number>
    auto alt_1= QKeyEvent (QEvent::KeyPress, (int) '1', Qt::AltModifier, "¡");
    qcompare (from_key_press_event (&alt_1), "A-1");
    // A-<alpha>
    auto alt_v= QKeyEvent (QEvent::KeyPress, (int) 'V', Qt::AltModifier, "√");
    qcompare (from_key_press_event (&alt_v), "A-v");
    // A-<not alpha and not number>
    auto alt_dot= QKeyEvent (QEvent::KeyPress, (int) '.', Qt::AltModifier, "≥");
    qcompare (from_key_press_event (&alt_dot), "≥");
  }
}

/*
 * [0250] Encoding tests for chat tab title storage.
 *
 * session->title stores UTF-8. to_qstring auto-detects Cork vs UTF-8.
 * These tests verify the encoding round-trip for UTF-8 inputs (CJK, Latin).
 * Note: Cork encoding tests require TeXmacs runtime dictionaries and
 * cannot run in a standalone test.
 */

// Helper: check that a Mogan string equals expected byte sequence
static bool
bytes_equal (string s, const char* expected) {
  string e (expected);
  if (N (s) != N (e)) return false;
  for (int i= 0; i < N (s); i++)
    if (s[i] != e[i]) return false;
  return true;
}

void
TestQtUtilities::test_to_qstring_utf8 () {
  // UTF-8: ö = 0xC3 0xB6 (two bytes)
  string  utf8_title= "Erwin Schr"
                      "\xC3\xB6"
                      "dinger";
  QString q         = to_qstring (utf8_title);
  QCOMPARE (q, QString::fromUtf8 ("Erwin Schrödinger"));

  // UTF-8: 你好 = E4 BD A0 E5 A5 BD
  string  cjk_title= "\xE4\xBD\xA0\xE5\xA5\xBD";
  QString q2       = to_qstring (cjk_title);
  QCOMPARE (q2, QString::fromUtf8 ("\xE4\xBD\xA0\xE5\xA5\xBD"));
}

void
TestQtUtilities::test_from_qstring_utf8_roundtrip () {
  // Latin: QString → from_qstring_utf8 → UTF-8 bytes
  QString qLatin= QString::fromUtf8 ("Erwin Schrödinger");
  string  utf8  = from_qstring_utf8 (qLatin);
  QVERIFY (bytes_equal (utf8, "Erwin Schr"
                              "\xC3\xB6"
                              "dinger"));

  // CJK: QString → from_qstring_utf8 → UTF-8 bytes
  QString qCJK    = QString::fromUtf8 ("\xE4\xBD\xA0\xE5\xA5\xBD");
  string  utf8_cjk= from_qstring_utf8 (qCJK);
  QVERIFY (bytes_equal (utf8_cjk, "\xE4\xBD\xA0\xE5\xA5\xBD"));
}

void
TestQtUtilities::test_title_encoding_roundtrip () {
  // Simulate the full title storage cycle:
  // to_qstring(mixed_input) → from_qstring_utf8 → store UTF-8
  // → to_qstring(stored_utf8) → display

  // CJK round-trip: UTF-8 verbatim → store UTF-8 → display
  {
    string  utf8_input= "\xE4\xBD\xA0\xE5\xA5\xBD";
    QString qTitle    = to_qstring (utf8_input);
    string  stored    = from_qstring_utf8 (qTitle);
    QVERIFY (bytes_equal (stored, "\xE4\xBD\xA0\xE5\xA5\xBD"));
    QString displayed= to_qstring (stored);
    QCOMPARE (displayed, QString::fromUtf8 ("\xE4\xBD\xA0\xE5\xA5\xBD"));
  }

  // Latin UTF-8 round-trip
  {
    string  utf8_input= "Schr"
                        "\xC3\xB6"
                        "dinger";
    QString qTitle    = to_qstring (utf8_input);
    string  stored    = from_qstring_utf8 (qTitle);
    QVERIFY (bytes_equal (stored, "Schr"
                                  "\xC3\xB6"
                                  "dinger"));
    QString displayed= to_qstring (stored);
    QCOMPARE (displayed, QString::fromUtf8 ("Schrödinger"));
  }

  // ASCII round-trip (no special encoding)
  {
    string  ascii_input= "Hello World";
    QString qTitle     = to_qstring (ascii_input);
    string  stored     = from_qstring_utf8 (qTitle);
    QVERIFY (bytes_equal (stored, "Hello World"));
    QString displayed= to_qstring (stored);
    QCOMPARE (displayed, QString ("Hello World"));
  }
}

void
TestQtUtilities::test_qt_embed_tree_images_data_uri () {
  // 1x1 transparent PNG as data URI
  string data_uri=
      "data:image/png;base64,"
      "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGA"
      "WjR9awAAAABJRU5ErkJggg==";
  tree t (IMAGE, data_uri, "0.6383w", "", "", "");
  qt_embed_tree_images (t);

  QVERIFY (is_func (t, IMAGE, 5));
  QVERIFY (is_func (t[0], TUPLE, 2));
  QVERIFY (is_func (t[0][0], RAW_DATA, 1));
  QCOMPARE (to_qstring (t[0][1]->label), QString ("png"));
  // Dimensions should be updated from default 0.6383w to 1pt x 1pt
  QCOMPARE (to_qstring (t[1]->label), QString ("1pt"));
  QCOMPARE (to_qstring (t[2]->label), QString ("1pt"));
}

void
TestQtUtilities::test_qt_embed_tree_images_file_url () {
  // Create a temporary PNG file
  QTemporaryFile tempFile (QDir::tempPath () + "/test_embed_XXXXXX.png");
  tempFile.setAutoRemove (true);
  QVERIFY (tempFile.open ());
  QImage img (2, 2, QImage::Format_RGB32);
  img.fill (Qt::red);
  QVERIFY (img.save (&tempFile, "PNG"));
  QString filePath= tempFile.fileName ();
  tempFile.close ();

  string file_url= "file:///" * from_qstring (filePath);
  tree   t (IMAGE, file_url, "0.6383w", "", "", "");
  qt_embed_tree_images (t);

  QVERIFY (is_func (t, IMAGE, 5));
  QVERIFY (is_func (t[0], TUPLE, 2));
  QVERIFY (is_func (t[0][0], RAW_DATA, 1));
  QCOMPARE (to_qstring (t[0][1]->label), QString ("png"));
  QCOMPARE (to_qstring (t[1]->label), QString ("2pt"));
  QCOMPARE (to_qstring (t[2]->label), QString ("2pt"));
}

void
TestQtUtilities::test_qt_embed_tree_images_already_embedded () {
  tree t (IMAGE, tuple (tree (RAW_DATA, "existing_binary_data"), "png"),
          "100pt", "50pt", "", "");
  qt_embed_tree_images (t);

  QVERIFY (is_func (t[0], TUPLE, 2));
  QVERIFY (is_func (t[0][0], RAW_DATA, 1));
  QCOMPARE (to_qstring (t[0][0][0]->label), QString ("existing_binary_data"));
  QCOMPARE (to_qstring (t[1]->label), QString ("100pt"));
  QCOMPARE (to_qstring (t[2]->label), QString ("50pt"));
}

void
TestQtUtilities::test_qt_embed_tree_images_nested_in_document () {
  string data_uri=
      "data:image/png;base64,"
      "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGA"
      "WjR9awAAAABJRU5ErkJggg==";
  tree doc (DOCUMENT, "Hello",
            tree (WITH, "par-mode", "center",
                  tree (IMAGE, data_uri, "0.6383w", "", "", "")),
            "World");
  qt_embed_tree_images (doc);

  // Check document structure preserved
  QVERIFY (is_func (doc, DOCUMENT, 3));
  QCOMPARE (to_qstring (doc[0]->label), QString ("Hello"));
  QCOMPARE (to_qstring (doc[2]->label), QString ("World"));

  // Check nested image embedded
  tree imgNode= doc[1][2];
  QVERIFY (is_func (imgNode, IMAGE, 5));
  QVERIFY (is_func (imgNode[0], TUPLE, 2));
  QVERIFY (is_func (imgNode[0][0], RAW_DATA, 1));
  QCOMPARE (to_qstring (imgNode[0][1]->label), QString ("png"));
}

void
TestQtUtilities::test_qt_embed_tree_images_invalid_url_fallback () {
  string invalid_url= "http://127.0.0.1:54321/nonexistent_image_12345.png";
  tree   t (IMAGE, invalid_url, "0.6383w", "", "", "");
  qt_embed_tree_images (t);

  // Fallback: tree unchanged
  QVERIFY (is_func (t, IMAGE, 5));
  QVERIFY (is_atomic (t[0]));
  QCOMPARE (to_qstring (t[0]->label), to_qstring (invalid_url));
  QCOMPARE (to_qstring (t[1]->label), QString ("0.6383w"));
}

QTEST_MAIN (TestQtUtilities)
#include "qt_utilities_test.moc"
