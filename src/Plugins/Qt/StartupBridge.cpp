
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
#include "qt_dpi_utils.hpp"
#include "qt_floating_toast.hpp"
#include "qt_pdf_preview_widget.hpp"
#include "qt_utilities.hpp"
#include "s7_tm.hpp"
#include "sys_utils.hpp"
#include "template_manager.hpp"

#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

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
  QJsonArray  files= obj["files"].toArray ();
  for (const auto& val : files) {
    QJsonObject o= val.toObject ();
    QString     p= QDir::fromNativeSeparators (o["path"].toString ());
    if (p.isEmpty ()) continue;
    QString name= o["name"].toString ();
    qint64  ts  = static_cast<qint64> (
        o.contains ("last_open") ? o["last_open"].toDouble () : 0);
    QDateTime dt= ts > 0 ? QDateTime::fromSecsSinceEpoch (ts)
                         : QDateTime::currentDateTime ();
    meta.insert (p, {name, dt.toString ("yyyy-MM-dd HH:mm")});
  }
  return meta;
}

QVariantMap
makeRecentEntry (const QString& fileName, const QString& filePath,
                 const QString& openedAt) {
  QDateTime dt= QDateTime::fromString (openedAt, "yyyy-MM-dd HH:mm");
  double    ts= dt.isValid ()
                    ? static_cast<double> (dt.toSecsSinceEpoch ())
                    : static_cast<double> (
                       QDateTime::currentDateTime ().toSecsSinceEpoch ());
  return {{"fileName", fileName},
          {"filePath", filePath},
          {"openedAt", openedAt},
          {"timestamp", ts}};
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

QString
StartupBridge::thumbnailCachePath () const {
  string cacheDir= get_env ("TEXMACS_HOME_PATH") * "/system/thumbnails";
  QDir ().mkpath (to_qstring (cacheDir));
  return to_qstring (cacheDir);
}

/** 确保远程缩略图已下载到本地缓存，返回本地 file:// 路径。 */
static QString
localThumbnailUrl (const QString& remoteUrl, const QString& cacheDir,
                   QHash<QString, QString>& cache,
                   QNetworkAccessManager*   networkManager) {
  if (remoteUrl.isEmpty ()) return remoteUrl;

  // 已是本地文件，直接返回
  if (remoteUrl.startsWith ("qrc:/") || remoteUrl.startsWith ("file://") ||
      remoteUrl.startsWith ("/") || remoteUrl.startsWith (":"))
    return remoteUrl;

  // 已缓存
  auto it= cache.constFind (remoteUrl);
  if (it != cache.constEnd ()) return *it;

  // 生成本地文件名: thumbnails/<md5>.png
  QByteArray hash=
      QCryptographicHash::hash (remoteUrl.toUtf8 (), QCryptographicHash::Md5);
  QString localPath= QDir (cacheDir).filePath (hash.toHex () + ".png");

  // 本地文件已存在，直接记录映射
  if (QFile::exists (localPath)) {
    cache.insert (remoteUrl, "file://" + localPath);
    return "file://" + localPath;
  }

  // 触发异步下载，先返回远程 URL
  cache.insert (remoteUrl, localPath); // 标记为正在下载
  QNetworkRequest req (remoteUrl);
  req.setAttribute (QNetworkRequest::RedirectPolicyAttribute,
                    QNetworkRequest::NoLessSafeRedirectPolicy);
  networkManager->get (req);
  return remoteUrl;
}

void
StartupBridge::ensureThumbnailsLocal () {
  QString cacheDir= thumbnailCachePath ();
  for (int i= 0; i < categoryTemplates_.size (); ++i) {
    QVariantMap m    = categoryTemplates_[i].toMap ();
    QString     url  = m["thumbnailUrl"].toString ();
    QString     local= localThumbnailUrl (url, cacheDir, thumbnailLocalCache_,
                                          networkManager_);
    if (local != url) {
      m["thumbnailUrl"]    = local;
      categoryTemplates_[i]= m;
    }
  }
}

void
StartupBridge::onThumbnailDownloaded (QNetworkReply* reply) {
  reply->deleteLater ();
  if (reply->error () != QNetworkReply::NoError) return;

  QString remoteUrl= reply->request ().url ().toString ();
  auto    it       = thumbnailLocalCache_.constFind (remoteUrl);
  if (it == thumbnailLocalCache_.constEnd ()) return;

  // 已是 file:// URL（之前下载过），跳过
  if (it->startsWith ("file://")) return;

  QString localPath= *it;
  QFile   file (localPath);
  if (!file.open (QIODevice::WriteOnly)) {
    thumbnailLocalCache_.remove (remoteUrl);
    return;
  }
  file.write (reply->readAll ());
  file.close ();

  // 更新映射：remoteUrl → file://localPath
  QString fileUrl= "file://" + localPath;
  thumbnailLocalCache_.insert (remoteUrl, fileUrl);

  // 更新当前 categoryTemplates_ 中对应条目的 thumbnailUrl
  bool changed= false;
  for (int i= 0; i < categoryTemplates_.size (); ++i) {
    QVariantMap m  = categoryTemplates_[i].toMap ();
    QString     url= m["thumbnailUrl"].toString ();
    if (url == remoteUrl) {
      m["thumbnailUrl"]    = fileUrl;
      categoryTemplates_[i]= m;
      changed              = true;
    }
  }
  if (changed) emit categoryTemplatesChanged ();
}

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
  connect (templateManager_, &TemplateManager::recommendTemplatesLoadFailed,
           this, &StartupBridge::onRecommendTemplatesLoadFailed,
           Qt::UniqueConnection);
  connect (templateManager_, &TemplateManager::templatesLoadFailed, this,
           &StartupBridge::onTemplatesLoadFailed, Qt::UniqueConnection);

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

void
StartupBridge::onRecommendTemplatesLoadFailed (const QString& error) {
  qWarning () << "[StartupBridge] Failed to load recommend templates:" << error;
}

void
StartupBridge::onTemplatesLoadFailed (const QString& error) {
  qWarning () << "[StartupBridge] Failed to load templates:" << error;
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
    auto    it= meta.constFind (path);
    if (it != meta.constEnd ()) {
      if (!it->first.isEmpty ()) name= it->first;
      time= it->second;
    }
    else {
      time= QDateTime::currentDateTime ().toString ("yyyy-MM-dd HH:mm");
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
    auto   m = v.toMap ();
    double ts= m.contains ("timestamp") ? m["timestamp"].toDouble () : now;
    arr << QJsonObject{{"path", m["filePath"].toString ()},
                       {"name", m["fileName"].toString ()},
                       {"last_open", ts},
                       {"show", true}};
  }
  root["files"]= arr;

  // Write to temp file then rename for atomicity
  QString tmpPath= recentDocsFilePath () + ".tmp";
  QFile   tmpFile (tmpPath);
  if (tmpFile.open (QIODevice::WriteOnly | QIODevice::Truncate)) {
    tmpFile.write (QJsonDocument (root).toJson ());
    tmpFile.close ();
    QString destPath= recentDocsFilePath ();
    // 跨文件系统 rename 可能失败，先用 remove 清理目标再 rename
    if (!QFile::remove (destPath) && QFile::exists (destPath))
      qWarning () << "[StartupBridge] Failed to remove old recent-files.json";
    if (!tmpFile.rename (destPath))
      qWarning () << "[StartupBridge] Failed to rename temp file to"
                  << destPath;
  }
  else {
    qWarning () << "[StartupBridge] Failed to open temp file for writing:"
                << tmpPath;
  }
}

void
StartupBridge::refreshRecentDocs () {
  loadRecentDocs ();
}

void
StartupBridge::scheduleRecentDocsRefresh () {
  QTimer::singleShot (1500, this, [this] { loadRecentDocs (); });
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
      QDateTime::currentDateTime ().toString ("yyyy-MM-dd HH:mm")));
  while (recentDocs_.size () > kMaxRecentDocs)
    recentDocs_.removeLast ();

  saveRecentDocs ();
  QTimer::singleShot (0, this, [normPath] {
    eval_scheme ("(startup-tab-add-recent-doc " *
                 qt_scheme_quote_utf8 (normPath) * ")");
  });
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

  // 命中缓存：直接复用，避免不必要的 clear+重建导致 QML Image 重新加载
  auto cacheIt= categoryTemplatesCache_.constFind (activeCategoryId_);
  if (cacheIt != categoryTemplatesCache_.constEnd ()) {
    if (categoryTemplates_ != *cacheIt) {
      categoryTemplates_= *cacheIt;
      emit categoryTemplatesChanged ();
    }
    // 确保已缓存的模板缩略图都转为了本地 file:// URL
    ensureThumbnailsLocal ();
    return;
  }

  // 缓存未命中：从 TemplateManager 构建并缓存
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
  // 启动缩略图下载（异步），下载完成后自动更新为 file:// URL
  ensureThumbnailsLocal ();
  categoryTemplatesCache_.insert (activeCategoryId_, categoryTemplates_);
  emit categoryTemplatesChanged ();
}

