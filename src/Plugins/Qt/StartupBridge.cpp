
/******************************************************************************
 * MODULE     : StartupBridge.cpp
 * DESCRIPTION: C++↔QML bridge for startup tab — implementation
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "StartupBridge.hpp"

#include "QTMTemplateOpener.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"
#include "sys_utils.hpp"
#include "template_manager.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace {

QString
recentDocsFilePath () {
  string homePath = get_env ("TEXMACS_HOME_PATH");
  string configDir= homePath * "/system";
  QDir ().mkpath (to_qstring (configDir));
  return QDir (to_qstring (configDir)).filePath ("recent-files.json");
}

QStringList
recentPathsFromScheme () {
  QStringList paths;
  tmscm       result= eval_scheme ("(startup-tab-get-recent-docs)");
  if (!tmscm_is_list (result)) return paths;
  for (tmscm cur= result; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (tmscm_is_string (item))
      paths << QString::fromUtf8 (as_charp (tmscm_to_string (item)));
  }
  return paths;
}

/** 读取 recent-files.json 缓存：path → {name, openedAt}。 */
QHash<QString, QPair<QString, QString>>
readRecentMeta () {
  QHash<QString, QPair<QString, QString>> meta;
  QFile                                   file (recentDocsFilePath ());
  if (!file.open (QIODevice::ReadOnly)) return meta;

  QJsonDocument doc= QJsonDocument::fromJson (file.readAll ());
  file.close ();
  if (!doc.isObject ()) return meta;

  QJsonObject obj  = doc.object ();
  QJsonArray  files= obj.contains ("files") ? obj["files"].toArray ()
                                            : obj["recent_documents"].toArray ();
  for (const auto& val : files) {
    QJsonObject o= val.toObject ();
    QString     p= QDir::fromNativeSeparators (o["path"].toString ());
    if (p.isEmpty ()) continue;
    QString name= o["name"].toString ();
    qint64  ts  = static_cast<qint64> (
        o.contains ("last_open") ? o["last_open"].toDouble () : 0);
    QDateTime dt= ts > 0 ? QDateTime::fromSecsSinceEpoch (ts)
                         : QDateTime::currentDateTime ();
    meta.insert (p, {name, dt.toString ("yyyy-MM-dd hh:mm")});
  }
  return meta;
}

QVariantMap
makeRecentEntry (const QString& fileName, const QString& filePath,
                 const QString& openedAt) {
  return {
      {"fileName", fileName}, {"filePath", filePath}, {"openedAt", openedAt}};
}

QVariantMap
makeTemplateEntry (const QString& id, const QString& name,
                   const QString& author, const QString& version,
                   const QString& thumbnailUrl) {
  return {{"id", id},
          {"name", name},
          {"author", author},
          {"version", version},
          {"thumbnailUrl", thumbnailUrl}};
}

} // namespace

// =========================================================================
// StartupBridge
// =========================================================================

StartupBridge::StartupBridge (QObject* parent) : QObject (parent) {}
StartupBridge::~StartupBridge ()= default;

void
StartupBridge::initialize () {
  eval_scheme ("(use-modules (startup-tab startup-tab-file))");
  rebuildStyleCards ();
  loadRecentDocs ();

  templateManager_= TemplateManager::instance ();
  connect (templateManager_, &TemplateManager::categoriesLoaded, this,
           &StartupBridge::onCategoriesLoaded, Qt::UniqueConnection);
  connect (templateManager_, &TemplateManager::recommendTemplatesLoaded, this,
           &StartupBridge::onRecommendTemplatesLoaded, Qt::UniqueConnection);
  connect (templateManager_, &TemplateManager::templatesLoaded, this,
           &StartupBridge::onTemplatesLoaded, Qt::UniqueConnection);

  if (templateManager_->isInitialized ()) {
    if (!templateManager_->categories ().isEmpty ()) onCategoriesLoaded ();
    if (!templateManager_->templates ().isEmpty ()) onTemplatesLoaded ();
  }
  else {
    QTimer::singleShot (0, this, [this] { templateManager_->initialize (); });
  }
}

