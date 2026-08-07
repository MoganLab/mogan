/******************************************************************************
 * MODULE     : TmUpdaterController.cpp
 * DESCRIPTION: 更新器状态控制器实现（QTimer 轮询 tm_updater 单例）
 * COPYRIGHT  : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "TmUpdaterController.hpp"
#include "qt_utilities.hpp" // utf8_to_qstring

/**
 * @brief 构造：500ms 轮询 + 启动时立即 poll() 一次（拿初始状态）。
 */
TmUpdaterController::TmUpdaterController (QObject* parent)
    : QObject (parent), timer_ (this) {
  timer_.setInterval (500);
  connect (&timer_, &QTimer::timeout, this, &TmUpdaterController::poll);
  timer_.start ();
  poll ();
}

int
TmUpdaterController::state () const {
  return state_;
}

int
TmUpdaterController::progress () const {
  return progress_;
}

QString
TmUpdaterController::version () const {
  return version_;
}

QString
TmUpdaterController::releaseNotes () const {
  return releaseNotes_;
}

QString
TmUpdaterController::errorCode () const {
  return errorCode_;
}

/**
 * @brief 前台触发检查（工作线程内执行网络请求，本函数仅返回是否已启动）。
 */
bool
TmUpdaterController::checkNow () {
  tm_updater* u= tm_updater::instance ();
  return u ? u->checkInForeground () : false;
}

/**
 * @brief 触发下载（仅 AVAILABLE 状态可启动；未可用返回 #f）。
 */
bool
TmUpdaterController::download () {
  tm_updater* u= tm_updater::instance ();
  return u ? u->downloadUpdate () : false;
}

/**
 * @brief 触发应用（仅 READY 状态可启动；成功后进程退出并安装）。
 */
bool
TmUpdaterController::apply () {
  tm_updater* u= tm_updater::instance ();
  return u ? u->applyUpdate () : false;
}

/**
 * @brief 读 tm_updater 当前状态，值变化时才 emit 对应信号。
 * @details Velopack 版本/发行说明是 UTF-8 字节，用 utf8_to_qstring 转换
 *          （不是 cork_to_utf8——那是 scheme 侧已转 Cork 的文本）。
 */
void
TmUpdaterController::poll () {
  tm_updater* u= tm_updater::instance ();
  if (!u) return;

  int newState= u->state ();
  if (newState != state_) {
    state_= newState;
    emit stateChanged (state_);
  }

  int newProgress= u->progress ();
  if (newProgress != progress_) {
    progress_= newProgress;
    emit progressChanged (progress_);
  }

  QString newVersion= utf8_to_qstring (u->availableVersion ());
  if (newVersion != version_) {
    version_= newVersion;
    emit versionChanged (version_);
  }

  QString newNotes= utf8_to_qstring (u->releaseNotes ());
  if (newNotes != releaseNotes_) {
    releaseNotes_= newNotes;
    emit releaseNotesChanged (releaseNotes_);
  }

  QString newError= utf8_to_qstring (u->errorCode ());
  if (newError != errorCode_) {
    errorCode_= newError;
    emit errorCodeChanged (errorCode_);
  }
}

/**
 * @brief 全局单例：函数局部 static，首次调用创建并启动轮询。
 */
TmUpdaterController*
get_updater_controller () {
  static TmUpdaterController* c= new TmUpdaterController ();
  return c;
}
