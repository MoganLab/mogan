/******************************************************************************
 * MODULE      : GoMenuBridge.hpp
 * DESCRIPTION : Go 菜单 QML bridge 声明
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef GO_MENU_BRIDGE_HPP
#define GO_MENU_BRIDGE_HPP

#include <QObject>
#include <QVariantMap>
#include <QWidget>

/**
 * @brief Go 菜单的 C++ ↔ QML 桥接对象。
 *
 * 负责从 Scheme
 * 中获取当前光标历史状态、当前打开的文档列表、最近使用的文档列表， 并提供给 QML
 * 菜单。同时暴露跳转/回退/前进等槽函数供 QML 触发。
 */
class GoMenuBridge : public QObject {
  Q_OBJECT
  Q_PROPERTY (QVariantMap meta READ meta CONSTANT)

public:
  explicit GoMenuBridge (QWidget* host= nullptr);
  virtual ~GoMenuBridge ();

  QVariantMap meta () const { return m_meta; }

  Q_INVOKABLE void goBack ();
  Q_INVOKABLE void goForward ();
  Q_INVOKABLE void savePosition ();
  Q_INVOKABLE void switchToBuffer (const QString& url);
  Q_INVOKABLE void loadBuffer (const QString& url);
  Q_INVOKABLE void closeMenu ();

private:
  QWidget*    m_host;
  QVariantMap m_meta;

  void loadMeta ();
};

#endif // GO_MENU_BRIDGE_HPP
