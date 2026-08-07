/******************************************************************************
 * MODULE     : TmUpdaterController.hpp
 * DESCRIPTION: 更新器状态控制器（QTimer 轮询 tm_updater 单例）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

/*! @file TmUpdaterController.hpp
 *  @brief 以 500ms QTimer 轮询 tm_updater 单例，把状态机/进度/版本/错误码暴露为
 *         Qt 属性（Q_PROPERTY），供 QML 绑定；提供 checkNow/download/apply 槽。
 *
 * @par 设计
 * - @b 轮询而非回调：tm_updater 的状态由 Velopack 工作线程更新（Task 3），
 *   控制器不订阅 C++ 回调，而是在 Qt 事件循环里周期 poll()，值变化时才 emit
 *   对应信号——避免高频重复 emit，QML 端只需响应变化。
 * - @b 编码：Velopack feed 的版本/发行说明是 UTF-8 字节，存于 project string；
 *   转 QString 用 utf8_to_qstring（NOT cork_to_utf8——那是 scheme 侧 Cork
 * 文本）。
 * - @b 网络：check/download/apply 都在 tm_velopack 工作线程内执行（Task 3），
 *   Scheme 与 Qt 侧只触发与读状态，不在主线程碰网络。
 */

#ifndef TM_UPDATER_CONTROLLER_HPP
#define TM_UPDATER_CONTROLLER_HPP

#include "Updater/tm_updater.hpp"

#include <QObject>
#include <QString>
#include <QTimer>

class TmUpdaterController : public QObject {
  Q_OBJECT
  Q_PROPERTY (int state READ state NOTIFY stateChanged)
  Q_PROPERTY (int progress READ progress NOTIFY progressChanged)
  Q_PROPERTY (QString version READ version NOTIFY versionChanged)
  Q_PROPERTY (QString releaseNotes READ releaseNotes NOTIFY releaseNotesChanged)
  Q_PROPERTY (QString errorCode READ errorCode NOTIFY errorCodeChanged)

public:
  explicit TmUpdaterController (QObject* parent= nullptr);
  int              state () const;
  int              progress () const;
  QString          version () const;
  QString          releaseNotes () const;
  QString          errorCode () const;
  Q_INVOKABLE bool checkNow (); // tm_updater::checkInForeground()
  Q_INVOKABLE bool download (); // tm_updater::downloadUpdate()
  Q_INVOKABLE bool apply ();    // tm_updater::applyUpdate()  (成功后进程退出)

signals:
  void stateChanged (int state);
  void progressChanged (int progress);
  void versionChanged (QString version);
  void releaseNotesChanged (QString notes);
  void errorCodeChanged (QString error);

private:
  void    poll ();
  QTimer  timer_;
  int     state_   = 0;
  int     progress_= 0;
  QString version_;
  QString releaseNotes_;
  QString errorCode_;
};

/*!
 * @brief 获取全局 TmUpdaterController 单例（QML context 属性用）。
 */
TmUpdaterController* get_updater_controller ();

#endif // TM_UPDATER_CONTROLLER_HPP
