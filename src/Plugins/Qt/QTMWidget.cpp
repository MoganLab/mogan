
/******************************************************************************
 * MODULE     : QTMWidget.cpp
 * DESCRIPTION: QT Texmacs widget class
 * COPYRIGHT  : (C) 2008 Massimiliano Gubinelli and Joris van der Hoeven
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMWidget.hpp"
#include "analyze.hpp"
#include "array.hpp"
#include "boot.hpp"
#include "converter.hpp"
#include "object_l5.hpp"
#include "preferences.hpp"
#include "qt_gui.hpp"
#include "qt_simple_widget.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "sys_utils.hpp"
#ifdef USE_MUPDF_RENDERER
#include "mupdf_picture.hpp"
#endif

#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>

#include "lolly/io/http.hpp"
#include "tm_url.hpp"
#include <QBuffer>
#include <QByteArray>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMimeData>
#include <QUrl>

using namespace moebius;

static int64_t QTMWcounter= 0; // debugging hack

static QByteArray
qt_download_image_data (const QString& url_str) {
  string url_s= from_qstring (url_str);
  debug_qt << "Downloading image from URL: " << url_s << LF;
  url                     image_url= url_system (url_s);
  lolly::io::http_headers headers;
  headers ("User-Agent")=
      "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
      "like Gecko) Chrome/91.0.4472.124 Safari/537.36";

  // Download image using temporary file
  url                  temp_file= url_temp ("img");
  lolly::io::http_tree response=
      lolly::io::download (image_url, temp_file, headers);

  long status_code= open_box<long> (
      lolly::io::http_response_ref (response, lolly::io::STATUS_CODE)->data);

  QByteArray data;
  if (status_code == 200 && exists (temp_file)) {
    // Use Qt file reading for binary data
    QFile file (to_qstring (as_string (temp_file)));
    if (file.open (QIODevice::ReadOnly)) {
      data= file.readAll ();
      file.close ();
      if (data.isEmpty ()) {
        debug_qt << "Downloaded empty data from " << url_s << LF;
      }
    }
    else {
      debug_qt << "Failed to open temp file: " << as_string (temp_file) << LF;
    }

    // Clean up temporary file
    remove (temp_file);
  }
  else {
    debug_qt << "Failed to download image from " << url_s
             << ", status code: " << status_code << LF;
  }

  return data;
}

/*! Constructor.

  \param _parent The parent QWidget.
  \param _tmwid the TeXmacs widget who owns this object.
 */
QTMWidget::QTMWidget (QWidget* _parent, qt_widget _tmwid)
    : QTMScrollView (_parent), tmwid (_tmwid), imwidget (NULL),
      preediting (false) {
  setObjectName (to_qstring (
      "QTMWidget" *
      as_string (QTMWcounter++))); // What is this for? (maybe only debugging?)
  setFocusPolicy (Qt::StrongFocus);
  setAttribute (Qt::WA_InputMethodEnabled);
  surface ()->setMouseTracking (true);
  surface ()->setAcceptDrops (true);
  grabGesture (Qt::PanGesture);
  grabGesture (Qt::PinchGesture);
  grabGesture (Qt::SwipeGesture);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
  surface ()->setTabletTracking (true);
  for (QWidget* parent= surface ()->parentWidget (); parent != nullptr;
       parent         = parent->parentWidget ())
    parent->setTabletTracking (true);
#endif

  if (DEBUG_QT)
    debug_qt << "Creating " << from_qstring (objectName ()) << " of widget "
             << (tm_widget () ? tm_widget ()->type_as_string () : "NULL") << LF;
  // part 1/2 of the fix for 43373
  if (!isEmbedded ())
    QApplication::postEvent (
        this, new QFocusEvent (QEvent::FocusIn, Qt::OtherFocusReason));
}

QTMWidget::~QTMWidget () {
  if (DEBUG_QT)
    debug_qt << "Destroying " << from_qstring (objectName ()) << " of widget "
             << (tm_widget () ? tm_widget ()->type_as_string () : "NULL") << LF;
}

