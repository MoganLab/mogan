/******************************************************************************
 * MODULE      : QTMQmlDialog.hpp
 * DESCRIPTION : 基于 QML 的模态对话框底座，经 QQuickWidget 嵌入 QWidget 体系。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file QTMQmlDialog.hpp
 *  @brief QML 模态对话框底座的头文件，含确认型与表单型两类弹窗的 glue 入口。
 *
 * @par 弹窗分类
 * 迁移到 QML 的弹窗按交互模型分两类：
 * - 确认型（display）：纯展示 + 按钮，无状态、一次性。数据流为
 *   scm 算好文案/按钮 → cpp 注入 QML → 返回按钮下标。
 *   参考入口：qt_show_qml_dialog() + cpp_confirm_close() + ConfirmClose.qml。
 * - 表单型（form）：含若干可编辑字段，需要双向数据绑定，走通用 form 引擎。
 *
 * @par 通用 form 引擎设计原则
 * - @b 数据模型由 scm 构造：字段定义、可选项、当前值、label 翻译文案全部在 scm
 *   算好打包传入；cpp / QML 不碰 i18n、不碰业务，只做框架与 UI。
 * - @b value 统一为 string：scm 构造字段表时即序列化（number→十进制、
 *   checkbox→"on"/"off"、color→"#rrggbb"），cpp / QML 全程只搬运 string。
 * - @b 逐字段可选实时回写（live 标志）：
 *   - live=true：用户改动走 QML → bridge → glue → scm setter，实时预览。
 *     @b 红线：setter 禁止任何模态操作（不弹对话框、不嵌套 exec()），否则破坏
 *     scheme continuation 栈。Cancel 回滚由调用方实现（font-selector
 * 走快照写回撤销， 普通 form 不做回滚）。
 *   - live=false（默认）：值暂存 QML，点 OK 随整表单返回 scm 统一提交，Cancel
 * 放弃。
 * - @b 控件类型：enum / input / checkbox / color /
 * number，按相同字段节点结构扩展。
 *
 * @par key 的维护位置（编译隔离的核心）
 * - 所有 preference key 全部定义在 scm 侧常量 module（pref-keys.scm，
 *   define-public 导出），字段表构造、preference 读写、live setter 全引用常量。
 * - cpp 对 key 字符串零硬编码、纯透传：从 tree 读出，OK 返回时原样带回。
 *   改 key / 加字段 / 调可选项只动 scm，永不重编译 cpp。
 *
 * @par 数据协议（scm → cpp）
 * scm 侧用 quasiquote 构造字段表（stree，scheme 列表）。glue 入参需 mogan
 * tree，
 * @b 不会自动转换 scheme pair，故调用方须 @c stree->tree 转换：
 * (cpp-form-dialog (stree->tree fields))。cpp 遍历 tree children 解析，不引入
 * JSON。字段节点结构（按 type 取不同形）：
 * @code
 *   (enum     <label> <key> (<option> ...) <value> <live?>)
 *   (input    <label> <key> <value> <live?>)
 *   (checkbox <label> <key> <value> <live?>)
 *   (color    <label> <key> <value> <live?>)
 *   (number   <label> <key> <value> <live?>)
 * @endcode
 * - @c label：已翻译的文案字符串
 * - @c key：引用 scm 常量的 preference 键名
 * - @c options：可选值列表（enum 专用；value 不在其中时 scm 侧会防御性插入）
 * - @c value：当前值（统一 string）
 * - @c live：是否实时回写（可省，默认 false）
 *
 * @par 弹窗尺寸
 * 确认型引擎固定尺寸；form 引擎字段数不固定，由 QML root 的
 * implicitWidth/implicitHeight 自报尺寸，cpp 读取后 setFixedSize（动态字段
 * 用 childrenRect 兜底，避免 Repeater 布局未完成时读到半成品尺寸）。
 *
 * @par OK 返回值（cpp → scm）
 * 用户点 OK，cpp 返回 mogan tree，tree->stree 后形如（value 均 string）：
 * @code
 *   (tuple (tuple <key> <value>) ...)
 * @endcode
 * scm 侧 tree->stree 转回 scheme 列表后，用 cadr/caddr 解构每个 kv
 * （mogan tree 非 scheme pair，不可直接 car/cadr）。Cancel / 关闭返回空
 * tree，cdr 得 ()，for-each 安全 no-op，scm 不写回。
 *
 * @par glue 注册
 * 新增弹窗按 cpp_confirm_close 的模式，两处改动：
 * - src/Scheme/Glue/glue_basic.lua：加 {scm_name, cpp_name, ret_type, arg_list}
 * - TeXmacs/progs/prog/glue-symbols.scm：在符号列表加 scm_name
 * tree 类型在 glue 中已被广泛支持（ret_type / arg_list 均可；注意 tree、
 * content、scheme_tree 三种别名的差异，混用会运行期 segfault）。
 *
 * @par TODO
 * - @b TODO 控件类型：input / checkbox / color / number。
 * - @b TODO live=true 实时回写链路（bridge 信号 → glue → scm setter；QML 侧需
 *   debounce throttle，避免 color picker / SpinBox 拖动时高频回调压垮主线程）。
 * - @b TODO QML 视觉骨架复用（第三个 QML 弹窗时抽 DialogBase.qml）。
 *
 * @note 完整设计文档见 record/qml/plan.md。本头文件不含 Qt 头，可安全被
 * 生成的 glue 代码 include；各 glue 入口声明附在下方，实现见 QTMQmlDialog.cpp。
 */

