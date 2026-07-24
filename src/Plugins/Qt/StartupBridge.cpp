
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

// =========================================================================
// Helpers
// =========================================================================

static QString
recentDocsFilePath () {
  string  homePath = get_env ("TEXMACS_HOME_PATH");
  QString configDir= to_qstring (homePath * "/system");
  QDir ().mkpath (configDir);
  return QDir (configDir).filePath ("recent-files.json");
}

static QStringList
recentPathsFromScheme () {
  QStringList paths;
  tmscm       result= eval_scheme ("(startup-tab-get-recent-docs)");
  if (!tmscm_is_list (result)) return paths;

  for (tmscm cur= result; !tmscm_is_null (cur); cur= tmscm_cdr (cur)) {
    tmscm item= tmscm_car (cur);
    if (!tmscm_is_string (item)) continue;
    paths.append (QString::fromUtf8 (as_charp (tmscm_to_string (item))));
  }
  return paths;
}

// =========================================================================
// StartupBridge
// =========================================================================

StartupBridge::StartupBridge (QObject* parent) : QObject (parent) {}

StartupBridge::~StartupBridge ()= default;

void
StartupBridge::initialize () {
  // Ensure Scheme module is loaded
  eval_scheme ("(use-modules (startup-tab startup-tab-file))");

  // Fixed style cards
  rebuildStyleCards ();

  // Load recent docs
  loadRecentDocs ();

  // Connect to TemplateManager
  connectTemplateManager ();
}

void
StartupBridge::connectTemplateManager () {
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
    QTimer::singleShot (0, this,
                        [this] () { templateManager_->initialize (); });
  }
}

// =========================================================================
// Style cards
// =========================================================================

QVariantMap
StartupBridge::makeStyleCard (const QString& kind, const QString& id,
                              const QString& name, const QString& titleText,
                              const QString& iconSrc, const QString& thumbSrc) {
  QVariantMap m;
  m["kind"]= kind;
  m["id"]  = id;
  m["name"]= name;
  if (!titleText.isEmpty ()) m["titleText"]= titleText;
  if (!iconSrc.isEmpty ()) m["iconSrc"]= iconSrc;
  if (!thumbSrc.isEmpty ()) m["thumbSrc"]= thumbSrc;
  return m;
}