bool
QTMWidget::isEmbedded () const {
  return tm_widget ()->is_embedded_widget ();
}

qt_simple_widget_rep*
QTMWidget::tm_widget () const {
  return concrete_simple_widget (tmwid);
}

void
QTMWidget::scrollContentsBy (int dx, int dy) {
  QTMScrollView::scrollContentsBy (dx, dy);

  the_gui->force_update ();
  // we force an update of the internal state to be in sync with the moving
  // scrollbars

  // 确保补全框也滚动到正确的位置
  tm_widget ()->scroll_completion_popup_by (dx, dy);
  tm_widget ()->scroll_math_completion_popup_by (dx, dy);
  tm_widget ()->scroll_image_popup_by (dx, dy);
  tm_widget ()->scroll_code_popup_by (dx, dy);
}

void
QTMWidget::resizeEvent (QResizeEvent* event) {
  (void) event;
  // Is this ok?
  // coord2 s = from_qsize (event->size());
  // the_gui -> process_resize (tm_widget(), s.x1, s.x2);

  // the_gui->force_update();

  // FIXME: I would like to have a force_update here but this causes a failed
  // assertion in TeXmacs since the at the boot not every internal structure is
  // initialized at this point. It seems not too difficult to fix but I
  // postpone this to discuss with Joris.
  //
  // Not having a force_update results in some lack of sync of the surface
  // while the user is actively resizing with the mouse.
}

void
QTMWidget::resizeEventBis (QResizeEvent* event) {
  coord2 s= from_qsize (event->size ());
  the_gui->process_resize (tm_widget (), s.x1, s.x2);
}

/*!
 In the current implementation repainting takes place during the call to
 the widget's repaint_invalid_regions() method in the_gui::update. All
 we have to do is to take the backing store and put it on screen according
 to the QRegion marked invalid.
 CHECK: Maybe just putting onscreen all the region bounding rectangles might
 be less expensive.
*/
#ifdef USE_MUPDF_RENDERER
void
QTMWidget::paintEvent (QPaintEvent* event) {
  QImage   bs= tm_widget ()->get_backing_store ();
  QPainter p (surface ());
  // this code override the invalid region computations
  p.drawImage (QRect (QPoint (), size ()), bs,
               QRect (QPoint (), size () * retina_factor));
  return;
}
#else
void
QTMWidget::paintEvent (QPaintEvent* event) {
  QPainter p (surface ());
#if QT_VERSION >= 0x060000
  QVector<QRect> rects (1, event->region ().boundingRect ());
#else
  QVector<QRect> rects= event->region ().rects ();
#endif
  for (int i= 0; i < rects.count (); ++i) {
    QRect qr= rects.at (i);
    p.drawPixmap (QRect (qr.x (), qr.y (), qr.width (), qr.height ()),
                  *(tm_widget ()->backingPixmap),
                  QRect (retina_factor * qr.x (), retina_factor * qr.y (),
                         retina_factor * qr.width (),
                         retina_factor * qr.height ()));
  }
}
#endif

void
QTMWidget::keyPressEvent (QKeyEvent* event) {
  if (is_nil (tmwid)) return;

  string r= from_key_press_event (event);
  if (is_empty (r)) return;
  if (DEBUG_KEYBOARD) debug_qt << "key pressed: " << r << LF;

  the_gui->process_keypress (tm_widget (), r, texmacs_time ());
}

void
QTMWidget::keyReleaseEvent (QKeyEvent* event) {
  string r= from_key_release_event (event);
  if (is_empty (r)) return;
  if (DEBUG_KEYBOARD) debug_qt << "key released: " << r << LF;

  the_gui->process_keypress (tm_widget (), r, texmacs_time ());
}

