/******************************************************************************
 * MODULE     : unified_readonly_test.cpp
 * DESCRIPTION: Unit tests for unified editor read-only mode
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "boot.hpp"
#include "data_cache.hpp"
#include "editor.hpp"
#include "file.hpp"
#include "gui.hpp"
#include "modification.hpp"
#include "new_buffer.hpp"
#include "new_document.hpp"
#include "s7_tm.hpp"
#include "server.hpp"
#include "tm_buffer.hpp"
#include "tm_sys_utils.hpp"
#include "tree_observer.hpp"
#include <QtTest/QtTest>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;

extern s7_pointer user_env;

class TestUnifiedReadonly : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    init_lolly ();
    init_texmacs_home_path ();
    init_texmacs_front ();
    int   argc  = 1;
    char* argv[]= {(char*) "unified_readonly_test", nullptr};
    gui_open (argc, argv);
    cache_initialize ();
    init_tex ();
    moebius::drd::init_std_drd ();
    if (!tm_s7) {
      tm_s7   = s7_init ();
      user_env= s7_inlet (tm_s7, s7_nil (tm_s7));
      s7_gc_protect (tm_s7, user_env);
    }
    new server (app_type::RESEARCH);
    the_et= tree (DOCUMENT);
  }

  void test_unified_readonly_modes_and_edit_announce () {
    url name= url_ramdisc ("unified_readonly_test.tm");
    set_buffer_tree (name, tree (DOCUMENT, tree ("initial")));
    tm_buffer buf= concrete_buffer (name);
    QVERIFY (!is_nil (buf));
    set_document (buf->rp, tree (DOCUMENT, tree ("initial")));
    buf->data->init ("no-zoom")  = "true";
    buf->data->init (ZOOM_FACTOR)= "1.0";

    editor ed= new_editor (nullptr, buf);
    QVERIFY (!ed->is_read_only ());
    QVERIFY (!ed->get_file_read_only ());
    QVERIFY (!ed->can_toggle_file_read_only ());

    // 1. 文件权限只读 (buf->buf->read_only) 触发统一只读
    buf->buf->read_only= true;
    QVERIFY (ed->get_file_read_only ());
    QVERIFY (ed->is_read_only ());

    bool threw= false;
    try {
      edit_announce (ed.operator->(),
                     mod_assign (buf->rp * 0, tree ("modified")));
    } catch (string msg) {
      threw= (msg == "read-only");
    }
    QVERIFY (threw);
    QCOMPARE (subtree (the_et, buf->rp * 0), tree ("initial"));

    // 2. 光标移动 (MOD_SET_CURSOR) 在只读模式下依然放行
    edit_announce (ed.operator->(),
                   mod_set_cursor (buf->rp * 0, 0, tree (TUPLE, tree ("x"))));

    // 3. 关闭只读标志后恢复可编辑
    buf->buf->read_only= false;
    QVERIFY (!ed->is_read_only ());
    edit_announce (ed.operator->(), mod_assign (buf->rp * 0, tree ("updated")));

    // 4. 系统文档装载 (set_document) 在只读模式下依然豁免
    buf->buf->read_only= true;
    QVERIFY (!in_system_document_load ());
    set_document (buf->rp, tree (DOCUMENT, tree ("reloaded")));
    QVERIFY (!in_system_document_load ());
    QCOMPARE (subtree (the_et, buf->rp), tree (DOCUMENT, tree ("reloaded")));

    buf->buf->read_only= false;

    // 5. 本地文件支持通过 set_file_read_only 切换文件写权限与只读状态
    url local_url= url_temp (".tm");
    save_string (local_url, "<TeXmacs|2.1.4>\n\n<style|generic>\n");
    set_buffer_tree (local_url, tree (DOCUMENT, tree ("local")));
    tm_buffer local_buf= concrete_buffer (local_url);
    QVERIFY (!is_nil (local_buf));
    set_document (local_buf->rp, tree (DOCUMENT, tree ("local")));
    local_buf->data->init ("no-zoom")  = "true";
    local_buf->data->init (ZOOM_FACTOR)= "1.0";

    editor local_ed= new_editor (nullptr, local_buf);
    QVERIFY (local_ed->can_toggle_file_read_only ());
    QVERIFY (!local_ed->get_file_read_only ());

    local_ed->set_file_read_only (true);
    QVERIFY (local_ed->get_file_read_only ());
    QVERIFY (local_ed->is_read_only ());

    threw= false;
    try {
      edit_announce (local_ed.operator->(),
                     mod_assign (local_buf->rp * 0, tree ("blocked")));
    } catch (string msg) {
      threw= (msg == "read-only");
    }
    QVERIFY (threw);

    local_ed->set_file_read_only (false);
    QVERIFY (!local_ed->get_file_read_only ());
    QVERIFY (!local_ed->is_read_only ());
    edit_announce (local_ed.operator->(),
                   mod_assign (local_buf->rp * 0, tree ("writable")));
    remove (local_url);
  }
};

#ifdef QTTEXMACS
QTEST_MAIN (TestUnifiedReadonly)
#else
int
main () {
  return 0;
}
#endif
#include "unified_readonly_test.moc"