void
StartupBridge::rebuildStyleCards () {
  styleCards_.clear ();

  // Fixed: "new" and "open" are built-in icon-mode cards in StartupHomePage.qml
  // Dynamic: recommended templates from TemplateManager

  TemplateManager* mgr= TemplateManager::instance ();
  if (mgr && mgr->isInitialized ()) {
    for (const auto& tmpl : mgr->recommendTemplates ()) {
      if (!tmpl) continue;
      styleCards_.append (makeStyleCard ("thumbnail", tmpl->id, tmpl->name,
                                         tmpl->name, QString (),
                                         tmpl->thumbnailUrl));
    }
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

QVariantMap
StartupBridge::makeRecentDoc (const QString& fileName, const QString& filePath,
                              const QString& openedAt) {
  QVariantMap m;
  m["fileName"]= fileName;
  m["filePath"]= filePath;
  m["openedAt"]= openedAt;
  return m;
}

void
StartupBridge::loadRecentDocs () {
  recentDocs_.clear ();

  QStringList paths= recentPathsFromScheme ();
  for (QString& path : paths)
    path= QDir::fromNativeSeparators (path);
  paths.removeDuplicates ();

  // Filter to existing files only
  QStringList existing;
  for (const QString& p : paths) {
    if (QFile::exists (p)) existing.append (p);
  }
  paths= existing;

  // Read metadata from JSON cache
  QHash<QString, QPair<QString, QString>> meta; // path → (name, openedAt)
  QFile                                   file (recentDocsFilePath ());
  if (file.open (QIODevice::ReadOnly)) {
    QJsonDocument doc= QJsonDocument::fromJson (file.readAll ());
    if (doc.isObject ()) {
      QJsonObject obj  = doc.object ();
      QJsonArray  files= obj.contains ("files")
                             ? obj["files"].toArray ()
                             : obj["recent_documents"].toArray ();
      for (const auto& val : files) {
        QJsonObject o   = val.toObject ();
        QString     p   = QDir::fromNativeSeparators (o["path"].toString ());
        QString     name= o["name"].toString ();
        qint64      ts  = static_cast<qint64> (
            o.contains ("last_open") ? o["last_open"].toDouble () : 0);
        QDateTime dt= ts > 0 ? QDateTime::fromSecsSinceEpoch (ts)
                             : QDateTime::currentDateTime ();
        if (!p.isEmpty ())
          meta.insert (p, {name, dt.toString ("yyyy-MM-dd hh:mm")});
      }
    }
    file.close ();
  }

  for (const QString& path : paths) {
    if (recentDocs_.size () >= kMaxRecentDocs) break;
    QString fileName= QFileInfo (path).fileName ();
    QString openedAt;
    if (meta.contains (path)) {
      auto& m = meta[path];
      fileName= m.first.isEmpty () ? fileName : m.first;
      openedAt= m.second;
    }
    else {
      openedAt= QDateTime::currentDateTime ().toString ("yyyy-MM-dd hh:mm");
    }
    recentDocs_.append (makeRecentDoc (fileName, path, openedAt));
  }

  emit recentDocsChanged ();
}

void
StartupBridge::saveRecentDocs () {
  QFile       file (recentDocsFilePath ());
  QJsonObject root;
  if (file.open (QIODevice::ReadOnly)) {
    QJsonDocument d= QJsonDocument::fromJson (file.readAll ());
    if (d.isObject ()) root= d.object ();
    file.close ();
  }
  if (root.isEmpty ()) {
    root["meta"] = QJsonObject{{"version", 1}, {"total", 0}};
    root["files"]= QJsonArray ();
  }

  QJsonArray files;
  for (const auto& doc : recentDocs_) {
    QJsonObject obj;
    obj["path"]= doc.toMap ()["filePath"].toString ();
    obj["name"]= doc.toMap ()["fileName"].toString ();
    obj["last_open"]=
        static_cast<double> (QDateTime::currentDateTime ().toSecsSinceEpoch ());
    obj["show"]= true;
    files.append (obj);
  }

  root["files"]= files;
  root["meta"] = QJsonObject{{"version", 1},
                             {"total", static_cast<int> (recentDocs_.size ())}};

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
  QString fileName= QFileInfo (normPath).fileName ();
  QString openedAt= QDateTime::currentDateTime ().toString ("yyyy-MM-dd hh:mm");

  // Remove duplicate if exists
  for (int i= 0; i < recentDocs_.size (); ++i) {
    if (recentDocs_[i].toMap()["filePath"].toString () == normPath) {
      recentDocs_.removeAt (i);
#ifdef LIII_DEBUG
      cout << "[StartupBridge] Moved existing recent doc to top: "
           << from_qstring (normPath) << "\n";
#endif
      break;
    }
  }

  recentDocs_.push_front (makeRecentDoc (fileName, normPath, openedAt));
  while (recentDocs_.size () > kMaxRecentDocs)
    recentDocs_.removeLast ();

  saveRecentDocs ();
  eval_scheme ("(startup-tab-add-recent-doc " *
               qt_scheme_quote_utf8 (normPath) * ")");
  emit recentDocsChanged ();
}

// =========================================================================
// Categories
// =========================================================================

void
StartupBridge::onCategoriesLoaded () {
  if (!templateManager_) return;

  categories_.clear ();
  for (const auto& cat : templateManager_->categories ()) {
    QVariantMap m;
    m["id"]  = cat.id;
    m["name"]= cat.name;
    categories_.append (m);
  }
  emit categoriesChanged ();
}

// =========================================================================
// Template page
// =========================================================================

QVariantMap
StartupBridge::makeTemplateItem (const QString& id, const QString& name,
                                 const QString& author, const QString& version,
                                 const QString& thumbnailUrl) {
  QVariantMap m;
  m["id"]          = id;
  m["name"]        = name;
  m["author"]      = author;
  m["version"]     = version;
  m["thumbnailUrl"]= thumbnailUrl;
  return m;
}

void
StartupBridge::refreshCategoryTemplates () {
  if (!templateManager_ || !templateManager_->isInitialized ()) return;

  categoryTemplates_.clear ();

  QList<TemplateMetadataPtr> templates;
  if (activeCategoryId_.isEmpty ()) {
    templates= templateManager_->templates ();
  }
  else {
    templates= templateManager_->templatesByCategory (activeCategoryId_);
  }

  for (const auto& tmpl : templates) {
    if (!tmpl) continue;
    categoryTemplates_.append (makeTemplateItem (
        tmpl->id, tmpl->name, tmpl->author, tmpl->version, tmpl->thumbnailUrl));
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
  const auto& docs= recentDocs_;
  if (!docs.isEmpty ()) {
    QString dir=
        QFileInfo (docs.first ().toMap ()["filePath"].toString ()).absolutePath ();
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
    if (recentDocs_[i].toMap()["filePath"].toString () == normPath) {
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

  // Look up display name
  activeCategoryName_.clear ();
  for (const auto& cat : categories_) {
    QVariantMap m= cat.toMap ();
    if (m["id"].toString () == categoryId) {
      activeCategoryName_= m["name"].toString ();
      break;
    }
  }

  emit activeCategoryChanged ();

  // Refresh templates for this category
  if (templateManager_ && templateManager_->isInitialized ()) {
    templateManager_->refreshTemplatesByCategory (categoryId);
  }
  refreshCategoryTemplates ();
}

void
StartupBridge::openTemplate (const QString& templateId) {
  QTMTemplateOpener opener;
  opener.openTemplate (templateId);
}

void
StartupBridge::previewTemplate (const QString& templateId) {
  // For now, open directly (same as openTemplate).
  // TODO: show preview dialog before opening.
  openTemplate (templateId);
}

void
StartupBridge::quit () {
  eval_scheme ("(quit-TeXmacs)");
}
