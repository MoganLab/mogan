
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget.hpp
 * DESCRIPTION: Continuous-scroll PDF reader widget with toolbar
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#ifndef QT_PDF_READER_WIDGET_HPP
#define QT_PDF_READER_WIDGET_HPP

#include <QComboBox>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QRubberBand>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief Represents a clickable link on a PDF page
 */
struct PdfLink {
  QRectF  rect; // normalized page coordinates [0,1]
  QString uri;  // original URI from MuPDF
  int     page; // resolved target page (0-based), -1 if unresolved
};

/**
 * @brief Key for per-page render cache
 */
struct PdfPageCacheKey {
  int  pageNumber;
  int  targetWidth;
  bool operator== (const PdfPageCacheKey& other) const {
    return pageNumber == other.pageNumber && targetWidth == other.targetWidth;
  }
};

inline uint
qHash (const PdfPageCacheKey& key, uint seed= 0) {
  return qHash (key.pageNumber, seed) ^ qHash (key.targetWidth, seed);
}

/**
 * @brief Continuous-scroll PDF reader widget with toolbar
 *
 * Renders all pages vertically in a scroll area.
 * Supports mouse wheel zoom, Fit Width/Height, and page navigation.
 */
class PDFReaderWidget : public QWidget {
  Q_OBJECT

public:
  explicit PDFReaderWidget (QWidget* parent= nullptr);
  ~PDFReaderWidget ();

  bool loadFromFile (const QString& filePath, int dpi= 150);
  void clear ();

  int    pageCount () const { return pageCount_; }
  bool   hasError () const { return hasError_; }
  double zoomFactor () const { return zoomFactor_; }
  void   setZoomFactor (double factor);
  void   fitWidth ();
  void   fitHeight ();
  void   zoomIn ();
  void   zoomOut ();

  int  currentPage () const;
  void goToPage (int page);

  bool canGoToPrevPage () const;
  bool canGoToNextPage () const;

  QWidget*    viewport () const;
  QScrollBar* verticalScrollBar () const;

  bool isRectSelectMode () const;

  // Link support (public for testing)
  void setTestLinks (int page, const QVector<PdfLink>& links);
  bool isOverLink () const;

Q_SIGNALS:
  void linkClicked (const QString& uri);

private slots:
  void onZoomChanged (int index);
  void onPrevPage ();
  void onNextPage ();
  void onPageEditingFinished ();
  void updatePageNavigation ();
  void onRectSelectToggled (bool checked);
  void keyPressEvent (QKeyEvent* event) override;

private:
  bool    renderPageToLabel (int pageNumber, QLabel* label, int targetWidth);
  void    rebuildPages ();
  int     pageWidth () const;
  void    setupToolBar ();
  void    updateZoomDisplay ();
  void    finishRectSelect (const QPoint& viewportPos);
  QLabel* findPageLabelAt (const QPoint& contentPos) const;
  QPixmap extractSelectionPixmap (QLabel*      label,
                                  const QRect& contentRect) const;

  void    extractPageLinks ();
  void    clearPageLinks ();
  PdfLink linkAtPos (const QPoint& contentPos) const;
  void    handleLinkClick (const PdfLink& link);
  void    updateLinkCursor (const QPoint& contentPos);

  bool eventFilter (QObject* watched, QEvent* event) override;

  QScrollArea* scrollArea_;
  QWidget*     contentWidget_;
  QVBoxLayout* pageLayout_;
  QVBoxLayout* mainLayout_;

  QWidget*     toolBar_;
  QComboBox*   zoomCombo_;
  QToolButton* zoomOutBtn_;
  QToolButton* prevPageBtn_;
  QLineEdit*   pageEdit_;
  QLabel*      pageTotalLabel_;
  QToolButton* nextPageBtn_;
  QToolButton* zoomInBtn_;
  QToolButton* rectSelectBtn_;

  QRubberBand* rubberBand_;
  bool         rectSelectMode_;
  QPoint       rectSelectStart_;
  bool         rectSelectDragging_;
  QLabel*      hintLabel_;

  // Browse (hand) tool state
  bool       browseDragging_;
  QPoint     browseDragStartPos_;
  bool       browseDragActive_;
  QScroller* scroller_;

  QByteArray pdfData_;
  int        pageCount_;
  bool       hasError_;
  QString    errorString_;
  int        targetDpi_;
  double     zoomFactor_;
  double     pageAspectRatio_;
  double     pageBaseWidthPts_;

  // 每页宽高比缓存（用于可见性裁剪和快速高度计算）
  QVector<double> pageAspectRatios_;

  // 每页链接列表（用于点击跳转）
  QVector<QVector<PdfLink>> pageLinks_;
  PdfLink                   currentLink_;
  bool                      overLink_;

  // 页面渲染缓存：key = (pageNumber, targetWidth)
  QHash<PdfPageCacheKey, QPixmap> pageCache_;

  // 防抖定时器
  QTimer* zoomDebounceTimer_;
  QTimer* resizeDebounceTimer_;

  static constexpr int    DEFAULT_DPI       = 150;
  static constexpr int    PAGE_MARGIN       = 16;
  static constexpr int    PRELOAD_MARGIN    = 200;
  static constexpr double MIN_ZOOM          = 0.12;
  static constexpr double MAX_ZOOM          = 8.0;
  static constexpr int    ZOOM_DEBOUNCE_MS  = 200;
  static constexpr int    RESIZE_DEBOUNCE_MS= 300;

  static constexpr int    ZOOM_LEVEL_COUNT= 12;
  static constexpr double ZOOM_LEVELS[ZOOM_LEVEL_COUNT]{
      0.25, 0.33, 0.50, 0.75, 1.00, 1.25, 1.50, 2.00, 3.00, 4.00, 6.00, 8.00};
};

/* PdfPageCacheKey qHash defined above */

#endif // QT_PDF_READER_WIDGET_HPP
