
/******************************************************************************
 * MODULE     : StartupBridge.hpp
 * DESCRIPTION: C++↔QML bridge for startup tab
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
 * @brief C++↔QML 桥接，为 StartupTab.qml 提供数据模型与动作处理。
 *
 * 通过 Q_PROPERTY 暴露最近文档、模板分类、推荐模板等列表，
 * 通过 Q_INVOKABLE 提供新建/打开文档、切换分类、退出等动作。
 *
 * QVariantList 条目均为 QVariantMap，key 约定：
 *   recentDocs:  {fileName, filePath, openedAt}
 *   categories:  {id, name}
 *   styleCards:  {kind, id, name, titleText?, iconSrc?, thumbSrc?}
 *   templates:   {id, name, author, version, thumbnailUrl}
 */
class StartupBridge : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY (StartupBridge)

  Q_PROPERTY (
      QVariantList styleCards READ styleCards NOTIFY styleCardsChanged FINAL)
  Q_PROPERTY (
      QVariantList recentDocs READ recentDocs NOTIFY recentDocsChanged FINAL)
  Q_PROPERTY (
      QVariantList categories READ categories NOTIFY categoriesChanged FINAL)
  Q_PROPERTY (QString activeCategoryId READ activeCategoryId NOTIFY
                  activeCategoryChanged FINAL)
  Q_PROPERTY (QString activeCategoryName READ activeCategoryName NOTIFY
                  activeCategoryChanged FINAL)
  Q_PROPERTY (QVariantList categoryTemplates READ categoryTemplates NOTIFY
                  categoryTemplatesChanged FINAL)
  Q_PROPERTY (bool categoryLoading READ categoryLoading NOTIFY
                  categoryLoadingChanged FINAL)

public:
  explicit StartupBridge (QObject* parent= nullptr);
  ~StartupBridge ();

  void initialize ();

  QVariantList styleCards () const { return styleCards_; }
  QVariantList recentDocs () const { return recentDocs_; }
  QVariantList categories () const { return categories_; }
  QString      activeCategoryId () const { return activeCategoryId_; }
  QString      activeCategoryName () const { return activeCategoryName_; }
  QVariantList categoryTemplates () const { return categoryTemplates_; }
  bool         categoryLoading () const { return categoryLoading_; }

  // ---- QML 工具方法 ----
  Q_INVOKABLE QString tr (const QString& text) const;

  // ---- QML 可调用动作 ----
  Q_INVOKABLE void newDocument ();
  Q_INVOKABLE void openDocument ();
  Q_INVOKABLE void openRecentDoc (const QString& path);
  Q_INVOKABLE void removeRecentDoc (const QString& path);
  Q_INVOKABLE void clearAllRecentDocs ();
  Q_INVOKABLE void selectCategory (const QString& categoryId);
  Q_INVOKABLE void openTemplate (const QString& templateId);
  Q_INVOKABLE void previewTemplate (const QString& templateId);
  Q_INVOKABLE void quit ();

  void addRecentDoc (const QString& path);
  void refreshRecentDocs ();

signals:
  void styleCardsChanged ();
  void recentDocsChanged ();
  void categoriesChanged ();
  void activeCategoryChanged ();
  void categoryTemplatesChanged ();
  void categoryLoadingChanged ();

private slots:
  void onCategoriesLoaded ();
  void onRecommendTemplatesLoaded ();
  void onTemplatesLoaded ();
  void onRecommendTemplatesLoadFailed (const QString& error);
  void onTemplatesLoadFailed (const QString& error);

private:
  void loadRecentDocs ();
  void saveRecentDocs ();
  void rebuildStyleCards ();
  void refreshCategoryTemplates ();
  void scheduleRecentDocsRefresh ();

  TemplateManager* templateManager_= nullptr;

  QVariantList styleCards_;
  QVariantList recentDocs_;
  QVariantList categories_;
  QString      activeCategoryId_;
  QString      activeCategoryName_;
  QVariantList categoryTemplates_;
  bool         categoryLoading_= false;

  static const int kMaxRecentDocs= 50;
};

#endif