static unsigned int
mouse_state (QMouseEvent* event, bool flag) {
  unsigned int          i     = 0;
  Qt::MouseButtons      bstate= event->buttons ();
  Qt::MouseButton       tstate= event->button ();
  Qt::KeyboardModifiers kstate= event->modifiers ();
  if (flag) bstate= bstate | tstate;
  if ((bstate & Qt::LeftButton) != 0) i+= 1;
#if QT_VERSION < 0x060000
  if ((bstate & Qt::MidButton) != 0) i+= 2;
#else
  if ((bstate & Qt::MiddleButton) != 0) i+= 2;
#endif
  if ((bstate & Qt::RightButton) != 0) i+= 4;
  if ((bstate & Qt::XButton1) != 0) i+= 8;
  if ((bstate & Qt::XButton2) != 0) i+= 16;
#ifdef Q_OS_MAC
  // We emulate right and middle clicks with ctrl and option, but we pass the
  // modifiers anyway: old code continues to work and new one can use them.
  if ((kstate & Qt::MetaModifier) != 0) i= 1024 + 4; // control key
  if ((kstate & Qt::AltModifier) != 0) i= 2048 + 2;  // option key
  if ((kstate & Qt::ShiftModifier) != 0) i+= 256;
  if ((kstate & Qt::ControlModifier) != 0) i+= 4096; // cmd key
#else
  if ((kstate & Qt::ShiftModifier) != 0) i+= 256;
  if ((kstate & Qt::ControlModifier) != 0) i+= 1024;
  if ((kstate & Qt::AltModifier) != 0) i+= 2048;
  if ((kstate & Qt::MetaModifier) != 0) i+= 4096;
#endif
  return i;
}

static string
mouse_decode (unsigned int mstate) {
  if (mstate & 2) return "middle";
  else if (mstate & 4) return "right";
  // we check for left clicks after the others for macos (see ifdef in
  // mouse_state)
  else if (mstate & 1) return "left";
  else if (mstate & 8) return "up";
  else if (mstate & 16) return "down";
  return "unknown";
}

void
QTMWidget::inputMethodEvent (QInputMethodEvent* event) {
  QString const& preedit_string= event->preeditString ();
  QString const& commit_string = event->commitString ();

  if (!commit_string.isEmpty ()) {
    bool done= false;
    if (!done) {
      if (DEBUG_QT)
        debug_qt << "IM committing: " << commit_string.toUtf8 ().data () << LF;
      if (get_preference ("speech", "off") == "on") {
        string key_u8= "speech:" * from_qstring_utf8 (commit_string);
        the_gui->process_keypress (tm_widget (), key_u8, texmacs_time ());
      }
      else {
        string key_u8= from_qstring_utf8 (commit_string);
        the_gui->process_keypress (tm_widget (), key_u8, texmacs_time ());
      }
    }
  }

  if (DEBUG_QT)
    debug_qt << "IM preediting :" << preedit_string.toUtf8 ().data () << LF;

  bool   im_preedit_switch= true;
  string im_preedit_str   = get_preference ("IM:preedit");
  if (im_preedit_str == "off") {
    im_preedit_switch= false;
  }
  else if (im_preedit_str == "on") {
    im_preedit_switch= true;
  }
  // Disable preedit in math mode to prevent crash in QQPinyin
  if (as_bool (call ("in-math?"))) im_preedit_switch= false;

  string r= "pre-edit:";
  if (im_preedit_switch && !preedit_string.isEmpty ()) {
    // find cursor position in the preedit string
    QList<QInputMethodEvent::Attribute> const& attrs= event->attributes ();
    //    int pos = preedit_string.size();
    int  pos        = 0;
    bool visible_cur= false;
    for (int i= 0; i < attrs.count (); i++)
      if (attrs[i].type == QInputMethodEvent::Cursor) {
        pos        = attrs[i].start;
        visible_cur= (attrs[i].length != 0);
      }

    // find selection in the preedit string
    int sel_start = 0;
    int sel_length= 0;
    if (pos < preedit_string.size ()) {
      for (int i= 0; i < attrs.count (); i++)
        if ((attrs[i].type == QInputMethodEvent::TextFormat) &&
            (attrs[i].start <= pos) &&
            (pos < attrs[i].start + attrs[i].length)) {
          sel_start = attrs[i].start;
          sel_length= attrs[i].length;
          if (!visible_cur) pos+= attrs[i].length;
        }
    }
    else {
      sel_start = pos;
      sel_length= 0;
    }
    (void) sel_start;
    (void) sel_length;

    r= r * as_string (pos) * ":" * from_qstring_utf8 (preedit_string);
  }

  if (im_preedit_switch && !is_nil (tmwid)) {
    preediting= !preedit_string.isEmpty ();
    the_gui->process_keypress (tm_widget (), r, texmacs_time ());
  }

  event->accept ();
}