#ifndef QTM_QML_DIALOG_H
#define QTM_QML_DIALOG_H

#include "array.hpp"
#include "string.hpp"
#include "tree.hpp"

/**
 * @brief 确认型弹窗的通用 QML 模态对话框。
 * @param qml_url QML 文档的 qrc 路径（如 "qrc:/qml/ConfirmClose.qml"）。
 * @param message 已翻译的正文。
 * @param buttons 按钮文案 key（英文，cpp 侧 qt_translate 翻译）。buttons[0]
 *   为肯定按钮、buttons[last] 为取消按钮。
 * @return 用户点选的按钮下标（≥0）；取消 / 窗口 X / Esc / QML 加载失败返回 -1。
 *
 * context property 名（dialogMessage / dialogButtons / dpScale / isDark）见
 * QTMQmlDialog.cpp。exec() 阻塞，模态串行化连点请求。
 */
int qt_show_qml_dialog (string qml_url, string message, array<string> buttons);

// ---- 具体弹窗的 glue 入口 -------------------------------------------------

/**
 * @brief 「确认关闭」弹窗的 glue 入口。
 * @param message 已翻译的正文。
 * @param scratch 为真时肯定按钮显示「另存为」（scratch / tmfs buffer）。
 * @return "Save" / "Don't save" / "Cancel" 之一。
 * @note 测试钩子 MOGAN_TEST_CONFIRM_CLOSE 命中时直接返回，不弹窗。
 */
string cpp_confirm_close (string message, bool scratch);

/**
 * @brief 通用 form 弹窗引擎的 glue 入口。
 * @param fields scm 构造的字段表，@b 须经 stree->tree 转换（glue 不自动转
 * pair）。 结构见顶部 @par 数据协议：(form (enum <label> <key> (<opt>...)
 * <value> <live?>) ...)。
 * @return 用户点 OK 返回 mogan tree，tree->stree 后形如 (tuple (tuple <key>
 * <value>) ...) （value 均 string）；Cancel / 关闭返回空 tree。cpp 对 key/value
 * 纯透传。
 * @note 测试钩子 MOGAN_TEST_FORM_DIALOG=ok/cancel
 * 命中时不弹窗（供自动化测试）。
 */
tree cpp_form_dialog (tree fields);

/**
 * @brief 字体选择器 QML 对话框的 glue 入口。
 * @param specs_key scheme specs-registry 的 int 句柄
 *（font-selector-register-specs 返回值）。
 * @return 非阻塞 show 路径立即返回空 tree（对话框刚打开、尚无结论，scheme
 *调用方 不读返回值）；仅 ok 测试钩子（MOGAN_TEST_FONT_SELECTOR=ok）返回 `(tuple
 *"ok")` 作标记，供自动化区分 OK 走了 commit。Cancel / 关闭 / QML 加载失败返回空
 *tree。
 * @b 注意：与 cpp-form-dialog 不同，本入口 @b 不 携带字段写回值——实际字体写回在
 * 用户点 OK 时由 bridge 调 font-selector-commit 完成（live 路径，commit 已写
 *buffer），返回值仅测试用。
 * @note 测试钩子 MOGAN_TEST_FONT_SELECTOR=ok|cancel 命中时不弹窗。详见
 * record/qml/font-selector.md Phase 2。
 */
tree cpp_font_selector_dialog (int specs_key);

/**
 * @brief 段落格式 QML 对话框的 glue 入口（live 写回 + 快照撤销）。
 * @param specs_key scheme paragraph-specs-registry 的 int 句柄
 *（paragraph-format-register-specs 返回值）。
 * @return 非阻塞 show 路径立即返回空 tree；ok 测试钩子
 *（MOGAN_TEST_PARAGRAPH_FORMAT=ok）返回 `(tuple "ok")` 供自动化区分。
 * @details 每次 setPara 经 make-multi-line-with 实时写回文档，主窗口 live 重排
 * 段落；OK 落定，Cancel/重置走打开时快照写回撤销。详见
 * devel/2029.md 与 ai-docs/qml/README.md。
 */
tree cpp_paragraph_format_dialog (int specs_key);

#endif // defined QTM_QML_DIALOG_H
