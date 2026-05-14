
/******************************************************************************
 * MODULE     : QTMTemplatePage.hpp
 * DESCRIPTION: Template page widget for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QTMTEMPLATEPAGE_HPP
#define QTMTEMPLATEPAGE_HPP

#include <QPointer>
#include <QQueue>
#include <QSharedPointer>
#include <QWidget>

class QGridLayout;
class QLabel;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QTimer;
class TemplateManager;
struct TemplateMetadata;
using TemplateMetadataPtr= QSharedPointer<TemplateMetadata>;

/**
 * @brief Template page widget for startup tab
 *
 * Displays template categories and grid of template cards.
 * Handles template download and opening.
 */
class QTMTemplatePage : public QWidget {
  Q_OBJECT

public:
  explicit QTMTemplatePage (QWidget* parent= nullptr);
  ~QTMTemplatePage ();

  void initialize ();

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;
  void showEvent (QShowEvent* event) override;
  void resizeEvent (QResizeEvent* event) override;

private slots:
  void onTemplatesLoaded ();
  void onCategoriesLoaded ();
  void onCategoryClicked ();

private:
  void     setupUI ();
  void     setupCategoryBar ();
  QWidget* createTemplateCard (const TemplateMetadataPtr& tmpl);
  void     refreshTemplateGrid (const QString& category);
  int      calculateColumnCount () const;
  void     showTemplatePreview (const QString& templateId);
  void     downloadAndUseTemplate (const QString& templateId);

  // UI components
  QLabel*      titleLabel_;
  QWidget*     categoryBar_;
  QScrollArea* scrollArea_;
  QWidget*     gridWidget_;
  QGridLayout* gridLayout_;

  // Data
  TemplateManager* templateManager_;
  QString          currentCategory_;
  QPushButton*     activeCategoryBtn_;

  // Responsive grid
  int currentColumnCount_= 4;

  // Avoid duplicate refresh when onTemplatesLoaded and showEvent both fire
  bool gridNeedsRefresh_= true;

  // Debounce timer for resize events to avoid frequent grid rebuilds
  QTimer* resizeDebounceTimer_;
};

#endif // QTMTEMPLATEPAGE_HPP
