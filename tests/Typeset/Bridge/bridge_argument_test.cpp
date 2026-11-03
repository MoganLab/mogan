/******************************************************************************
 * MODULE     : bridge_argument_test.cpp
 * DESCRIPTION: Tests for bridge_argument_rep change notification safety
 * COPYRIGHT  : (C) 2026 Mogan contributors
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "Bridge/bridge.hpp"
#include "Metafont/load_tex.hpp"
#include "base.hpp"
#include "data_cache.hpp"
#include "env.hpp"
#include "tm_sys_utils.hpp"
#include "typesetter.hpp"
#include <QtTest/QtTest>
#include <moebius/drd/drd_std.hpp>

using namespace moebius;
using moebius::drd::std_drd;

static edit_env
create_test_env () {
  drd_info              drd ("none", std_drd);
  hashmap<string, tree> h1 (UNINIT), h2 (UNINIT);
  hashmap<string, tree> h3 (UNINIT), h4 (UNINIT);
  hashmap<string, tree> h5 (UNINIT), h6 (UNINIT);
  return edit_env (drd, "none", h1, h2, h3, h4, h5, h6);
}

class TestBridgeArgument : public QObject {
  Q_OBJECT

private slots:
  void initTestCase () {
    init_lolly ();
    init_texmacs_home_path ();
    cache_initialize ();
    init_tex ();
  }

  void test_notify_change_when_body_nil ();
  void test_document_notify_remove_with_arg_child ();
};

void
TestBridgeArgument::test_notify_change_when_body_nil () {
  edit_env   env= create_test_env ();
  typesetter ttt= new_typesetter (env, tree (DOCUMENT), path ());

  bridge arg_br= make_bridge (ttt, tree (ARG, "test"), path ());
  // Calling notify_change on an un-typeset bridge_argument must not crash.
  arg_br->notify_change ();
  QCOMPARE (arg_br->status, CORRUPTED);

  delete_typesetter (ttt);
}

void
TestBridgeArgument::test_document_notify_remove_with_arg_child () {
  edit_env   env= create_test_env ();
  tree       doc= tree (DOCUMENT, tree (ARG, "x"), tree (CONCAT, "item 1"),
                        tree (CONCAT, "item 2"));
  typesetter ttt= new_typesetter (env, doc, path ());

  // Removing an item touches neighbors via notify_change, reaching the ARG
  // child.
  notify_remove (ttt, path (1), 1);
  QVERIFY (ttt != nullptr);

  delete_typesetter (ttt);
}

QTEST_MAIN (TestBridgeArgument)
#include "bridge_argument_test.moc"
