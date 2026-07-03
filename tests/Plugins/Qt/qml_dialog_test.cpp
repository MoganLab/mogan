/******************************************************************************
 * MODULE     : qml_dialog_test.cpp
 * DESCRIPTION: 单元测试 QML form 引擎的字段 tree 解析纯函数。
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "Qt/QTMQmlDialogInternal.hpp"
#include "base.hpp"

#include "tree_helper.hpp" // compound()

#include <QtTest/QtTest>

// 构造字段节点 (type label key (opts...) value)。
// opts 用 compound("tuple", ...) 造一个 compound；与运行期 stree->tree 的
// options 形状一致（field_tree_to_qml 只检查 is_compound，不读 options 自身
// label）。
static tree
make_field (const char* type, const char* label, const char* key, tree opts,
            const char* value) {
  return compound (string (type), tree (label), tree (key), opts, tree (value));
}

// 带 live 标志的重载：(type label key (opts...) value live)。
static tree
make_field (const char* type, const char* label, const char* key, tree opts,
            const char* value, const char* live) {
  return compound (string (type), tree (label), tree (key), opts, tree (value),
                   tree (live));
}

class TestQmlDialog : public QObject {
  Q_OBJECT

private slots:
  void init () { init_lolly (); }

  void test_field_indices ();
  void test_field_valid_shape ();
  void test_field_key_value_passthrough ();
  void test_field_tree_to_qml_enum ();
  void test_field_tree_to_qml_input ();
  void test_field_tree_to_qml_live_flag ();
  void test_field_tree_to_qml_options_not_compound ();
  void test_field_tree_to_qml_malformed ();
};

// 字段下标协议常量与协议文档一致。
void
TestQmlDialog::test_field_indices () {
  QCOMPARE (FIELD_LABEL, 0);
  QCOMPARE (FIELD_KEY, 1);
  QCOMPARE (FIELD_OPTIONS, 2);
  QCOMPARE (FIELD_VALUE, 3);
  QCOMPARE (FIELD_LIVE, 4);
}

// field_valid 仅接受 compound 且 arity 至少到 value 位置（>3）。
void
TestQmlDialog::test_field_valid_shape () {
  tree opts= compound ("tuple", tree ("default"), tree ("ggv"));
  QVERIFY (field_valid (make_field ("enum", "L", "k", opts, "v")));
  // 原子节点非法。
  QVERIFY (!field_valid (tree ("atomic")));
  // arity 不足（缺 value）非法。
  tree short_node= compound ("enum", tree ("L"), tree ("k"), opts);
  QVERIFY (!field_valid (short_node));
}

// field_key / field_value 纯透传子树（不拷贝、不翻译）。
void
TestQmlDialog::test_field_key_value_passthrough () {
  tree opts= compound ("tuple", tree ("default"));
  tree f   = make_field ("enum", "Preview command:", "preview command", opts,
                         "default");
  QCOMPARE (field_key (f)->label, string ("preview command"));
  QCOMPARE (field_value (f)->label, string ("default"));
}

// enum：完整字段表，options 全部透传成 QVariantList。
void
TestQmlDialog::test_field_tree_to_qml_enum () {
  tree opts= compound ("tuple", tree ("default"), tree ("ggv"), tree ("gv"));
  tree f   = make_field ("enum", "Preview command:", "preview command", opts,
                         "default");
  QVariantMap m= field_tree_to_qml (f);
  QCOMPARE (m.value ("type").toString (), QStringLiteral ("enum"));
  QCOMPARE (m.value ("label").toString (), QStringLiteral ("Preview command:"));
  QCOMPARE (m.value ("key").toString (), QStringLiteral ("preview command"));
  QCOMPARE (m.value ("value").toString (), QStringLiteral ("default"));
  QCOMPARE (m.value ("live").toBool (), false);
  const auto optsList= m.value ("options").toList ();
  QCOMPARE (optsList.size (), 3);
  QCOMPARE (optsList[0].toString (), QStringLiteral ("default"));
  QCOMPARE (optsList[2].toString (), QStringLiteral ("gv"));
}

// 控件类型字段透传：type 取自节点 label，与具体控件名无关（input/checkbox 等
// 尚未定稿的控件只要按统一 5 位协议构造，type 即可正确透传）。
void
TestQmlDialog::test_field_tree_to_qml_input () {
  // (input "Label" "key" "opts-placeholder" "current") —— options
  // 位放原子占位， is_compound 为假 → options 字段不写入 map，其余字段正常。
  tree        f= compound (string ("input"), tree ("Label"), tree ("key"),
                           tree ("not-a-list"), tree ("current"));
  QVariantMap m= field_tree_to_qml (f);
  QCOMPARE (m.value ("type").toString (), QStringLiteral ("input"));
  QCOMPARE (m.value ("label").toString (), QStringLiteral ("Label"));
  QCOMPARE (m.value ("key").toString (), QStringLiteral ("key"));
  QCOMPARE (m.value ("value").toString (), QStringLiteral ("current"));
  QVERIFY (!m.contains ("options"));
}

// live=true / 省略 / false 三态。
void
TestQmlDialog::test_field_tree_to_qml_live_flag () {
  tree opts= compound ("tuple", tree ("a"));
  // live = "true"
  {
    QVariantMap m=
        field_tree_to_qml (make_field ("enum", "L", "k", opts, "v", "true"));
    QVERIFY (m.value ("live").toBool ());
  }
  // live = "false" → false
  {
    QVariantMap m=
        field_tree_to_qml (make_field ("enum", "L", "k", opts, "v", "false"));
    QVERIFY (!m.value ("live").toBool ());
  }
  // 省略 live → false（默认）
  {
    QVariantMap m= field_tree_to_qml (make_field ("enum", "L", "k", opts, "v"));
    QVERIFY (!m.value ("live").toBool ());
  }
}

// options 非 compound（如被错放成原子）时，options 字段应被跳过，不崩溃。
void
TestQmlDialog::test_field_tree_to_qml_options_not_compound () {
  tree        f= compound (string ("enum"), tree ("L"), tree ("k"),
                           tree ("not-a-list"), tree ("v"));
  QVariantMap m= field_tree_to_qml (f);
  QVERIFY (!m.contains ("options"));
  QCOMPARE (m.value ("value").toString (), QStringLiteral ("v"));
}

// 形状不符（原子、arity 不足）：返回空 map。
void
TestQmlDialog::test_field_tree_to_qml_malformed () {
  QVERIFY (field_tree_to_qml (tree ("atomic")).isEmpty ());
  tree opts= compound ("tuple", tree ("a"));
  QVERIFY (field_tree_to_qml (compound ("enum", tree ("L"), tree ("k"), opts))
               .isEmpty ());
}

#ifdef QTTEXMACS
QTEST_MAIN (TestQmlDialog)
#else
// 非 Qt 构建也能编译（target 仅在 Qt 平台启用测试）。
int
main () {
  return 0;
}
#endif
#include "qml_dialog_test.moc"
