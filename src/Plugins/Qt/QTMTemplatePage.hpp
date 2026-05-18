
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

  // 初始化：连接 TemplateManager 信号
  void initialize ();

  // 设置当前显示的分类
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

  // UI 组件
  QLabel*      titleLabel_;
  QScrollArea* scrollArea_;
  QWidget*     gridWidget_;
  QGridLayout* gridLayout_;

  // 数据
  TemplateManager* templateManager_;
  QString          currentCategory_;

  // 响应式网格：当前列数
  int currentColumnCount_= 4;

  // 避免 onTemplatesLoaded 和 showEvent 重复刷新
  bool gridNeedsRefresh_= true;

  // resize 防抖定时器，避免拖拽窗口时频繁重建网格
  QTimer* resizeDebounceTimer_;
};

#endif // QTMTEMPLATEPAGE_HPP