QVariant
QTMWidget::inputMethodQuery (Qt::InputMethodQuery query) const {
  switch (query) {
#if QT_VERSION < 0x060000
  case Qt::ImMicroFocus: {
    const QPoint& topleft= cursor_pos - tm_widget ()->backing_pos +
                           surface ()->geometry ().topLeft ();
    return QVariant (QRect (topleft, QSize (5, 5)));
  }
#else
  case Qt::ImEnabled: {
    return QVariant (true);
  }
  case Qt::ImCursorRectangle: {
    const QPoint& topleft= cursor_pos - tm_widget ()->backing_pos +
                           surface ()->geometry ().topLeft ();
    return QVariant (QRect (topleft, QSize (5, 5)));
  }
#endif // TODO : Correctly implement input methods
  default:
    return QWidget::inputMethodQuery (query);
  }
}

void
QTMWidget::mousePressEvent (QMouseEvent* event) {
  if (is_nil (tmwid)) return;
  QPoint       point = event->pos () + origin ();
  coord2       pt    = from_qpoint (point);
  unsigned int mstate= mouse_state (event, false);
  string       s     = "press-" * mouse_decode (mstate);
  the_gui->process_mouse (tm_widget (), s, pt.x1, pt.x2, mstate,
                          texmacs_time ());
  event->accept ();
}

void
QTMWidget::mouseReleaseEvent (QMouseEvent* event) {
  if (is_nil (tmwid)) return;
  QPoint       point = event->pos () + origin ();
  coord2       pt    = from_qpoint (point);
  unsigned int mstate= mouse_state (event, true);
  string       s     = "release-" * mouse_decode (mstate);
  the_gui->process_mouse (tm_widget (), s, pt.x1, pt.x2, mstate,
                          texmacs_time ());
  event->accept ();
}

void
QTMWidget::mouseMoveEvent (QMouseEvent* event) {
  if (is_nil (tmwid)) return;
  QPoint       point = event->pos () + origin ();
  coord2       pt    = from_qpoint (point);
  unsigned int mstate= mouse_state (event, false);
  string       s     = "move";
  the_gui->process_mouse (tm_widget (), s, pt.x1, pt.x2, mstate,
                          texmacs_time ());
  event->accept ();
}

#if (QT_VERSION >= 0x050000)
static unsigned int
tablet_state (QTabletEvent* event, bool flag) {
  unsigned int     i     = 0;
  Qt::MouseButtons bstate= event->buttons ();
  Qt::MouseButton  tstate= event->button ();
  if (flag) bstate= bstate | tstate;
  if ((bstate & Qt::LeftButton) != 0) i+= 1;
#if QT_VERSION < 0x060000
  if ((bstate & Qt::MidButton) != 0) i+= 2;
#else
  if ((bstate & Qt::MiddleButton) != 0) i+= 2;
#endif
  if ((bstate & Qt::RightButton) != 0) i+= 4;
  if ((bstate & Qt::XButton1) != 0) i+= 8;
  if ((bstate & Qt::XButton2) != 0) i+= 16;
  return i;
}

