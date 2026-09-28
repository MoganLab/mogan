/******************************************************************************
 * MODULE     : tmfs_no_save_test.cpp
 * DESCRIPTION: Unit test for tmfs buffer no-save behavior
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "Data/new_buffer.hpp"
#include "base.hpp"
#include "new_view.hpp"
#include "tm_buffer.hpp"
#include "tmfs_url.hpp"

class TestTmfsNoSave : public QObject {
  Q_OBJECT

private slots:
  void init () {
    init_lolly ();
    the_et= tuple ();
  }

  void cleanup () { cleanup_qt_top_level_widgets (); }

  void test_is_no_save_buffer () {
    QCOMPARE (is_no_save_buffer (url ("tmfs://startup-tab")), true);
    QCOMPARE (is_no_save_buffer (url ("tmfs://chat-tab")), true);
    QCOMPARE (is_no_save_buffer (url ("tmfs://chat/test-session/input")), true);
    QCOMPARE (is_no_save_buffer (url ("tmfs://chat/test-session/message")),
              true);
    QCOMPARE (is_no_save_buffer (url ("tmfs://aux/search")), true);
    QCOMPARE (is_no_save_buffer (url ("tmfs://collab/doc1")), true);

    QCOMPARE (is_no_save_buffer (url ("/home/user/test.tmu")), false);
    QCOMPARE (is_no_save_buffer (url ("")), false);
  }

  void test_buffer_modified_tmfs () {
    QCOMPARE (buffer_modified (url ("tmfs://startup-tab")), false);
    QCOMPARE (buffer_modified (url ("tmfs://chat-tab")), false);
    QCOMPARE (buffer_modified (url ("tmfs://chat/test/input")), false);
    QCOMPARE (buffer_modified (url ("tmfs://chat/test/message")), false);

    QCOMPARE (buffer_modified_since_autosave (url ("tmfs://startup-tab")),
              false);
    QCOMPARE (buffer_modified_since_autosave (url ("tmfs://chat-tab")), false);
    QCOMPARE (buffer_modified_since_autosave (url ("tmfs://chat/test/input")),
              false);
    QCOMPARE (buffer_modified_since_autosave (url ("tmfs://chat/test/message")),
              false);
  }

  void test_tm_buffer_needs_to_be_saved () {
    tm_buffer_rep buf_startup (url ("tmfs://startup-tab"));
    QCOMPARE (buf_startup.needs_to_be_saved (), false);
    QCOMPARE (buf_startup.needs_to_be_autosaved (), false);

    tm_buffer_rep buf_chat (url ("tmfs://chat-tab"));
    QCOMPARE (buf_chat.needs_to_be_saved (), false);
    QCOMPARE (buf_chat.needs_to_be_autosaved (), false);

    tm_buffer_rep buf_input (url ("tmfs://chat/123/input"));
    QCOMPARE (buf_input.needs_to_be_saved (), false);
    QCOMPARE (buf_input.needs_to_be_autosaved (), false);

    tm_buffer_rep buf_msg (url ("tmfs://chat/123/message"));
    QCOMPARE (buf_msg.needs_to_be_saved (), false);
    QCOMPARE (buf_msg.needs_to_be_autosaved (), false);
  }

  void test_pretend_modified_tmfs_remains_unmodified () {
    url chat_input_url ("tmfs://chat/test_sid/input");
    pretend_buffer_modified (chat_input_url);

    QCOMPARE (buffer_modified (chat_input_url), false);
    QCOMPARE (buffer_modified_since_autosave (chat_input_url), false);
  }
};

QTEST_MAIN (TestTmfsNoSave)
#include "tmfs_no_save_test.moc"
