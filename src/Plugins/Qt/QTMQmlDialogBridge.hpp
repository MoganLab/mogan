/******************************************************************************
 * MODULE      : QTMQmlDialogBridge.hpp
 * DESCRIPTION : QML 弹窗的宿主侧辅助：暴露给 QML 的桥对象（choose/submit/
 *               cancel 交互回流）与 ESC 兜底事件过滤器（QmlDialogEscFilter）。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef QTM_QML_DIALOG_BRIDGE_H
#define QTM_QML_DIALOG_BRIDGE_H

#include "boot.hpp"

#include <QDialog>
#include <QKeyEvent>
#include <QObject>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QString>
#include <QVariantMap>
#include <QWindow>

class QmlDialogBridge : public QObject {
  Q_OBJECT

public:
  /**
   * @brief 构造桥对象，绑定宿主 QDialog（用于 choose/submit 回流结束模态）。
   * @param host 生命期覆盖 exec() 的宿主；本类不挂其为 QObject parent —— bridge
   *        须跨越 host 存活（form 型 exec 返回后还要 results()），挂 parent 会
   *        随 host 析构被 Qt 自动 delete，造成调用方 use-after-free。所有权交由
   *        inject_common_context 的调用方（持有指针、用完 delete）。
   */
  explicit QmlDialogBridge (QDialog* host) : QObject (), m_host (host) {
    ASSERT (host != NULL, "QmlDialogBridge expects a valid QDialog host");
  }

  /**
   * @brief 确认型弹窗：以按钮下标结束宿主模态 QDialog。
   * @param index 按钮下标，>= 0 表示点按钮；< 0（如 choose(-1)）表示取消。
   */
  Q_INVOKABLE void choose (int index) { m_host->done (index); }

  /**
   * @brief 语义化取消：以 Rejected 结束宿主模态 QDialog。
   *
   * 与 choose(-1) 对调用方等价（form 型忽略退出码、靠 results() 判定；Cancel 时
   * results() 为空），但语义比魔法值 choose(-1) 清晰。choose(n>=0) 仍用于「选第
   * n 个按钮」（如 ConfirmClose 的 Save/Don't save/Cancel）。
   */
  Q_INVOKABLE void cancel () { m_host->done (QDialog::Rejected); }

  /**
   * @brief 拖动无边框窗口，委托给底层 QWindow 的系统级移动（startSystemMove）。
   *
   * 由 QML 背景区域的 DragHandler 在拖拽开始时调用。
   */
  Q_INVOKABLE void startMove () {
    if (m_host && m_host->windowHandle ()) {
      m_host->windowHandle ()->startSystemMove ();
    }
  }

  /**
   * @brief 表单型弹窗：暂存整张表单的最终值并以 Accepted 结束宿主 QDialog。
   * @param values {key: value, ...}，value 均为 string（见 QTMQmlDialog.hpp 的
   *   value 约定）。cpp 侧随后用 results() 取出，遍历构造返回 tree。
   */
  Q_INVOKABLE void submit (QVariantMap values) {
    m_results= values;
    m_host->done (QDialog::Accepted);
  }

  /**
   * @return submit() 暂存的结果；Cancel / 关闭时为空 map。
   */
  const QVariantMap& results () const { return m_results; }

  /**
   * @brief live=true 字段的实时回写：QML 控件 onChange 时 emit，cpp 在槽里走
   *        glue 调 scm setter。
   * @note TODO 首案不启用（全 live=false）。高频控件（color picker / SpinBox）
   *   需在 QML 侧 debounce，避免压垮主线程。
   */
signals:
  void fieldChanged (const QString& key, const QString& value);

private:
  QDialog*    m_host;
  QVariantMap m_results;
};

/*! @class QmlDialogEscFilter
 *  @brief ESC 被 QQuickWidget 静默吞掉时的兜底关闭过滤器（任务 0925）。
 *
 * @par 背景
 * QML 弹窗的 ESC 正常链路：DialogShell（focus:true）→ Keys.onEscapePressed →
 * closeBridge.cancel() → 宿主 done(Rejected)。该链路依赖 QML 场景存在
 * activeFocusItem：QQuickDeliveryAgent 只向 activeFocusItem 投递按键；场景无
 * 焦点项时按键事件不被投递、保持 accepted，也不再沿 QWidget 链传播给宿主
 * QDialog::reject()——ESC 被静默吞掉，弹窗无法关闭。
 *
 * @par 语义
 * 仅在「ESC 到达 QQuickWidget 且 QML 场景无 activeFocusItem」时 reject() 宿主
 * ——此刻 QML 链路必然不会处理，兜底不抢占任何弹窗自身的取消语义；其余情况
 * 一律放行。仅用于 run_qml_dialog（exec 引擎）：其弹窗的取消语义即 Rejected。
 * live 写回弹窗（run_modal_qml_dialog）的取消须走 scheme 快照撤销，不得用
 * 本过滤器兜底（done() 不触发 WA_DeleteOnClose，且会跳过快照恢复）。
 */
class QmlDialogEscFilter : public QObject {
public:
  /**
   * @brief 构造 ESC 兜底过滤器。
   * @param host 宿主 QDialog（兜底时 reject），生命期须覆盖过滤器。
   * @param view 内嵌的 QQuickWidget（读取其 QML 场景焦点态）。
   * @param parent QObject 父对象，过滤器随其析构。
   */
  QmlDialogEscFilter (QDialog* host, QQuickWidget* view, QObject* parent)
      : QObject (parent), m_host (host), m_view (view) {}

protected:
  bool eventFilter (QObject* obj, QEvent* ev) override {
    if (ev->type () == QEvent::KeyPress) {
      QKeyEvent* ke= static_cast<QKeyEvent*> (ev);
      if (ke->key () == Qt::Key_Escape && ke->modifiers () == Qt::NoModifier &&
          m_view->quickWindow () &&
          !m_view->quickWindow ()->activeFocusItem ()) {
        m_host->reject ();
        return true;
      }
    }
    return QObject::eventFilter (obj, ev);
  }

private:
  QDialog*      m_host;
  QQuickWidget* m_view;
};

#endif // defined QTM_QML_DIALOG_BRIDGE_H