void
QTMWidget::tabletEvent (QTabletEvent* event) {
  if (is_nil (tmwid)) return;
  unsigned int mstate= tablet_state (event, true);
  string       s     = "move";
  if (event->button () != 0) {
    if (event->pressure () == 0) s= "release-" * mouse_decode (mstate);
    else s= "press-" * mouse_decode (mstate);
  }
  if ((mstate & 4) == 0 || s == "press-right") {
    QPoint point=
        event->position ().toPoint () + origin () - surface ()->pos ();
    double        x = point.x ();
    double        y = point.y ();
    coord2        pt= coord2 ((SI) (x * PIXEL), (SI) (-y * PIXEL));
    array<double> data;
    data << ((double) event->pressure ()) << ((double) event->rotation ())
         << ((double) event->xTilt ()) << ((double) event->yTilt ())
         << ((double) event->z ()) << ((double) event->tangentialPressure ());
    the_gui->process_mouse (tm_widget (), s, pt.x1, pt.x2, mstate,
                            texmacs_time (), data);
  }
  /*
  cout << HRULE << LF;
  cout << "button= " << event->button() << LF;
  cout << "globalX= " << event->globalX() << LF;
  cout << "globalY= " << event->globalY() << LF;
  cout << "hiResGlobalX= " << event->hiResGlobalX() << LF;
  cout << "hiResGlobalY= " << event->hiResGlobalY() << LF;
  cout << "globalX= " << event->globalX() << LF;
  cout << "globalY= " << event->globalY() << LF;
  cout << "x= " << event->x() << LF;
  cout << "y= " << event->y() << LF;
  cout << "z= " << event->z() << LF;
  cout << "xTilt= " << event->xTilt() << LF;
  cout << "yTilt= " << event->yTilt() << LF;
  cout << "pressure= " << event->pressure() << LF;
  cout << "rotation= " << event->rotation() << LF;
  cout << "tangentialPressure= " << event->tangentialPressure() << LF;
  cout << "pointerType= " << event->pointerType() << LF;
  cout << "uniqueId= " << event->uniqueId() << LF;
  */
  event->accept ();
}
#endif

void
QTMWidget::gestureEvent (QGestureEvent* event) {
  if (is_nil (tmwid)) return;
  string        s= "gesture";
  array<double> data;
  QPointF       hotspot;
  if (QGesture* swipe_gesture= event->gesture (Qt::SwipeGesture)) {
    QSwipeGesture* swipe= static_cast<QSwipeGesture*> (swipe_gesture);
    s                   = "swipe";
    hotspot             = swipe->hotSpot ();
    if (swipe->state () == Qt::GestureFinished) {
      if (swipe->horizontalDirection () == QSwipeGesture::Left) s= "swipe-left";
      else if (swipe->horizontalDirection () == QSwipeGesture::Right)
        s= "swipe-right";
      else if (swipe->verticalDirection () == QSwipeGesture::Up) s= "swipe-up";
      else if (swipe->verticalDirection () == QSwipeGesture::Down)
        s= "swipe-down";
    }
    else {
      event->accept ();
      return;
    }
  }
  else if (QGesture* pan_gesture= event->gesture (Qt::PanGesture)) {
    QPanGesture* pan= static_cast<QPanGesture*> (pan_gesture);
    string       s  = "pan";
    hotspot         = pan->hotSpot ();
    // QPointF delta = pan->delta();
    // cout << "Pan " << delta.x() << ", " << delta.y() << LF;
  }
  else if (QGesture* pinch_gesture= event->gesture (Qt::PinchGesture)) {
    QPinchGesture* pinch= static_cast<QPinchGesture*> (pinch_gesture);
    s                   = "pinch";
    hotspot             = pinch->hotSpot ();
    QPinchGesture::ChangeFlags changeFlags= pinch->changeFlags ();
    if (pinch->state () == Qt::GestureStarted) {
      pinch->setRotationAngle (0.0);
      pinch->setScaleFactor (1.0);
      s= "pinch-start";
    }
    else if (pinch->state () == Qt::GestureFinished) {
      pinch->setRotationAngle (0.0);
      pinch->setScaleFactor (1.0);
      s= "pinch-end";
    }
    else if (changeFlags & QPinchGesture::RotationAngleChanged) {
      qreal angle= pinch->rotationAngle ();
      s          = "rotate";
      data << ((double) angle);
    }
    else if (changeFlags & QPinchGesture::ScaleFactorChanged) {
      qreal scale= pinch->totalScaleFactor ();
      s          = "scale";
      data << ((double) scale);
    }
  }
  else return;
  QPoint point (hotspot.x (), hotspot.y ());
  coord2 pt= from_qpoint (point);
  // cout << s << ", " << pt.x1 << ", " << pt.x2 << LF;
  the_gui->process_mouse (tm_widget (), s, pt.x1, pt.x2, 0, texmacs_time (),
                          data);
  event->accept ();
}

