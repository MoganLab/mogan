/******************************************************************************
 * MODULE     : input_file_flush_test.cpp
 * COPYRIGHT  : (C) 2026  OpenAI
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "base.hpp"
#include "analyze.hpp"
#include "file.hpp"
#include "Generic/input.hpp"
#include "tm_url.hpp"
#include "tm_link.hpp"
#include "tree.hpp"
#include "tree_helper.hpp"
#include "url.hpp"
#include <QtTest/QtTest>

class TestInputFileFlush : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }
  void test_file_protocol_loads_pdf_as_image ();
};

void
TestInputFileFlush::test_file_protocol_loads_pdf_as_image () {
  url pdf_url= resolve (url_system ("$TEXMACS_PATH/tests/PDF/pdf_1_4_sample.pdf"),
                        "r");
  QVERIFY (exists (pdf_url));

  string pdf_path   = concretize (pdf_url);
  string pdf_bytes  = string_load (pdf_url);
  url    reparsed   = url_system (pdf_path);
  QVERIFY (exists (reparsed));
  qcompare (suffix (reparsed), "pdf");

  string protocol_s;
  protocol_s << DATA_BEGIN << "file:" << pdf_path
             << "?width=0.3par&height=0px" << DATA_END;

  texmacs_input in ("output");
  for (int i= 0; i < N (protocol_s); ++i)
    (void) in->put (protocol_s[i]);

  tree doc= in->get ("output");
  QVERIFY (is_document (doc));
  QCOMPARE (N (doc), 1);

  tree image= doc[0];
  QVERIFY (is_func (image, moebius::IMAGE, 5));
  QVERIFY (is_func (image[0], moebius::TUPLE, 2));
  QVERIFY (is_func (image[0][0], moebius::RAW_DATA, 1));

  qcompare (as_string (image[0][1]), "pdf");
  qcompare (as_string (image[1]), "0.3par");
  qcompare (as_string (image[2]), "");
  QCOMPARE (N (as_string (image[0][0][0])), N (pdf_bytes));
}

QTEST_MAIN (TestInputFileFlush)
#include "input_file_flush_test.moc"
