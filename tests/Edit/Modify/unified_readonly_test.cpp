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
#include "converter.hpp"
#include "data_cache.hpp"
#include "dictionary.hpp"
#include "editor.hpp"
#include "file.hpp"
#include "gui.hpp"
#include "language.hpp"
#include "modification.hpp"
#include "new_buffer.hpp"
#include "new_document.hpp"
#include "new_view.hpp"
#ifdef QTTEXMACS
#include "Qt/qt_sys_utils.hpp"
#endif
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

    // 6. 有未保存修改时启用只读，应先保存再启用只读，避免修改丢失
    url local_vw= get_passive_view (local_url);
    QVERIFY (!is_none (local_vw));
    editor local_vw_ed= view_to_editor (local_vw);
    assign (local_buf->rp * 0, tree ("saved_before_ro"));
    pretend_buffer_modified (local_url);
    QVERIFY (local_buf->needs_to_be_saved ());
    local_vw_ed->set_file_read_only (true);
    QVERIFY (!local_buf->needs_to_be_saved ());
    QVERIFY (local_vw_ed->get_file_read_only ());
    QVERIFY (local_vw_ed->is_read_only ());
    string saved_content;
    QVERIFY (!load_string (local_url, saved_content, false));
    QVERIFY (occurs ("saved_before_ro", saved_content));
    delete_view (local_vw);

    // 7. 外部修改文件权限后重新加载 (set_buffer_tree)，同步更新 read_only
    // 与标题
#ifdef QTTEXMACS
    local_ed->set_file_read_only (false);
    QVERIFY (!local_buf->buf->read_only);
    QVERIFY (qt_set_file_readonly (local_url, true));
    set_buffer_tree (local_url, tree (DOCUMENT, tree ("reloaded_ro")));
    QVERIFY (local_buf->buf->read_only);
    QVERIFY (local_ed->is_read_only ());
    string ro_prefix= '[' * cork_to_utf8 (translate ("read-only")) * "] ";
    QVERIFY (starts (local_buf->buf->title, ro_prefix));

    QVERIFY (qt_set_file_readonly (local_url, false));
    set_buffer_tree (local_url, tree (DOCUMENT, tree ("reloaded_rw")));
    QVERIFY (!local_buf->buf->read_only);
    QVERIFY (!local_ed->is_read_only ());
    QVERIFY (!starts (local_buf->buf->title, ro_prefix));
#endif

    // 8. 未注册 handler 的 tmfs 默认可编辑
    url default_tmfs= url_system ("tmfs://permission-default-cpp/buffer");
    set_buffer_tree (default_tmfs, tree (DOCUMENT, tree ("editable")));
    tm_buffer default_tmfs_buf= concrete_buffer (default_tmfs);
    QVERIFY (!is_nil (default_tmfs_buf));
    QVERIFY (!default_tmfs_buf->buf->read_only);
    default_tmfs_buf->data->init ("no-zoom")  = "true";
    default_tmfs_buf->data->init (ZOOM_FACTOR)= "1.0";
    editor default_tmfs_ed= new_editor (nullptr, default_tmfs_buf);
    QVERIFY (!default_tmfs_ed->is_read_only ());
    edit_announce (default_tmfs_ed.operator->(),
                   mod_assign (default_tmfs_buf->rp * 0, tree ("updated")));

    // 只有 load handler 不再隐式导致只读
    eval_scheme ("(tmfs-handler \"permission-load-cpp\" 'load "
                 "(lambda (name) '(document \"loaded\")))");
    url load_tmfs= url_system ("tmfs://permission-load-cpp/buffer");
    set_buffer_tree (load_tmfs, tree (DOCUMENT, tree ("editable")));
    tm_buffer load_tmfs_buf= concrete_buffer (load_tmfs);
    QVERIFY (!is_nil (load_tmfs_buf));
    QVERIFY (!load_tmfs_buf->buf->read_only);

    // 重命名已有只读前缀的 tmfs 缓冲区时，不重复添加 [只读] 前缀
    url tmfs_a= url_system ("tmfs://help/readonly_rename_a");
    url tmfs_b= url_system ("tmfs://help/readonly_rename_b");
    set_buffer_tree (tmfs_a, tree (DOCUMENT, tree ("tmfs")));
    tm_buffer tmfs_buf= concrete_buffer (tmfs_a);
    QVERIFY (!is_nil (tmfs_buf));
    string ro_prefix_tmfs= '[' * cork_to_utf8 (translate ("read-only")) * "] ";
    tmfs_buf->buf->read_only= true;
    set_title_buffer (tmfs_a, ro_prefix_tmfs * "Document");
    rename_buffer (tmfs_a, tmfs_b);
    QCOMPARE (tmfs_buf->buf->title, ro_prefix_tmfs * "Document");
    QVERIFY (!starts (tmfs_buf->buf->title, ro_prefix_tmfs * ro_prefix_tmfs));

    // 9. 显式 Chat 权限声明先于 buffer 创建：input 可写、message 只读，
    //    系统写入豁免允许后台更新只读消息区
    eval_scheme ("(use-modules (llm chat-permissions))");
    url chat_inp_url= url_system ("tmfs://chat/unit-test/input");
    set_buffer_tree (chat_inp_url, tree (DOCUMENT, tree ("")));
    tm_buffer chat_inp_buf= concrete_buffer (chat_inp_url);
    QVERIFY (!is_nil (chat_inp_buf));
    QVERIFY (!chat_inp_buf->buf->read_only);

    url chat_msg_url= url_system ("tmfs://chat/unit-test/message");
    set_buffer_tree (chat_msg_url, tree (DOCUMENT, tree ("hello")));
    tm_buffer chat_msg_buf= concrete_buffer (chat_msg_url);
    QVERIFY (!is_nil (chat_msg_buf));
    QVERIFY (chat_msg_buf->buf->read_only);
    chat_msg_buf->data->init ("no-zoom")  = "true";
    chat_msg_buf->data->init (ZOOM_FACTOR)= "1.0";
    editor chat_msg_ed                    = new_editor (nullptr, chat_msg_buf);
    QVERIFY (chat_msg_ed->is_read_only ());

    threw= false;
    try {
      edit_announce (chat_msg_ed.operator->(),
                     mod_assign (chat_msg_buf->rp * 0, tree ("blocked")));
    } catch (string msg) {
      threw= (msg == "read-only");
    }
    QVERIFY (threw);

    begin_system_document_load ();
    edit_announce (chat_msg_ed.operator->(),
                   mod_assign (chat_msg_buf->rp * 0, tree ("streamed")));
    end_system_document_load ();
    QVERIFY (!in_system_document_load ());

    threw= false;
    try {
      edit_announce (chat_msg_ed.operator->(),
                     mod_assign (chat_msg_buf->rp * 0, tree ("blocked-again")));
    } catch (string msg) {
      threw= (msg == "read-only");
    }
    QVERIFY (threw);

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