bool
QTMWidget::event (QEvent* event) {
  // Catch Keypresses to avoid default handling of (Shift+)Tab keys
  if (event->type () == QEvent::KeyPress) {
    QKeyEvent* ke= static_cast<QKeyEvent*> (event);
    keyPressEvent (ke);
    return true;
  }
  /* NOTE: we catch ShortcutOverride in order to disable the QKeySequences we
   assigned to QActions while building menus, etc. In doing this, we keep the
   shortcut text in the menus while relaying all keypresses through the editor*/
  if (event->type () == QEvent::ShortcutOverride) {
    event->accept ();
    return true;
  }
  if (event->type () == QEvent::Gesture) {
    gestureEvent (static_cast<QGestureEvent*> (event));
    return true;
  }
  return QTMScrollView::event (event);
}

void
QTMWidget::focusInEvent (QFocusEvent* event) {
  if (!is_nil (tmwid)) {
    if (DEBUG_QT)
      debug_qt << "FOCUSIN: " << tm_widget ()->type_as_string () << LF;
    the_gui->process_keyboard_focus (tm_widget (), true, texmacs_time ());
  }
  QTMScrollView::focusInEvent (event);
  // part 2/2 of the fix for bug 43373.
  if (!isEmbedded ()) {
    if (!isActiveWindow ()) activateWindow ();
    if (isActiveWindow () && !hasFocus ()) setFocus (Qt::OtherFocusReason);
    //=> this will send us back here...
    // This redundancy is weird but definitely needed to properly get focus with
    // Qt >= 5.15. Qt bug?
  }
}

void
QTMWidget::focusOutEvent (QFocusEvent* event) {
  if (!is_nil (tmwid)) {
    if (DEBUG_QT)
      debug_qt << "FOCUSOUT: " << tm_widget ()->type_as_string () << LF;
    the_gui->process_keyboard_focus (tm_widget (), false, texmacs_time ());
  }
  QTMScrollView::focusOutEvent (event);
}

QSize
QTMWidget::sizeHint () const {
  SI w= 0, h= 0;
  if (!is_nil (tmwid)) tm_widget ()->handle_get_size_hint (w, h);
  return to_qsize (w, h);
}

void
QTMWidget::dragEnterEvent (QDragEnterEvent* event) {
  if (is_nil (tmwid)) return;
  const QMimeData* md= event->mimeData ();
  if (md->hasText () || md->hasUrls () || md->hasImage () ||
      md->hasFormat ("application/pdf") ||
      md->hasFormat ("application/postscript"))
    event->acceptProposedAction ();
}

// cache to transfer drop data to the editor
// via standard mouse events, see dropEvent below

int                drop_payload_serial= 0;
hashmap<int, tree> payloads;

