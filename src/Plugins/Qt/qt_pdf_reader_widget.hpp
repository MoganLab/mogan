
/******************************************************************************
 * MODULE     : qt_pdf_reader_widget.hpp
 * DESCRIPTION: Continuous-scroll PDF reader widget with toolbar
 * COPYRIGHT  : (C) 2026 Da Shen
 ******************************************************************************/

#ifndef QT_PDF_READER_WIDGET_HPP
#define QT_PDF_READER_WIDGET_HPP

#include <QHash>
#include <QLabel>
#include <QRubberBand>
#include <QScrollArea>
#include <QScrollBar>
#include <QScroller>
#include <QTimer>
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

  int  renderCallCount () const { return renderCallCount_; }
  void simulatePinchGesture (Qt::GestureState state, double scaleFactor);

  void setTestLinks (int page, const QVector<PdfLink>& links);
  bool isOverLink () const;

  void updateZoomDisplay ();

  void showContextMenu (const QPoint& pos);

Q_SIGNALS:
  void linkClicked (const QString& uri);
  void zoomChanged (const QString& text);
  void pageChanged (int current, int total);
  void rectSelectModeChanged (bool checked);

public slots:
  void setRectSelectMode (bool checked);
  void onPrevPage ();
  void onNextPage ();
  void updatePageNavigation ();

private slots:
  void keyPressEvent (QKeyEvent* event) override;

  bool event (QEvent* event) override;

private:
  void    startPinchGesture ();
  void    finishPinchGesture ();
  bool    renderPageToLabel (int pageNumber, QLabel* label, int targetWidth);
  void    rebuildPages ();
  void    onResizeDebounced ();
  bool    maybeAutoFitWidth ();
  int     pageWidth () const;
  void    applyZoomToLabels ();
  void    finishRectSelect (const QPoint& viewportPos);
  QLabel* findPageLabelAt (const QPoint& contentPos) const;
  QPixmap extractSelectionPixmap (QLabel*      label,
                                  const QRect& contentRect) const;

  void    extractPageLinks ();
  void    clearPageLinks ();
  PdfLink linkAtPos (const QPoint& contentPos) const;
  void    handleLinkClick (const PdfLink& link);
  void    updateLinkCursor (const QPoint& contentPos);
  void    saveZoomAnchor (const QPoint& viewportPos);
  void    restoreZoomAnchor ();

  bool eventFilter (QObject* watched, QEvent* event) override;

  QScrollArea* scrollArea_;
  QWidget*     contentWidget_;
  QVBoxLayout* pageLayout_;
  QVBoxLayout* mainLayout_;

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
  QString    pdfFilePath_;
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
  QTimer* gestureSafetyTimer_;

  bool   inPinchGesture_;
  bool   blockRender_;
  bool   autoFitApplied_;
  double pinchStartZoom_;

  // Zoom anchor: remembers the content position that should stay
  // fixed during a zoom operation.
  double zoomAnchorContentY_;  // content Y in contentWidget coords
  double zoomAnchorViewportY_; // corresponding Y in viewport coords
  double zoomAnchorOldZoom_;   // zoom factor when anchor was saved
  bool   hasZoomAnchor_;

  int renderCallCount_;

  static constexpr int    DEFAULT_DPI              = 150;
  static constexpr int    PAGE_MARGIN              = 16;
  static constexpr int    PRELOAD_MARGIN           = 200;
  static constexpr double MIN_ZOOM                 = 0.12;
  static constexpr double MAX_ZOOM                 = 8.0;
  static constexpr int    ZOOM_DEBOUNCE_MS         = 200;
  static constexpr int    RESIZE_DEBOUNCE_MS       = 300;
  static constexpr int    GESTURE_SAFETY_TIMEOUT_MS= 500;

  static constexpr int    ZOOM_LEVEL_COUNT= 12;
  static constexpr double ZOOM_LEVELS[ZOOM_LEVEL_COUNT]{
      0.25, 0.33, 0.50, 0.75, 1.00, 1.25, 1.50, 2.00, 3.00, 4.00, 6.00, 8.00};
};

/* PdfPageCacheKey qHash defined above */

#endif // QT_PDF_READER_WIDGET_HPP
