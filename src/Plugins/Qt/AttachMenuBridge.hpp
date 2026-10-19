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
 * 负责从 Scheme 中获取最近使用的文档列表提供给 QML 菜单，
 * 同时暴露选择最近文档、打开系统文件对话框及关闭菜单的槽函数供 QML 触发。
 */
class AttachMenuBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QVariantMap meta READ meta CONSTANT)

public:
  explicit AttachMenuBridge (QWidget* host= nullptr,
                             const string& sessionId= "",
                             ChatController* controller= nullptr);
  virtual ~AttachMenuBridge ();

  QVariantMap meta () const { return m_meta; }

  Q_INVOKABLE void attachFile (const QString& url);
  Q_INVOKABLE void chooseFiles ();
  Q_INVOKABLE void closeMenu ();

private:
  QWidget*        m_host;
  string          m_sessionId;
  ChatController* m_controller;
  QVariantMap     m_meta;

  void loadMeta ();
};

#endif // ATTACH_MENU_BRIDGE_HPP
