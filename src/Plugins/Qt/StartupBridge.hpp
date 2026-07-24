
/******************************************************************************
 * MODULE     : StartupBridge.hpp
 * DESCRIPTION: C++↔QML bridge for startup tab — data models + action handlers
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef STARTUP_BRIDGE_HPP
#define STARTUP_BRIDGE_HPP

#include <QObject>
#include <QVariantList>

class TemplateManager;

/**
 * @brief C++↔QML bridge for the startup tab.
 *
 * Exposes data models (recent docs, categories, templates) as Q_PROPERTY
 * lists of QVariantMap, and provides Q_INVOKABLE action handlers for
 * user interactions (new/open document, open template, quit, etc.).
 *
 * Owned by QTMStartupTabWidget; injected as "startupBridge" context
 * property into the QQuickWidget hosting StartupTab.qml.
 */
class StartupBridge : public QObject {
  Q_OBJECT

  // ---- 首页数据 ----
  /** Style cards shown on home page (fixed "new"/"open" + recommended
   * templates). Each entry: {kind, id, name, titleText?, iconSrc?, thumbSrc?}
   */
  Q_PROPERTY (
      QVariantList styleCards READ styleCards NOTIFY styleCardsChanged FINAL)

  /** Recent documents. Each entry: {fileName, filePath, openedAt}. */
  Q_PROPERTY (
      QVariantList recentDocs READ recentDocs NOTIFY recentDocsChanged FINAL)

  // ---- 侧边栏数据 ----
  /** Template categories for sidebar navigation.
   *  Each entry: {id, name}. */
  Q_PROPERTY (
      QVariantList categories READ categories NOTIFY categoriesChanged FINAL)

  // ---- 模板页数据 ----
  /** Currently selected category id ("" = none / home). */
  Q_PROPERTY (QString activeCategoryId READ activeCategoryId NOTIFY
                  activeCategoryChanged FINAL)

  /** Currently selected category display name. */
  Q_PROPERTY (QString activeCategoryName READ activeCategoryName NOTIFY
                  activeCategoryChanged FINAL)

  /** Templates filtered by current category.
   *  Each entry: {id, name, author, version, thumbnailUrl, description}. */
  Q_PROPERTY (QVariantList categoryTemplates READ categoryTemplates NOTIFY
                  categoryTemplatesChanged FINAL)

public:
  explicit StartupBridge (QObject* parent= nullptr);
  ~StartupBridge ();

  void initialize ();

  // ---- Property accessors ----
  QVariantList styleCards () const { return styleCards_; }
  QVariantList recentDocs () const { return recentDocs_; }
  QVariantList categories () const { return categories_; }
  QString      activeCategoryId () const { return activeCategoryId_; }
  QString      activeCategoryName () const { return activeCategoryName_; }
  QVariantList categoryTemplates () const { return categoryTemplates_; }

  // ---- QML actions ----
  Q_INVOKABLE void newDocument ();
  Q_INVOKABLE void openDocument ();
  Q_INVOKABLE void openRecentDoc (const QString& path);
  Q_INVOKABLE void removeRecentDoc (const QString& path);
  Q_INVOKABLE void clearAllRecentDocs ();
  Q_INVOKABLE void selectCategory (const QString& categoryId);
  Q_INVOKABLE void openTemplate (const QString& templateId);
  Q_INVOKABLE void previewTemplate (const QString& templateId);
  Q_INVOKABLE void quit ();

  /** Called externally when a document is opened (adds to recent list). */
  void addRecentDoc (const QString& path);
  /** Called externally to refresh the recent docs list. */
  void refreshRecentDocs ();

signals:
  void styleCardsChanged ();
  void recentDocsChanged ();
  void categoriesChanged ();
  void activeCategoryChanged ();
  void categoryTemplatesChanged ();

private slots:
  void onCategoriesLoaded ();
  void onRecommendTemplatesLoaded ();
  void onTemplatesLoaded ();

private:
  void loadRecentDocs ();
  void saveRecentDocs ();
  void rebuildStyleCards ();
  void refreshCategoryTemplates ();
  void connectTemplateManager ();

  static QVariantMap makeStyleCard (const QString& kind, const QString& id,
                                    const QString& name,
                                    const QString& titleText= QString (),
                                    const QString& iconSrc  = QString (),
                                    const QString& thumbSrc = QString ());
  static QVariantMap makeRecentDoc (const QString& fileName,
                                    const QString& filePath,
                                    const QString& openedAt);
  static QVariantMap makeTemplateItem (const QString& id, const QString& name,
                                       const QString& author,
                                       const QString& version,
                                       const QString& thumbnailUrl);

  TemplateManager* templateManager_= nullptr;

  // Data
  QVariantList styleCards_;
  QVariantList recentDocs_;
  QVariantList categories_;
  QString      activeCategoryId_;
  QString      activeCategoryName_;
  QVariantList categoryTemplates_;

  static const int kMaxRecentDocs= 50;
};

#endif // STARTUP_BRIDGE_HPP
