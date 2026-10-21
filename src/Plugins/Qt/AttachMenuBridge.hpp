/******************************************************************************
 * MODULE      : AttachMenuBridge.hpp
 * DESCRIPTION : 附件菜单 QML bridge 声明
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef ATTACH_MENU_BRIDGE_HPP
#define ATTACH_MENU_BRIDGE_HPP

#include "string.hpp"

#include <QObject>
#include <QVariantMap>
#include <QWidget>

class ChatController;

/**
 * @brief 附件菜单的 C++ ↔ QML 桥接对象。
 *
 * 复用 GoMenu.qml：与 GoMenuBridge 同形（Q_INVOKABLE loadBuffer / openFile /
 * closeMenu + QVariantMap meta），由 QML 上下文属性 "goBridge" 注入。
 * loadBuffer(url) 把最近文档走 attachment-ingest；openFile() 走多选文件
 * 对话框；closeMenu() 关闭宿主浮层。QML 端无任何「附件」命名。
 */
class AttachMenuBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QVariantMap meta READ meta CONSTANT)

public:
  explicit AttachMenuBridge (QWidget*        host      = nullptr,
                             const string&   sessionId = "",
                             ChatController* controller= nullptr);
  virtual ~AttachMenuBridge ();

  QVariantMap meta () const { return m_meta; }

  Q_INVOKABLE void loadBuffer (const QString& url);
  Q_INVOKABLE void openFile ();
  Q_INVOKABLE void closeMenu ();

private:
  QWidget*        m_host;
  string          m_sessionId;
  ChatController* m_controller;
  QVariantMap     m_meta;

  void loadMeta ();
};

#endif // ATTACH_MENU_BRIDGE_HPP
