/******************************************************************************
 * MODULE      : AttachMenuBridge.cpp
 * DESCRIPTION : 附件菜单 QML bridge 实现
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "AttachMenuBridge.hpp"

#include "qt_chat_controller.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"     // eval_scheme + tmscm helpers
#include "scheme.hpp"    // scheme_cmd + exec_delayed
#include "tm_server.hpp" // is_server_started

#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QTimer>
#include <QVariantList>

namespace {

QString
tmscm_to_qstring (tmscm v) {
  if (tmscm_is_string (v)) return to_qstring (tmscm_to_string (v));
  if (tmscm_is_symbol (v)) return to_qstring (tmscm_to_symbol (v));
  return QString ();
}

} // namespace

AttachMenuBridge::AttachMenuBridge (QWidget* host, const string& sessionId,
                                    ChatController* controller)
    : QObject (host), m_host (host), m_sessionId (sessionId),
      m_controller (controller) {
  loadMeta ();
}

AttachMenuBridge::~AttachMenuBridge ()= default;

void
AttachMenuBridge::loadMeta () {
  m_meta.clear ();
  // 默认兜底值
  m_meta["label_recent"]  = qt_translate ("Recent");
  m_meta["label_open"]    = qt_translate ("Choose file");
  m_meta["label_attached"]= qt_translate ("Attached");
  m_meta["recent"]        = QVariantList ();
  m_meta["attached"]      = QVariantList ();

  // 已加入当前会话附件的文件：从 Recents 过滤，同时列入「已添加」子菜单。
  // 匹配用原文件路径——文档类附件转换后 item.path 已指向临时 PDF
  QSet<QString> attachedPaths;
  QVariantList  attached;
  if (m_controller) {
    ChatSession* session=
        m_controller->sessionManager_.getSession (m_sessionId);
    if (session && session->panel) {
      ChatConversationPanel* panel=
          static_cast<ChatConversationPanel*> (session->panel);
      const QList<AttachmentItem>& items= panel->attachments ();
      for (const AttachmentItem& item : items) {
        QString src= item.sourcePath.isEmpty () ? item.path : item.sourcePath;
        if (!src.isEmpty ()) attachedPaths.insert (src);
        QVariantMap entry;
        entry["url"]  = src;
        entry["title"]= item.filename;
        attached.append (entry);
      }
    }
  }
  m_meta["attached"]= attached;

  if (!is_server_started ()) return;

  tmscm res= eval_scheme ("(go-menu-meta)");
  if (!tmscm_is_list (res)) return;

  for (tmscm cur= res; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm pair= tmscm_car (cur);
    if (!tmscm_is_pair (pair)) continue;
    QString key= tmscm_to_qstring (tmscm_car (pair));
    tmscm   val= tmscm_cdr (pair);

    if (key == "label_recent") {
      m_meta["label_recent"]= tmscm_to_qstring (val);
    }
    // 注意：go-menu-meta 中的 label_open 是 "Open file"，而 AttachMenu 需要的是
    // "Choose file"， 故此处不覆盖 label_open，保持 qt_translate ("Choose
    // file")。
    else if (key == "recent" && tmscm_is_list (val)) {
      QVariantList recs;
      for (tmscm rcur= val; !tmscm_is_null (rcur); rcur= tmscm_cdr (rcur)) {
        tmscm ritem= tmscm_car (rcur);
        if (tmscm_is_list (ritem)) {
          tmscm url_scm= tmscm_car (ritem);
          ritem        = tmscm_cdr (ritem);
          tmscm title_scm=
              tmscm_is_null (ritem) ? tmscm_null () : tmscm_car (ritem);

          QVariantMap item;
          item["url"]  = tmscm_to_qstring (url_scm);
          item["title"]= tmscm_to_qstring (title_scm);
          recs.append (item);
        }
      }
      m_meta["recent"]= recs;
    }
  }

  m_meta["recent"]= filterRecent (
      filterExistingRecent (m_meta["recent"].toList ()), attachedPaths);
}

QVariantList
AttachMenuBridge::filterExistingRecent (const QVariantList& recent) {
  QVariantList out;
  for (const QVariant& v : recent) {
    QString url= v.toMap ().value ("url").toString ();
    if (QFileInfo::exists (url)) out.append (v);
  }
  return out;
}

QVariantList
AttachMenuBridge::filterRecent (const QVariantList&  recent,
                                const QSet<QString>& attachedPaths) {
  if (attachedPaths.isEmpty ()) return recent;
  QVariantList out;
  for (const QVariant& v : recent) {
    QString url= v.toMap ().value ("url").toString ();
    if (!attachedPaths.contains (url)) out.append (v);
  }
  return out;
}

void
AttachMenuBridge::closeMenu () {
  if (m_host) {
    m_host->close ();
  }
}

void
AttachMenuBridge::openFile () {
  QWidget* parent= m_host ? m_host->parentWidget () : nullptr;
  if (!parent) parent= m_host;
  // 原生对话框会运行嵌套事件循环，返回前仅隐藏菜单，避免销毁仍在回调的 QML。
  if (m_host) m_host->hide ();

  QString     caption= qt_translate ("Add Attachment");
  QString     filter = qt_translate ("All files") + QString (" (*)");
  QStringList files;

  QString preset= qEnvironmentVariable ("MOGAN_TEST_CHOOSE_FILES");
  if (preset.isEmpty ()) preset= qEnvironmentVariable ("MOGAN_TEST_OPEN_FILE");
  if (preset == "event-loop-cancel") {
    QCoreApplication::processEvents ();
  }
  else if (!preset.isEmpty () && preset != "cancel") {
    files= preset.split (";", Qt::SkipEmptyParts);
  }
  else if (preset.isEmpty ()) {
    files= QFileDialog::getOpenFileNames (parent, caption, QString (), filter);
  }

  if (!files.isEmpty ()) {
    closeMenu ();
    auto* ctrl= m_controller ? m_controller : get_chat_controller ();
    if (ctrl) {
      string sid= m_sessionId;
      QTimer::singleShot (
          0, [ctrl, sid, files] () { ctrl->attachFiles (sid, files); });
    }
  }
  else {
    closeMenu ();
  }
}

void
AttachMenuBridge::loadBuffer (const QString& url) {
  closeMenu ();
  if (url.isEmpty ()) return;
  auto* ctrl= m_controller ? m_controller : get_chat_controller ();
  if (!ctrl) return;
  string sid= m_sessionId;
  QTimer::singleShot (0, [ctrl, sid, url] () {
    ctrl->attachFiles (sid, QStringList () << url);
  });
}
