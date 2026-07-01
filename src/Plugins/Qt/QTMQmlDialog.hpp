/******************************************************************************
 * MODULE      : QTMQmlDialog.hpp
 * DESCRIPTION : 基于 QML 的模态对话框底座，经 QQuickWidget 嵌入 QWidget 体系。
 *               从旧的「Qt widget 导出 scheme」弹窗机制逐步迁移到 QML 的通用
 *               入口。每个具体弹窗（如 confirm-close）有自己的 glue 入口函数，
 *               复用这里的 qt_show_qml_dialog 拼装出 QQuickWidget + 模态
 *               QDialog + exec() 的生命周期。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 *
 * 设计要点
 *   - qt_show_qml_dialog 是同步调用：内部跑一个无边框本地模态 QDialog，
 *     其中包一个加载了指定 QML 的 QQuickWidget，exec() 阻塞直到用户关闭，
 *     返回用户点选的按钮下标（或 -1 表示取消 / X / Esc）。exec() 的模态特性
 *     天然把连点请求串行化，从根上消除「连点重复弹窗」与「X 关闭后无法二次
 *     弹出」两类问题。
 *   - 正文与按钮文案均由调用方（通常 scheme 侧算好经 glue 传入）提供，
 *     i18n 继续走既有 translate() 机制。
 *   - 本头文件不含 Qt 头，可安全被生成的 glue 代码 include。
 *   - 各具体弹窗的 glue 入口（如 cpp_confirm_close）声明附在下方，实现见
 *     QTMQmlDialog.cpp；后续新增弹窗按同样模式追加。
 ******************************************************************************/

#ifndef QTM_QML_DIALOG_H
#define QTM_QML_DIALOG_H

#include "array.hpp"
#include "string.hpp"

// 通用 QML 模态对话框：加载 qml_url 指向的 QML 文档，把 message 作为正文、
// buttons 作为按钮文案注入（context property 名见 QTMQmlDialog.cpp），
// exec() 阻塞返回用户点选的按钮下标；点取消 / 窗口 X / Esc 返回 -1。
// buttons[0] 视为肯定按钮、buttons[last] 视为取消按钮。
// QML 加载失败时返回 -1，由调用方决定如何兜底。
int qt_show_qml_dialog (string qml_url, string message, array<string> buttons);

// ---- 具体弹窗的 glue 入口 -------------------------------------------------

// 「确认关闭」弹窗。返回 "Save" / "Don't save" / "Cancel" 之一。
// message 为已翻译好的正文；scratch 为真时肯定按钮显示「另存为」。
string cpp_confirm_close (string message, bool scratch);

#endif // defined QTM_QML_DIALOG_H