void
StartupBridge::onTemplatesLoaded () {
  categoryLoading_= false;
  emit categoryLoadingChanged ();
  // 异步新数据到达，清除缓存强制重新构建
  categoryTemplatesCache_.remove (activeCategoryId_);
  refreshCategoryTemplates ();
}

// =========================================================================
// QML 工具方法
// =========================================================================

QString
StartupBridge::tr (const QString& text) const {
  return qt_translate (from_qstring (text));
}

// =========================================================================
// QML actions
// =========================================================================

void
StartupBridge::newDocument () {
  eval_scheme ("(new-document-with-style \"generic\")");
  scheduleRecentDocsRefresh ();
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
  scheduleRecentDocsRefresh ();
}

void
StartupBridge::openRecentDoc (const QString& path) {
  if (path.isEmpty ()) return;
  if (!QFile::exists (path)) {
    removeRecentDoc (path);
    QWidget* pw= qobject_cast<QWidget*> (parent ());
    QtFloatingToast::showToast (
        pw, qt_translate ("File not found, removed from recent list"), 3000,
        QtFloatingToast::Error);
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
  QTimer::singleShot (0, this, [normPath] {
    eval_scheme ("(startup-tab-clear-recent-doc " *
                 qt_scheme_quote_utf8 (normPath) * ")");
  });
  emit recentDocsChanged ();
}

void
StartupBridge::clearAllRecentDocs () {
  QTimer::singleShot (0, this,
                      [] { eval_scheme ("(startup-tab-clear-all-recent)"); });
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
  if (templateManager_ && templateManager_->isInitialized ()) {
    categoryLoading_= true;
    emit categoryLoadingChanged ();
    templateManager_->refreshTemplatesByCategory (categoryId);
    // 同步刷新：处理已缓存分类（refreshTemplatesByCategory 对已缓存分类
    // 是 no-op，不会触发 onTemplatesLoaded），同时为未缓存分类展示已有数据
    refreshCategoryTemplates ();
    // 若数据已就绪（缓存命中），同步结束 loading；否则等待 onTemplatesLoaded
    if (!categoryTemplates_.isEmpty ()) {
      categoryLoading_= false;
      emit categoryLoadingChanged ();
    }
  }
  else {
    // 未初始化时无异步回调，直接同步刷新
    refreshCategoryTemplates ();
  }
}

void
StartupBridge::openTemplate (const QString& id) {
  QTMTemplateOpener opener;
  opener.openTemplate (id);
  scheduleRecentDocsRefresh ();
}

void
StartupBridge::previewTemplate (const QString& id) {
  if (!templateManager_) return;

  TemplateMetadataPtr tmpl= templateManager_->templateById (id);
  if (!tmpl) return;

  QWidget* parentWidget= qobject_cast<QWidget*> (parent ());

  // ---- 预览对话框 ----
  // TODO: PDF 预览框后续改为 QML 实现
  QDialog* dialog= new QDialog (parentWidget);
  dialog->setWindowTitle (
      qt_translate ("Template Preview - %1").arg (tmpl->name));
  dialog->setAttribute (Qt::WA_DeleteOnClose);

  // 根据屏幕可用区域限制尺寸
  QScreen* screen= dialog->screen ();
  if (!screen) screen= QGuiApplication::primaryScreen ();
  QRect availGeo= screen ? screen->availableGeometry () : QRect ();

  constexpr int kPreviewImageSize= 600;
  constexpr int kLayoutMargin    = 24;
  constexpr int kLayoutSpacing   = 16;
  constexpr int kTitleFontPx     = 18;
  constexpr int kDescFontPx      = 14;
  constexpr int kBtnFontPx       = 13;

  int previewSize= DpiUtils::scaled (kPreviewImageSize);
  int maxDlgH    = availGeo.height () > 0 ? qRound (availGeo.height () * 0.9)
                                          : DpiUtils::scaled (800);
  previewSize    = qMin (previewSize, qRound (maxDlgH * 0.7));

  QVBoxLayout* layout= new QVBoxLayout (dialog);
  layout->setSpacing (DpiUtils::scaled (kLayoutSpacing));
  layout->setContentsMargins (
      DpiUtils::scaled (kLayoutMargin), DpiUtils::scaled (kLayoutMargin),
      DpiUtils::scaled (kLayoutMargin), DpiUtils::scaled (kLayoutMargin));

  // 标题
  QLabel* titleLabel= new QLabel (tmpl->name, dialog);
  QFont   titleFont = DpiUtils::scaledFont (titleLabel->font (), kTitleFontPx);
  titleFont.setBold (true);
  titleLabel->setFont (titleFont);
  layout->addWidget (titleLabel);

  // 描述
  if (!tmpl->description.isEmpty ()) {
    QLabel* descLabel= new QLabel (tmpl->description, dialog);
    descLabel->setWordWrap (true);
    DpiUtils::applyScaledFont (descLabel, kDescFontPx);
    layout->addWidget (descLabel);
  }

  // 信息行
  QHBoxLayout* infoLayout= new QHBoxLayout ();
  if (!tmpl->author.isEmpty ())
    infoLayout->addWidget (
        new QLabel (qt_translate ("Author: %1").arg (tmpl->author)));
  if (!tmpl->version.isEmpty ())
    infoLayout->addWidget (
        new QLabel (qt_translate ("Version: %1").arg (tmpl->version)));
  infoLayout->addStretch ();
  layout->addLayout (infoLayout);

  // PDF 预览
  QTPdfPreviewWidget* previewWidget= new QTPdfPreviewWidget (dialog);
  previewWidget->setFixedSize (previewSize, previewSize);
  if (!tmpl->previewUrl.isEmpty ())
    previewWidget->loadFromUrl (tmpl->previewUrl);
  else previewWidget->clearPreview (qt_translate ("No Preview"));
  layout->addWidget (previewWidget, 0, Qt::AlignCenter);

  // 按钮
  QHBoxLayout* btnLayout= new QHBoxLayout ();
  btnLayout->addStretch ();

  QPushButton* cancelBtn= new QPushButton (qt_translate ("Cancel"), dialog);
  cancelBtn->setObjectName ("template-cancel-btn");
  DpiUtils::applyScaledFont (cancelBtn, kBtnFontPx);
  cancelBtn->setCursor (Qt::PointingHandCursor);
  connect (cancelBtn, &QPushButton::clicked, dialog, &QDialog::reject);
  btnLayout->addWidget (cancelBtn);

  QPushButton* useBtn= new QPushButton (qt_translate ("Use Template"), dialog);
  useBtn->setObjectName ("template-use-btn");
  DpiUtils::applyScaledFont (useBtn, kBtnFontPx);
  useBtn->setCursor (Qt::PointingHandCursor);
  useBtn->setDefault (true);
  connect (useBtn, &QPushButton::clicked, [dialog, id] () {
    dialog->accept ();
    QTMTemplateOpener opener;
    opener.openTemplate (id);
  });
  btnLayout->addWidget (useBtn);
  layout->addLayout (btnLayout);

  dialog->show ();
}

void
StartupBridge::quit () {
  eval_scheme ("(quit-TeXmacs)");
}
