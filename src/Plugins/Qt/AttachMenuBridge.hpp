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
#include <QSet>
#include <QVariantList>
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
 *
 * meta 在 GoMenuBridge 同形键之外另带 attached / label_attached：已加入
 * 当前会话附件的文件从 recent 中过滤（仅本菜单的展示层，不回写全局最近
 * 文件列表），并列入「已添加」子菜单。
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

  /**
   * @brief 从 Recents 列表中过滤已加入会话附件的条目（纯函数）。
   * @param recent meta 中的 recent 列表（每项含 url/title 键）
   * @param attachedPaths 已添加附件的原文件路径集合
   * @return 过滤后的列表；attachedPaths 为空时原样返回
   */
  static QVariantList filterRecent (const QVariantList&  recent,
                                    const QSet<QString>& attachedPaths);

  /**
   * @brief 过滤附件菜单 Recents 中磁盘上已不存在的条目。
   * @param recent meta 中的 recent 列表（每项含 url/title 键）
   * @return 仅保留当前仍存在的本地文件
   * @note 仅用于附件菜单；标题栏 Go 菜单的失效条目清理由 scheme 侧负责
   */
  static QVariantList filterExistingRecent (const QVariantList& recent);

  Q_INVOKABLE void loadBuffer (const QString& url);
  Q_INVOKABLE void openFile ();
  Q_INVOKABLE void closeMenu ();
  Q_INVOKABLE void requestResize (qreal width, qreal height) {
    emit resizeRequested (width, height);
  }

signals:
  void resizeRequested (qreal width, qreal height);

private:
  QWidget*        m_host;
  string          m_sessionId;
  ChatController* m_controller;
  QVariantMap     m_meta;

  void loadMeta ();
};

#endif // ATTACH_MENU_BRIDGE_HPP