// =========================================================================
// Style cards — 首页推荐模板
// =========================================================================

void
StartupBridge::rebuildStyleCards () {
  styleCards_.clear ();
  auto* mgr= TemplateManager::instance ();
  if (!mgr || !mgr->isInitialized ()) return;
  for (const auto& t : mgr->recommendTemplates ()) {
    if (!t) continue;
    QVariantMap m;
    m["kind"]     = "thumbnail";
    m["id"]       = t->id;
    m["name"]     = t->name;
    m["titleText"]= t->name;
    if (!t->thumbnailUrl.isEmpty ()) m["thumbSrc"]= t->thumbnailUrl;
    styleCards_ << m;
  }
  emit styleCardsChanged ();
}

void
StartupBridge::onRecommendTemplatesLoaded () {
  rebuildStyleCards ();
}

// =========================================================================
// Recent documents
// =========================================================================

void
StartupBridge::loadRecentDocs () {
  recentDocs_.clear ();

  QStringList paths= recentPathsFromScheme ();
  for (auto& p : paths)
    p= QDir::fromNativeSeparators (p);
  paths.removeDuplicates ();

  // 只保留仍存在的文件
  paths.erase (
      std::remove_if (paths.begin (), paths.end (),
                      [] (const QString& p) { return !QFile::exists (p); }),
      paths.end ());

  auto meta= readRecentMeta ();
  for (const auto& path : paths) {
    if (recentDocs_.size () >= kMaxRecentDocs) break;
    QString name= QFileInfo (path).fileName ();
    QString time;
    if (meta.contains (path)) {
      auto& m= meta[path];
      if (!m.first.isEmpty ()) name= m.first;
      time= m.second;
    }
    else {
      time= QDateTime::currentDateTime ().toString ("yyyy-MM-dd hh:mm");
    }
    recentDocs_ << makeRecentEntry (name, path, time);
  }
  emit recentDocsChanged ();
}

void
StartupBridge::saveRecentDocs () {
  QJsonObject root;
  QFile       file (recentDocsFilePath ());
  if (file.open (QIODevice::ReadOnly)) {
    QJsonDocument d= QJsonDocument::fromJson (file.readAll ());
    if (d.isObject ()) root= d.object ();
    file.close ();
  }
  if (!root.contains ("files")) root["files"]= QJsonArray ();

  double now=
      static_cast<double> (QDateTime::currentDateTime ().toSecsSinceEpoch ());
  QJsonArray arr;
  for (const auto& v : recentDocs_) {
    auto m= v.toMap ();
    arr << QJsonObject{{"path", m["filePath"].toString ()},
                       {"name", m["fileName"].toString ()},
                       {"last_open", now},
                       {"show", true}};
  }
  root["files"]= arr;

  if (file.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    file.write (QJsonDocument (root).toJson ());
    file.close ();
  }
}

void
StartupBridge::refreshRecentDocs () {
  loadRecentDocs ();
}

void
StartupBridge::addRecentDoc (const QString& path) {
  QString normPath= QDir::fromNativeSeparators (path);

  for (int i= 0; i < recentDocs_.size (); ++i) {
    if (recentDocs_[i].toMap ()["filePath"].toString () == normPath) {
      recentDocs_.removeAt (i); // 去重：删旧，后面 push_front
      break;
    }
  }

  recentDocs_.push_front (makeRecentEntry (
      QFileInfo (normPath).fileName (), normPath,
      QDateTime::currentDateTime ().toString ("yyyy-MM-dd hh:mm")));
  while (recentDocs_.size () > kMaxRecentDocs)
    recentDocs_.removeLast ();

  saveRecentDocs ();
  eval_scheme ("(startup-tab-add-recent-doc " *
               qt_scheme_quote_utf8 (normPath) * ")");
  emit recentDocsChanged ();
}