void
QTMWidget::dropEvent (QDropEvent* event) {
  if (is_nil (tmwid)) return;

  QPoint point= event->position ().toPoint () + origin ();
  coord2 pt   = from_qpoint (point);

  tree             doc (CONCAT);
  const QMimeData* md= event->mimeData ();
  QByteArray       buf;

  if (md->hasUrls ()) {
    QList<QUrl> l= md->urls ();
    for (int i= 0; i < l.size (); i++) {
      string name;
#ifdef OS_MACOS
      name= from_qstring (fromNSUrl (l[i]));
#else
      name= from_qstring (l[i].toLocalFile ());
#endif
#if defined(OS_MINGW) || defined(OS_WIN)
      if (N (name) >= 2 && is_alpha (name[0]) && name[1] == ':')
        name= "/" * locase_all (name (0, 1)) * name (2, N (name));
#endif
      string extension= suffix (name);
      string w, h;
      int    ww, hh;
      // 检查是否为远程URL (http/https)
      if (l[i].scheme ().startsWith ("http")) {
        QByteArray imageData= qt_download_image_data (l[i].toString ());
        if (!imageData.isEmpty ()) {
          QBuffer qbuf (&buf);
          QImage  image;
          qbuf.open (QIODevice::WriteOnly);

          // 尝试加载下载的图片数据
          if (image.loadFromData (imageData)) {
            image.save (&qbuf, "PNG");

            QSize size= image.size ();
            ww        = size.width ();
            hh        = size.height ();
            qt_pretty_image_size (ww, hh, w, h);
            string imagedata (buf.constData (), buf.size ());

            tree t (IMAGE, tuple (tree (RAW_DATA, imagedata), "png"), w, h, "",
                    "");
            doc << t;
          }
          else {
            // 如果图片加载失败，回退到文本链接
            doc << from_qstring (l[i].toString ());
          }
        }
        else {
          // 如果下载失败，回退到文本链接
          doc << from_qstring (l[i].toString ());
        }
      }
#ifdef USE_MUPDF_RENDERER
      else if (mupdf_supports (extension) || extension == "eps" ||
               extension == "ps" || extension == "pdf") {
        string imagedata= mupdf_load_and_parse_image (
            l[i].toLocalFile ().toStdString ().c_str (), ww, hh, extension, &w,
            &h);
#else
      else if ((extension == "eps") || (extension == "ps") ||
#if (QT_VERSION >= 0x050000)
               (extension == "svg") ||
#endif
               (extension == "pdf") || (extension == "png") ||
               (extension == "jpg") || (extension == "jpeg") ||
               (extension == "webp")) {
        QBuffer qbuf (&buf);
        QImage  image;
        qbuf.open (QIODevice::WriteOnly);
        image.load (l[i].toLocalFile ());
        image.save (&qbuf, "PNG");
        QSize size= image.size ();
        ww        = size.width ();
        hh        = size.height ();
        string imagedata (buf.constData (), buf.size ());
        qt_pretty_image_size (ww, hh, w, h);
#endif
        tree t (IMAGE, tuple (tree (RAW_DATA, imagedata), name), w, h, "", "");
        doc << t;
      }
      else {
        doc << name;
      }
    }
  }
  else if (md->hasImage ()) {
    QBuffer qbuf (&buf);
    QImage  image= qvariant_cast<QImage> (md->imageData ());
    QSize   size = image.size ();
    qbuf.open (QIODevice::WriteOnly);
    image.save (&qbuf, "PNG");
    int    ww= size.width (), hh= size.height ();
    string w, h;
    qt_pretty_image_size (ww, hh, w, h);
    tree t (IMAGE,
            tree (RAW_DATA, string (buf.constData (), buf.size ()), "png"), w,
            h, "", "");
    doc << t;
  }
  else if (md->hasFormat ("application/postscript")) {
    buf= md->data ("application/postscript");
    tree t (IMAGE,
            tree (RAW_DATA, string (buf.constData (), buf.size ()), "ps"), "",
            "", "", "");
    doc << t;
  }
  else if (md->hasFormat ("application/pdf")) {
    buf= md->data ("application/pdf");
    tree t (IMAGE,
            tree (RAW_DATA, string (buf.constData (), buf.size ()), "pdf"), "",
            "", "", "");
    doc << t;
  }
  else if (md->hasText ()) {
    buf= md->text ().toUtf8 ();
    doc << string (buf.constData (), buf.size ());
  }

  if (N (doc) > 0) {
    if (N (doc) == 1) doc= doc[0];
    else {
      tree sec (CONCAT, doc[0]);
      for (int i= 1; i < N (doc); i++)
        sec << tree (" ") << doc[i];
      doc= sec;
    }
    int ticket       = drop_payload_serial++;
    payloads (ticket)= doc;
    the_gui->process_mouse (tm_widget (), "drop", pt.x1, pt.x2, ticket,
                            texmacs_time ());
    event->acceptProposedAction ();
  }
}

static unsigned int
wheel_state (QWheelEvent* event) {
  // TODO: factor mouse_state, tablet_state, wheel_state
  // This should be easier on modern versions of Qt
  unsigned int          i     = 0;
  Qt::MouseButtons      bstate= event->buttons ();
  Qt::KeyboardModifiers kstate= event->modifiers ();
  if ((bstate & Qt::LeftButton) != 0) i+= 1;
#if QT_VERSION < 0x060000
  if ((bstate & Qt::MidButton) != 0) i+= 2;
#else
  if ((bstate & Qt::MiddleButton) != 0) i+= 2;
#endif
  if ((bstate & Qt::RightButton) != 0) i+= 4;
  if ((bstate & Qt::XButton1) != 0) i+= 8;
  if ((bstate & Qt::XButton2) != 0) i+= 16;
#ifdef Q_OS_MAC
  // We emulate right and middle clicks with ctrl and option, but we pass the
  // modifiers anyway: old code continues to work and new one can use them.
  if ((kstate & Qt::MetaModifier) != 0) i= 1024 + 4; // control key
  if ((kstate & Qt::AltModifier) != 0) i= 2048 + 2;  // option key
  if ((kstate & Qt::ShiftModifier) != 0) i+= 256;
  if ((kstate & Qt::ControlModifier) != 0) i+= 4096; // cmd key
#else
  if ((kstate & Qt::ShiftModifier) != 0) i+= 256;
  if ((kstate & Qt::ControlModifier) != 0) i+= 1024;
  if ((kstate & Qt::AltModifier) != 0) i+= 2048;
  if ((kstate & Qt::MetaModifier) != 0) i+= 4096;
#endif
  return i;
}

void
QTMWidget::wheelEvent (QWheelEvent* event) {
  if (is_nil (tmwid)) return;
  if (as_bool (call ("wheel-capture?"))) {
#if (QT_VERSION >= 0x060000)
    QPoint point= (event->position ()).toPoint () + origin ();
#else
    QPoint point= event->pos () + origin ();
#endif
    QPoint        wheel = event->pixelDelta ();
    coord2        pt    = from_qpoint (point);
    coord2        wh    = from_qpoint (wheel);
    unsigned int  mstate= wheel_state (event);
    array<double> data;
    data << ((double) wh.x1) << ((double) wh.x2);
    the_gui->process_mouse (tm_widget (), "wheel", pt.x1, pt.x2, mstate,
                            texmacs_time (), data);
  }
  else if (QApplication::keyboardModifiers () == Qt::ControlModifier) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // see https://doc.qt.io/qt-5/qwheelevent-obsolete.html#delta
    if (event->delta () > 0)
#else
    QPoint numDegrees= event->angleDelta () / 8;
    if (numDegrees.y () > 0)
#endif
      call ("zoom-in", object (sqrt (sqrt (2.0))));
    else call ("zoom-out", object (sqrt (sqrt (2.0))));
  }
  else QAbstractScrollArea::wheelEvent (event);
}

void
QTMWidget::showEvent (QShowEvent* event) {
  the_gui->force_update ();
}
