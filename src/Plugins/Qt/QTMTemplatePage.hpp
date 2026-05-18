
/******************************************************************************
 * MODULE     : QTMTemplatePage.hpp
 * DESCRIPTION: Template page implementation for startup tab
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTMTEMPLATEPAGE_HPP
#define QTMTEMPLATEPAGE_HPP

#include <QSharedPointer>
#include <QWidget>

class QGridLayout;
class QLabel;
class QResizeEvent;
class QScrollArea;
class QTimer;
class TemplateManager;
struct TemplateMetadata;
using TemplateMetadataPtr= QSharedPointer<TemplateMetadata>;

/**
 * @brief Template page widget for startup tab
 *
 * Displays a grid of template cards for the selected category.
 * Handles template download and opening.
 */
class QTMTemplatePage : public QWidget {
  Q_OBJECT

public:
  explicit QTMTemplatePage (QWidget* parent= nullptr);
  ~QTMTemplatePage ();

  void initialize ();

  void    setCategory (const QString& categoryId,
                       const QString& displayName= QString ());
  QString currentCategory () const { return currentCategory_; }
  void    refreshGrid ();

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;
  void showEvent (QShowEvent* event) override;
  void resizeEvent (QResizeEvent* event) override;

private slots:
  void onTemplatesLoaded ();

private:
  void     setupUI ();
  QWidget* createTemplateCard (const TemplateMetadataPtr& tmpl);
  void     refreshTemplateGrid ();
  int      calculateColumnCount () const;
  void     showTemplatePreview (const QString& templateId);

  // UI components
  QLabel*      titleLabel_;
  QScrollArea* scrollArea_;
  QWidget*     gridWidget_;
  QGridLayout* gridLayout_;

  // Data
  TemplateManager* templateManager_;
  QString          currentCategory_;

  // Responsive grid
  int currentColumnCount_= 4;

  // Avoid duplicate refresh when onTemplatesLoaded and showEvent both fire
  bool gridNeedsRefresh_= true;

  // Debounce timer for resize events to avoid frequent grid rebuilds
  QTimer* resizeDebounceTimer_;
};

#endif // QTMTEMPLATEPAGE_HPP