// =========================================================================
// Categories — 侧边栏模板分类
// =========================================================================

void
StartupBridge::onCategoriesLoaded () {
  if (!templateManager_) return;
  categories_.clear ();
  for (const auto& c : templateManager_->categories ())
    categories_ << QVariantMap{{"id", c.id}, {"name", c.name}};
  emit categoriesChanged ();
}

// =========================================================================
// Template page
// =========================================================================

void
StartupBridge::refreshCategoryTemplates () {
  if (!templateManager_ || !templateManager_->isInitialized ()) return;
  categoryTemplates_.clear ();

  auto templates=
      activeCategoryId_.isEmpty ()
          ? templateManager_->templates ()
          : templateManager_->templatesByCategory (activeCategoryId_);
  for (const auto& t : templates) {
    if (!t) continue;
    categoryTemplates_ << makeTemplateEntry (t->id, t->name, t->author,
                                             t->version, t->thumbnailUrl);
  }
  emit categoryTemplatesChanged ();
}

void
StartupBridge::onTemplatesLoaded () {
  refreshCategoryTemplates ();
  rebuildStyleCards ();
}

// =========================================================================
// QML actions
// =========================================================================

void
StartupBridge::newDocument () {
  eval_scheme ("(new-document-with-style \"generic\")");
}

void
StartupBridge::openDocument () {
  if (!recentDocs_.isEmpty ()) {
    QString dir=
        QFileInfo (recentDocs_.first ().toMap ()["filePath"].toString ())
            .absolutePath ();
    eval_scheme ("(choose-file load-buffer \"Load file\" \"action_open\" \"\" "
                 "(system->url " *
                 qt_scheme_quote_utf8 (dir) * "))");
  }
  else {
    eval_scheme ("(open-document)");
  }
}

void
StartupBridge::openRecentDoc (const QString& path) {
  if (path.isEmpty ()) return;
  if (!QFile::exists (path)) {
    removeRecentDoc (path);
    return;
  }
  addRecentDoc (path);
  eval_scheme ("(load-document " * qt_scheme_quote_utf8 (path) * ")");
}

void
StartupBridge::removeRecentDoc (const QString& path) {
  QString normPath= QDir::fromNativeSeparators (path);
  for (int i= 0; i < recentDocs_.size (); ++i) {
    if (recentDocs_[i].toMap ()["filePath"].toString () == normPath) {
      recentDocs_.removeAt (i);
      break;
    }
  }
  saveRecentDocs ();
  eval_scheme ("(startup-tab-clear-recent-doc " *
               qt_scheme_quote_utf8 (normPath) * ")");
  emit recentDocsChanged ();
}

void
StartupBridge::clearAllRecentDocs () {
  eval_scheme ("(startup-tab-clear-all-recent)");
  recentDocs_.clear ();
  saveRecentDocs ();
  emit recentDocsChanged ();
}

void
StartupBridge::selectCategory (const QString& categoryId) {
  if (activeCategoryId_ == categoryId) return;
  activeCategoryId_= categoryId;
  activeCategoryName_.clear ();
  for (const auto& v : categories_) {
    auto m= v.toMap ();
    if (m["id"].toString () == categoryId) {
      activeCategoryName_= m["name"].toString ();
      break;
    }
  }
  emit activeCategoryChanged ();
  if (templateManager_ && templateManager_->isInitialized ())
    templateManager_->refreshTemplatesByCategory (categoryId);
  refreshCategoryTemplates ();
}

void
StartupBridge::openTemplate (const QString& id) {
  QTMTemplateOpener opener;
  opener.openTemplate (id);
}

void
StartupBridge::previewTemplate (const QString& id) {
  openTemplate (id); // TODO: 预览弹窗
}

void
StartupBridge::quit () {
  eval_scheme ("(quit-TeXmacs)");
}
