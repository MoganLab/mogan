
/******************************************************************************
 * MODULE     : qt_utilities.cpp
 * DESCRIPTION: Utilities for QT
 * COPYRIGHT  : (C) 2007  Massimiliano Gubinelli
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_utilities.hpp"
#include "QTMStyle.hpp"
#include "url.hpp"
#include <time.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QKeySequence>
#include <QLocale>
#include <QPainter>
#include <QStringList>

#ifdef USE_QT_PRINTER
#include <QPrintDialog>
#include <QPrinter>
#endif

#include <QApplication>
#include <QClipboard>
#include <QImageReader>
#include <QMimeData>

#include "colors.hpp"

#include "converter.hpp"
#include "cork.hpp"
#include "dictionary.hpp"
#include "locale.hpp"
#include "preferences.hpp"
#include "scheme.hpp"
#include "sys_utils.hpp"
#include "wencoding.hpp"

#include "editor.hpp"
#include "new_view.hpp" // get_current_editor()
#include "qt_gui.hpp"   // gui_maximal_extents()

#include <lolly/data/numeral.hpp>
using lolly::data::as_hexadecimal;

#ifdef USE_PLUGIN_GS
#include "Ghostscript/gs_utilities.hpp"
#endif

#define SCREEN_PIXEL (PIXEL)

hashmap<int, string> qtkeymap (0);
hashmap<int, string> qtdeadmap (0);
hashmap<int, string> qtcomposemap (0);

inline void
map (int code, string name) {
  qtkeymap (code)= name;
}

inline void
deadmap (int code, string name) {
  qtdeadmap (code)= name;
}

void
initkeymap () {
  static bool fInit= false;
  if (fInit) return;
  fInit= true;
  if (DEBUG_QT && DEBUG_KEYBOARD) debug_qt << "Initializing keymap\n";
  map (Qt::Key_Space, "space");
  map (Qt::Key_Tab, "tab");
  map (Qt::Key_Backtab, "tab");
  map (Qt::Key_Return, "return");
  map (Qt::Key_Enter, "enter");
  map (Qt::Key_Escape, "escape");
  map (Qt::Key_Backspace, "backspace");
  map (Qt::Key_Up, "up");
  map (Qt::Key_Down, "down");
  map (Qt::Key_Left, "left");
  map (Qt::Key_Right, "right");
  map (Qt::Key_F1, "F1");
  map (Qt::Key_F2, "F2");
  map (Qt::Key_F3, "F3");
  map (Qt::Key_F4, "F4");
  map (Qt::Key_F5, "F5");
  map (Qt::Key_F6, "F6");
  map (Qt::Key_F7, "F7");
  map (Qt::Key_F8, "F8");
  map (Qt::Key_F9, "F9");
  map (Qt::Key_F10, "F10");
  map (Qt::Key_F11, "F11");
  map (Qt::Key_F12, "F12");
  map (Qt::Key_F13, "F13");
  map (Qt::Key_F14, "F14");
  map (Qt::Key_F15, "F15");
  map (Qt::Key_F16, "F16");
  map (Qt::Key_F17, "F17");
  map (Qt::Key_F18, "F18");
  map (Qt::Key_F19, "F19");
  map (Qt::Key_F20, "F20");
  map (Qt::Key_F21, "F21");
  map (Qt::Key_F22, "F22");
  map (Qt::Key_F23, "F23");
  map (Qt::Key_F24, "F24");
  map (Qt::Key_F25, "F25");
  map (Qt::Key_F26, "F26");
  map (Qt::Key_F27, "F27");
  map (Qt::Key_F28, "F28");
  map (Qt::Key_F29, "F29");
  map (Qt::Key_F30, "F30");
  map (Qt::Key_F31, "F31");
  map (Qt::Key_F32, "F32");
  map (Qt::Key_F33, "F33");
  map (Qt::Key_F34, "F34");
  map (Qt::Key_F35, "F35");
  map (Qt::Key_Insert, "insert");
  map (Qt::Key_Delete, "delete");
  map (Qt::Key_Home, "home");
  map (Qt::Key_End, "end");
  map (Qt::Key_PageUp, "pageup");
  map (Qt::Key_PageDown, "pagedown");
  map (Qt::Key_ScrollLock, "scrolllock");
  map (Qt::Key_Pause, "pause");
  map (Qt::Key_SysReq, "sysreq");
  map (Qt::Key_Stop, "stop");
  map (Qt::Key_Menu, "menu");
  map (Qt::Key_Print, "print");
  map (Qt::Key_Select, "select");
  map (Qt::Key_Execute, "execute");
  map (Qt::Key_Help, "help");
  map (Qt::Key_section, "section");

  deadmap (Qt::Key_Dead_Acute, "acute");
  deadmap (Qt::Key_Dead_Grave, "grave");
  deadmap (Qt::Key_Dead_Diaeresis, "umlaut");
  deadmap (Qt::Key_Dead_Circumflex, "hat");
  deadmap (Qt::Key_Dead_Tilde, "tilde");

  // map (0x0003              , "K-enter");
  // map (Qt::Key_Begin       , "begin" );
  // map (Qt::Key_PrintScreen , "printscreen" );
  // map (Qt::Key_Break       , "break" );
  // map (Qt::Key_User        , "user" );
  // map (Qt::Key_System      , "system" );
  // map (Qt::Key_Reset       , "reset" );
  // map (Qt::Key_ClearLine   , "clear" );
  // map (Qt::Key_ClearDisplay, "cleardisplay" );
  // map (Qt::Key_InsertLine  , "insertline" );
  // map (Qt::Key_DeleteLine  , "deleteline" );
  // map (Qt::Key_InsertChar  , "insert" );
  // map (Qt::Key_DeleteChar  , "delete" );
  // map (Qt::Key_Prev        , "prev" );
  // map (Qt::Key_Next        , "next" );
  // map (Qt::Key_Undo        , "undo" );
  // map (Qt::Key_Redo        , "redo" );
  // map (Qt::Key_Find        , "find" );
  // map (Qt::Key_ModeSwitchFunctionKey, "modeswitch" );
}
#if defined(OS_MINGW) || defined(OS_WIN)
enum WindowsNativeModifiers {
  ShiftLeft   = 0x00000001,
  ControlLeft = 0x00000002,
  AltLeft     = 0x00000004,
  MetaLeft    = 0x00000008,
  ShiftRight  = 0x00000010,
  ControlRight= 0x00000020,
  AltRight    = 0x00000040,
  MetaRight   = 0x00000080,
  CapsLock    = 0x00000100,
  NumLock     = 0x00000200,
  ScrollLock  = 0x00000400,
  ExtendedKey = 0x01000000,
};
#endif

/******************************************************************************
 * Debugging
 ******************************************************************************/

void
qt_dump (QObject* obj, int indent) {
  if (obj == NULL) {
    cout << "NULL\n";
    return;
  }
  for (int j= 0; j < indent; ++j)
    cout << "  ";
  cout << from_qstring (obj->metaObject ()->className ()) << ":\n";
  foreach (QObject* child, obj->children ()) {
    qt_dump (child, indent + 1);
  }
}

tm_ostream&
operator<< (tm_ostream& out, QRect rect) {
  return out << "(" << rect.x () << "," << rect.y () << "," << rect.width ()
             << "," << rect.height () << ")";
}

/******************************************************************************
 * Conversion of data types
 ******************************************************************************/

QFont
to_qfont (int style, QFont font) {
  if ((style & WIDGET_STYLE_MINI) && tm_style_sheet == "" && use_mini_bars) {
    // Use smaller text font
    int fs= as_int (get_preference ("gui:mini-fontsize", QTM_MINI_FONTSIZE));
    font.setPointSize (qt_zoom (fs > 0 ? fs : QTM_MINI_FONTSIZE));
  }
  if (style & WIDGET_STYLE_MONOSPACED) { // Use monospaced font
    font.setFixedPitch (true);           // FIXME: ignored for fonts in QActions
#if (QT_VERSION >= 0x040800)
    font.setStyleHint (QFont::Monospace);
#endif
  }
  if (style & WIDGET_STYLE_GREY)    // use grey text font
    font.setWeight (QFont::Light);  // FIXME: this is only an approximation
  if (style & WIDGET_STYLE_PRESSED) // Button is currently pressed
  {
  }
  if (style & WIDGET_STYLE_INERT) // Only render, don't associate any action
  {
  }
  if (style & WIDGET_STYLE_BUTTON) // Render button as standard button
  {
  }
  if (style & WIDGET_STYLE_BOLD) font.setBold (true);
  return font;
}

double
em_factor () {
#if (QT_VERSION < 0x050000)
  if (tm_style_sheet == "") return 1.0 / 1.4;
#endif
  return 1.0;
}

/*! Try to convert a TeXmacs length (em, px, w, h) into a QSize.

 This uses the widget's current size to compute relative sizes as specified
 with "FFw", where FF is the string representation of a double.
 A value of "1w" should not affect the widget size.

 FIXME: does 1w mean 100% of the contents' size or 100% of the available size?
 */
QSize
qt_decode_length (string width, string height, const QSize& ref,
                  const QFontMetrics& fm) {
  QSize size= ref;

  string w_unit, h_unit;
  double w_len, h_len;
  parse_length (width, w_len, w_unit);
  parse_length (height, h_len, h_unit);

  // Width as a function of the default width
  if (w_unit == "w") size.rwidth ()*= w_len;
  // Width as a function of the default height
  else if (w_unit == "h") size.rwidth ()= w_len * size.height ();
  // Absolute EM units (temporarily fixed to 14px)
  else if (w_unit == "em") {
    // size.setWidth (em_factor()* w_len * fm.width("M"));
    if (tm_style_sheet != "") w_len*= 14 * retina_scale;
    else if (retina_zoom == 2) w_len*= 21;
    else w_len*= 14;
    size.setWidth (w_len);
  }
  // Absolute pixel units
  else if (w_unit == "px") {
    if (tm_style_sheet != "") w_len*= retina_scale;
    else if (retina_zoom == 2) w_len*= 1.5;
    size.setWidth (w_len);
  }

  // Height as a function of the default width
  if (h_unit == "w") size.rheight ()= h_len * size.width ();
  // Height as a function of the default height
  else if (h_unit == "h") size.rheight ()*= h_len;
  else if (h_unit == "em") {
    // size.setHeight (em_factor()* h_len * fm.width("M"));
    if (tm_style_sheet != "") h_len*= 14 * retina_scale;
    else if (retina_zoom == 2) h_len*= 21;
    else w_len*= 14;
    size.setHeight (h_len);
  }
  else if (h_unit == "px") {
    if (tm_style_sheet != "") h_len*= retina_scale;
    else if (retina_zoom == 2) h_len*= 1.5;
    size.setHeight (h_len);
  }
  return size;
}

// used only by to_qkeysequence
static string
conv_sub (const string& ks) {
  string r (ks);
  r= replace (r, "pageup", "pgup");
  r= replace (r, "pagedown", "pgdown");
  r= replace (r, "S-", "Shift+");
  r= replace (r, "A-", "Alt+");
  // r = replace (r, "K-", "");
#ifdef Q_OS_MAC
  r= replace (r, "C-", "Meta+");
  r= replace (r, "M-", "Ctrl+");
#else
  r= replace (r, "C-", "Ctrl+");
  r= replace (r, "M-", "Meta+");
#endif
  array<string> a= tokenize (r, " ");
  for (int i= 0; i < N (a); ++i) {
    int pos= -1, tmp= 0, n= N (a[i]);
    while (tmp < n && (tmp= search_forwards ("+", tmp, a[i])) != -1)
      pos= tmp++;
    if (n > pos + 1) {
      if (is_locase (a[i][pos + 1]))
        a[i]= a[i](0, pos) * upcase_all (a[i](pos, n));
      else if (is_upcase (a[i][pos + 1])) {
        // HACK: don't prepend Shift for function keys F1, F2...
        if (n > pos + 2 && a[i][pos + 1] == 'F' && as_int (a[i][pos + 2]) > 0)
          ;
        else a[i]= a[i](0, pos + 1) * "Shift+" * upcase_all (a[i](pos + 1, n));
      }
    }
  }
  return recompose (a, ",");
}

QKeySequence
to_qkeysequence (string ks) {
  string r (conv_sub (ks));
  if (DEBUG_QT && N (r) > 0) {
    QKeySequence qks (to_qstring (r));
    debug_qt << "ks: " << ks << " --> " << r << " --> "
             << qks.toString (QKeySequence::NativeText).toLatin1 ().data ()
             << LF;
    return qks;
  }
  if (ends (r, "&")) {
    // HACK: because Qt menu can not show the key &
    r= r (0, N (r) - 1) * "Shift+7";
  }
  return QKeySequence (to_qstring (r));
}

tm_ostream&
operator<< (tm_ostream& out, coord4 c) {
  out << "[" << c.x1 << "," << c.x2 << "," << c.x3 << "," << c.x4 << "]";
  return out;
}

tm_ostream&
operator<< (tm_ostream& out, coord2 c) {
  out << "[" << c.x1 << "," << c.x2 << "]";
  return out;
}

coord4
from_qrect (const QRect& rect) {
  SI c1, c2, c3, c4;
  c1= rect.x () * SCREEN_PIXEL;
  c2= -(rect.y () + rect.height ()) * SCREEN_PIXEL;
  c3= (rect.x () + rect.width ()) * SCREEN_PIXEL;
  c4= -rect.y () * SCREEN_PIXEL;
  return coord4 (c1, c2, c3, c4);
}

/*! Transforms a rectangle given by its lower left and upper right corners
 into one given by its upper left and width/height */
QRect
to_qrect (const coord4& p) {
  float c= 1.0 / SCREEN_PIXEL;
  return QRect (p.x1 * c, -p.x4 * c, (p.x3 - p.x1 + SCREEN_PIXEL - 1) * c,
                (p.x4 - p.x2 + SCREEN_PIXEL - 1) * c);
}

coord2
from_qsize (const QSize& s) {
  return coord2 (s.width () * SCREEN_PIXEL, s.height () * SCREEN_PIXEL);
}

QSize
to_qsize (const coord2& p) {
  float c= 1.0 / SCREEN_PIXEL;
  return QSize (p.x1 * c, p.x2 * c);
}

QSize
to_qsize (const SI& w, const SI& h) {
  float c= 1.0 / SCREEN_PIXEL;
  return QSize (w * c, h * c);
}

coord2
from_qpoint (const QPoint& pt) {
  return coord2 (pt.x () * SCREEN_PIXEL, -pt.y () * SCREEN_PIXEL);
}

/*! Transforms texmacs coordinates, with origin at the lower left corner, into
 Qt coordinates, with origin at the upper left corner */
QPoint
to_qpoint (const coord2& p) {
  float c= 1.0 / SCREEN_PIXEL;
  return QPoint (p.x1 * c, -p.x2 * c);
}

array<string>
from_qstringlist (const QStringList& l) {
  array<string> tl (l.size ());
  for (QStringList::const_iterator it= l.begin (); it != l.end (); ++it)
    tl << from_qstring (*it);
  return tl;
}

QStringList
to_qstringlist (array<string> l) {
  QStringList ql;
  for (int i= 0; i < N (l); ++i)
    ql << to_qstring (l[i]);
  return ql;
}

/* HACK!!! Something has to be done wrt. to internal encoding: most of the times
 it's cork, but often it's utf8. For instance when the title is built in a tmfs
 title handler in scheme, it is sent to us as an utf8 string. Should we convert
 before? Another example are items in the go-menu: file names are internally
 stored using the os 8-bit encoding (UTF-8 on linux/Mac OS locale code page on
 windows), but we assume that strings are sent to us for display in widgets as
 cork and thus display the wrong encoding.

 It gets tricky soon, so for the time being we use this hack.
 */
QString // uses heuristics
to_qstring (const string& s) {
  if (looks_utf8 (s) && !(looks_ascii (s) || looks_universal (s)))
    return utf8_to_qstring (s);
  else return utf8_to_qstring (cork_to_utf8 (s));
}

string
from_qstring (const QString& s) {
  return utf8_to_cork (from_qstring_utf8 (s));
}

QString
latin1_to_qstring (const string& s) {
  c_string p (s);
  QString  nss= QString::fromLatin1 (p, N (s));
  return nss;
}

QString
utf8_to_qstring (const string& s) {
  c_string p (s);
  QString  nss= QString::fromUtf8 (p, N (s));
  return nss;
}

string
from_qstring_utf8 (const QString& s) {
  QByteArray  arr = s.toUtf8 ();
  const char* cstr= arr.constData ();
  return string ((char*) cstr);
}

// This should provide better lookup times
static QHash<QString, QColor> _NamedColors;

/*!
  Takes either an hexadecimal RGB color, as in #e3a1ff, or a named color
  as those defined in src/Graphics/Renderer/ * _colors.hpp and returns a QColor
 */
QColor
to_qcolor (const string& col) {
  QString _col= to_qstring (col);
  if (_NamedColors.contains (_col)) return _NamedColors[_col];
  color c= named_color (col);
  if (c == 0 && locase_all (col) != "black") {
    if (DEBUG_QT_WIDGETS)
      debug_widgets << "to_qcolor(" << col << "): "
                    << "name is not defined.\n";
    return QColor (100, 100, 100); // FIXME?
  }
  QColor _c= to_qcolor (c);
  _NamedColors.insert (_col, _c);
  return _c;
}

/*! Returns a color encoded as a string with hexadecimal RGB values,
 as in #e3a1ff
 */
string
from_qcolor (const QColor& col) {
  return from_qstring (col.name ());
}

QColor
to_qcolor (color c) {
  int r, g, b, a;
  get_rgb_color (c, r, g, b, a);
  if (get_reverse_colors ()) reverse (r, g, b);
  return QColor (r, g, b, a);
}

color
to_color (const QColor& c) {
  int r, g, b, a;
  c.getRgb (&r, &g, &b, &a);
  if (get_reverse_colors ()) reverse (r, g, b);
  return rgb_color (r, g, b, a);
}

/******************************************************************************
 * Image conversion
 ******************************************************************************/

bool
qt_supports (url u) {
  static QList<QByteArray> formats= QImageReader::supportedImageFormats ();
  if (DEBUG_CONVERT) {
    debug_convert << "QT valid formats:";
    foreach (QString _format, formats)
      debug_convert << ", " << from_qstring (_format);
    debug_convert << LF;
  }
  string suf= suffix (u);
  // as of 2023, even if qt claims it can handle pdf, do not use it as it
  // produces blurry pngs see
  // http://forum.texmacs.cn/t/how-are-graphics-supposed-to-look-like/963/12
  if (suf == "pdf") return false;
  bool ans= (bool) formats.contains ((QByteArray) as_charp (suf));
  // if (DEBUG_CONVERT) {debug_convert <<"QT valid
  // format:"<<((ans)?"yes":"no")<<LF;}
  if (DEBUG_CONVERT) {
    debug_convert << "QT valid format:" << ((ans) ? "yes" : "no") << LF;
  }
  return ans;
}

bool
qt_load_image_from_ramdisc (url u, QImage& im) {
  // ramdisc URL 结构：concat(root("ramdisc", data), filename)
  // 在访问树节点子元素前进行边界检查
  if (N (u->t) < 2) {
    im= QImage ();
    return false;
  }
  url root_part= u[1];
  if (N (root_part->t) < 3) {
    im= QImage ();
    return false;
  }
  url    data_url= root_part[2];
  string img_data= data_url->t->label;
  im             = QImage ();
  return im.loadFromData ((const uchar*) img_data.begin (), N (img_data));
}

bool
qt_image_size (url image, int& w, int& h) { // w, h in points
  if (DEBUG_CONVERT) debug_convert << "qt_image_size :" << LF;
  QImage im;
  if (is_ramdisc (image)) {
    qt_load_image_from_ramdisc (image, im);
  }
  else {
    string concrete= concretize (image);
    im             = QImage (utf8_to_qstring (concrete));
  }
  if (im.isNull ()) {
    convert_error << "Cannot read image file '" << image << "'"
                  << " in qt_image_size" << LF;
    w= 35;
    h= 35;
    return false;
  }
  else {
    SI pt= get_current_editor ()->as_length ("1pt");
    SI px= get_current_editor ()->as_length ("1px");
    w    = (int) im.width () * px * 1.0 / pt;
    h    = (int) im.height () * px * 1.0 / pt;
    if (DEBUG_CONVERT)
      debug_convert << "QT image_size (pt): " << w << " x " << h << LF;
    return true;
  }
}

bool
qt_native_image_size (url image, int& w, int& h) {
  if (DEBUG_CONVERT) debug_convert << "qt_image_size :" << LF;
  QImage im;
  if (is_ramdisc (image)) {
    qt_load_image_from_ramdisc (image, im);
  }
  else {
    im= QImage (utf8_to_qstring (concretize (image)));
  }
  if (im.isNull ()) return false;
  else {
    w= im.width ();
    h= im.height ();
    return true;
  }
}

void
qt_pretty_image_size (int ww, int hh, string& w, string& h) {
  SI pt = get_current_editor ()->as_length ("1pt");
  SI par= get_current_editor ()->as_length ("1par");
  if (ww <= 0 || hh <= 0) {
    w= "";
    h= "";
  }
  else if (ww * pt > par) {
    w= "0.8par";
    h= as_string (0.8 / ww * hh) * "par";
  }
  else {
    w= as_string (ww) * "pt";
    h= as_string (hh) * "pt";
  }
}

bool
qt_pretty_image_size (url image, string& w, string& h) {
  if (suffix (image) == "pdf" || suffix (image) == "ps" ||
      suffix (image) == "eps") {
    w= "";
    h= "";
    return false;
  }
  int  ww, hh;
  bool r= qt_image_size (image, ww, hh);
  qt_pretty_image_size (ww, hh, w, h);
  return r;
}

void
qt_convert_image (url image, url dest, int w, int h) { // w, h in pixels
  if (DEBUG_CONVERT)
    debug_convert << "qt_convert_image " << image << " -> " << dest << LF;
  QImage im;
  if (is_ramdisc (image)) {
    qt_load_image_from_ramdisc (image, im);
  }
  else {
    im= QImage (utf8_to_qstring (concretize (image)));
  }
  if (im.isNull ())
    convert_error << "Cannot read image file '" << image << "'"
                  << " in qt_convert_image" << LF;
  else {
    if (w > 0 && h > 0)
      im= im.scaled (w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    im.scaled (w, h).save (utf8_to_qstring (concretize (dest)));
  }
}

#ifdef USE_QT_PRINTER
void
qt_image_to_pdf (url image, url outfile, int w_pt, int h_pt, int dpi) {
  // use a QPrinter to output raster images to eps or pdf
  // dpi is the maximum dpi : the image will either be dowsampled to that dpi
  // or the actual dpi will be lower
  if (DEBUG_CONVERT)
    debug_convert << "qt_image_to_eps_or_pdf " << image << " -> " << outfile
                  << LF;
  QPrinter printer;
#if QT_VERSION < 0x060000
  printer.setOrientation (QPrinter::Portrait);
#else
  printer.setPageOrientation (QPageLayout::Portrait);
#endif
  if (suffix (outfile) == "eps") {
#if (QT_VERSION >= 0x050000)
    // note that PostScriptFormat is gone in Qt5. a substitute?:
    // http://soft.proindependent.com/eps/
    cout << "TeXmacs] warning: PostScript format no longer supported in Qt5\n";
    printer.setOutputFormat (QPrinter::PdfFormat);
#else
    printer.setOutputFormat (QPrinter::PostScriptFormat);
#endif
  }
  else printer.setOutputFormat (QPrinter::PdfFormat);
  printer.setFullPage (true);
  if (!dpi) dpi= 96;
  printer.setResolution (dpi);
  printer.setOutputFileName (utf8_to_qstring (concretize (outfile)));
  QImage im (utf8_to_qstring (concretize (image)));
  if (im.isNull ()) {
    convert_error << "Cannot read image file '" << image << "'"
                  << " in qt_image_to_pdf" << LF;
    // load the "?" image?
  }
  else {
    /*  if (DEBUG_CONVERT) debug_convert << "size asked " << w_pt << "x"<<h_pt
      << " at " << maximum dpi <<" dpi"<<LF
      << "dpi set: " << printer.resolution() <<LF;
    */
    if (dpi > 0 && w_pt > 0 && h_pt > 0) {

#if QT_VERSION < 0x060000
      printer.setPaperSize (QSizeF (w_pt, h_pt), QPrinter::Point); // in points
#else
      printer.setPageSize (QPageSize (QSizeF (w_pt, h_pt), QPageSize::Point));
#endif

      // w_pt and h_pt are dimensions in points (and there are 72 points per
      // inch)
      int ww= w_pt * dpi / 72;
      int hh= h_pt * dpi / 72;
      if ((ww < im.width ()) ||
          (hh < im.height ())) // downsample if possible to reduce file size
        im= im.scaled (ww, hh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
      else // image was too small, reduce dpi accordingly to fill page
        printer.setResolution ((int) (dpi * im.width ()) / (double) ww);
      if (DEBUG_CONVERT)
        debug_convert << "dpi asked: " << dpi
                      << " ; actual dpi set: " << printer.resolution () << LF;
    }
#if QT_VERSION < 0x060000
    else
      printer.setPaperSize (QSizeF (im.width (), im.height ()),
                            QPrinter::DevicePixel);
#else
    else
      printer.setPageSize (
          QPageSize (QSizeF (im.width (), im.height ()), QPageSize::Point));
#endif
    QPainter p;
    p.begin (&printer);
    p.drawImage (0, 0, im);
    p.end ();
  }
}
#else
void
qt_image_to_pdf (url image, url outfile, int w_pt, int h_pt, int dpi) {
  if (DEBUG_CONVERT)
    debug_convert << "NOT SUPPORTED: qt_image_to_eps_or_pdf " << image << " -> "
                  << outfile << LF;
}
#endif // defined(USE_QT_PRINTER)

QPixmap
as_pixmap (const QImage& im) {
  QPixmap pm (im.size ());
#if (QT_VERSION >= 0x040700)
  pm.convertFromImage (im);
#else
  pm.fromImage (im);
#endif
  return pm;
}

/******************************************************************************
 * Stuff related to widgets
 ******************************************************************************/

QString
parse_tm_style (int style) {
  QString sheet;
  if ((style & WIDGET_STYLE_MINI) && tm_style_sheet == "" && use_mini_bars) {
    // Use smaller text font
    int fs= as_int (get_preference ("gui:mini-fontsize", QTM_MINI_FONTSIZE));
    sheet+= QString ("font-size: %1pt;").arg (fs > 0 ? fs : QTM_MINI_FONTSIZE);
    sheet+= QString ("padding: 1px;");
  }
  if (style & WIDGET_STYLE_MONOSPACED) // Use monospaced font
    sheet+= "font-family: \"monospace\";";
  if (style & WIDGET_STYLE_GREY) // Use grey text font
    sheet+= "color: #414141;";
  if (style & WIDGET_STYLE_PRESSED) // Button is currently pressed
    sheet+= "";
  if (style & WIDGET_STYLE_INERT) // Only render, don't associate any action
    sheet+= "color: #414141;";
  if (style & WIDGET_STYLE_BUTTON) // Render button as standard button
    sheet+= "";
  if (style & WIDGET_STYLE_CENTERED) // Use centered text
    sheet+= "text-align: center;";
  if (style & WIDGET_STYLE_BOLD) sheet+= "font-weight: bold;";
  if (DEBUG_QT_WIDGETS) sheet+= "border:1px solid rgb(255, 0, 0);";

  if (occurs ("dark", tm_style_sheet) ||
      occurs ("liii-night", tm_style_sheet)) {
    if (style & WIDGET_STYLE_GREY) sheet+= "color: #a0a0a0;";
    if (style & WIDGET_STYLE_INERT) sheet+= "color: #a0a0a0;";
  }
  return sheet;
}

void
qt_apply_tm_style (QWidget* qwid, int style) {
  QString sheet= "* {" + parse_tm_style (style) + "}";
  qwid->setStyleSheet (sheet);
  qwid->setEnabled (!(style & WIDGET_STYLE_INERT));
}

void
qt_apply_tm_style (QWidget* qwid, int style, color c) {
  int r, g, b, a;
  get_rgb_color (c, r, g, b, a);
  a= a * 100 / 255;
  if (occurs ("dark", tm_style_sheet) ||
      occurs ("liii-night", tm_style_sheet)) {
    r= g= b= 224;
    a      = 100;
  }
  QString sheet= "* {" + parse_tm_style (style) +
                 QString ("color: rgba(%1, %2, %3, %4%);")
                     .arg (r)
                     .arg (g)
                     .arg (b)
                     .arg (a) +
                 "} ";

#ifdef Q_OS_MAC
  /* Disabled QLabels are not greyed out (at least in MacOS, since Qt 4.7.2),
   see: https://bugreports.qt-project.org/browse/QTBUG-19008
   For consistency we set the disabled color for all widgets.
   */
  sheet+= " :disabled { color: #7F7F7F; }";
#endif
  qwid->setEnabled (!(style & WIDGET_STYLE_INERT));
  qwid->setStyleSheet (sheet);
}

QString
qt_translate (const string& s) {
  string in_lan = get_input_language ();
  string out_lan= get_output_language ();
  return to_qstring (tm_var_encode (translate (s, in_lan, out_lan)));
}

string
qt_application_directory () {
  return string (
      QCoreApplication::applicationDirPath ().toLatin1 ().constData ());
  // This is used to set $TEXMACS_PATH
  // in Windows TeXmacs cannot run if this path contains unicode characters
  // apparently because Guile uses standard narrow char api to load its modules
  // => patch Guile?. return from_qstring (QCoreApplication::applicationDirPath
  // ());
}

string
qt_get_date (string lan, string fm) {
  QDateTime localtime= QDateTime::currentDateTime ();
  if (fm == "") {
    if ((lan == "british") || (lan == "english") || (lan == "american"))
      fm= "MMMM d, yyyy";
    else if (lan == "german") fm= "d. MMMM yyyy";
    else if (lan == "chinese" || lan == "japanese" || lan == "korean" ||
             lan == "taiwanese") {
      string y= as_string (localtime.date ().year ());
      string m= as_string (localtime.date ().month ());
      string d= as_string (localtime.date ().day ());
      if (lan == "korean")
        return y * "<#b144> " * m * "<#c6d4> " * d * "<#c77c>";
      return y * "<#5e74>" * m * "<#6708>" * d * "<#65e5>";
    }
    else fm= "d MMMM yyyy";
  }
  else if (fm[0] == '%') {
    char   buf[64];
    time_t ti;
    time (&ti);
    strftime (buf, sizeof (buf), as_charp (fm), ::localtime (&ti));
    return buf;
  }
  QLocale loc= QLocale (to_qstring (language_to_locale (lan)));
#if (QT_VERSION >= 0x040400)
  QString date= loc.toString (localtime, to_qstring (fm));
#else
  QString date= localtime.toString (to_qstring (fm));
#endif
  return from_qstring (date);
}

string
qt_pretty_time (int t) {
#if QT_VERSION >= 0x060000
  QDateTime dt= QDateTime::fromSecsSinceEpoch (t);
#else
  QDateTime dt= QDateTime::fromTime_t (t);
#endif
  QString s= dt.toString ();
  return from_qstring (s);
}

#ifdef USE_QT_PRINTER
#ifndef _MBD_EXPERIMENTAL_PRINTER_WIDGET // this is in qt_printer_widget

#if QT_VERSION >= 0x060000
#define PAPER(fmt)                                                             \
  case QPageSize::fmt:                                                         \
    return "fmt"
static string
qt_papersize_to_string (QPageSize::PageSizeId sz) {
  switch (sz) {
    PAPER (A0);
    PAPER (A1);
    PAPER (A2);
    PAPER (A3);
    PAPER (A4);
    PAPER (A5);
    PAPER (A6);
    PAPER (A7);
    PAPER (A8);
    PAPER (A9);
    PAPER (B0);
    PAPER (B1);
    PAPER (B2);
    PAPER (B3);
    PAPER (B4);
    PAPER (B5);
    PAPER (B6);
    PAPER (B7);
    PAPER (B8);
    PAPER (B9);
    PAPER (B10);
    PAPER (Letter);
  default:
    return "A4";
  }
}
#else
#define PAPER(fmt)                                                             \
  case QPrinter::fmt:                                                          \
    return "fmt"
static string
qt_papersize_to_string (QPrinter::PaperSize sz) {
  switch (sz) {
    PAPER (A0);
    PAPER (A1);
    PAPER (A2);
    PAPER (A3);
    PAPER (A4);
    PAPER (A5);
    PAPER (A6);
    PAPER (A7);
    PAPER (A8);
    PAPER (A9);
    PAPER (B0);
    PAPER (B1);
    PAPER (B2);
    PAPER (B3);
    PAPER (B4);
    PAPER (B5);
    PAPER (B6);
    PAPER (B7);
    PAPER (B8);
    PAPER (B9);
    PAPER (B10);
    PAPER (Letter);
  default:
    return "A4";
  }
}
#endif
#undef PAPER

bool
qt_print (bool& to_file, bool& landscape, string& pname, url& filename,
          string& first, string& last, string& paper_type) {
  static QPrinter* qprinter= NULL;
  if (!qprinter) {
    qprinter= new QPrinter;
  }
  QPrintDialog pdialog (qprinter);
  if (pdialog.exec () == QDialog::Accepted) {
    to_file = !(qprinter->outputFileName ().isNull ());
    pname   = from_qstring (qprinter->printerName ());
    filename= from_qstring (qprinter->outputFileName ());
#if QT_VERSION >= 0x060000
    landscape=
        (qprinter->pageLayout ().orientation () == QPageLayout::Landscape);
    paper_type=
        qt_papersize_to_string (qprinter->pageLayout ().pageSize ().id ());
#else
    landscape = (qprinter->orientation () == QPrinter::Landscape);
    paper_type= qt_papersize_to_string (qprinter->paperSize ());
#endif
    if (qprinter->printRange () == QPrinter::PageRange) {
      first= qprinter->fromPage ();
      last = qprinter->toPage ();
    }
    // cout << "Printer :" << pname << LF;
    // cout << "File :" << filename << LF;
    return true;
  }
  return false;
}

#endif //(not defined) _MBD_EXPERIMENTAL_PRINTER_WIDGET
#else
bool
qt_print (bool& to_file, bool& landscape, string& pname, url& filename,
          string& first, string& last, string& paper_type) {
  return false;
}
#endif // defined(USE_QT_PRINTER)

#ifdef OS_MACOS

// Additional utilities for MACOS
// this part has to be at the end because it imports CoreFoundation definitions
// which interfere with TeXmacs and QT types...

#define extend CFextend   // avoid name clashes...
#define outline CFoutline // avoid name clashes...
#include <CoreFoundation/CoreFoundation.h>
#undef extend

// HACK: this function is needed on MacOS when dropping URLS
// which could not correspond to standard Unix paths

QString
fromNSUrl (const QUrl& url) {
  QString localFileQString= url.toLocalFile ();
  // [pzion 20150805] Work around
  // https://bugreports.qt.io/browse/QTBUG-40449
  if (localFileQString.startsWith ("/.file/id=")) {
    CFStringRef relCFStringRef= CFStringCreateWithCString (
        kCFAllocatorDefault, localFileQString.toUtf8 ().constData (),
        kCFStringEncodingUTF8);
    CFURLRef relCFURL= CFURLCreateWithFileSystemPath (
        kCFAllocatorDefault, relCFStringRef, kCFURLPOSIXPathStyle,
        false); // isDirectory
    CFErrorRef error= 0;
    CFURLRef   absCFURL=
        CFURLCreateFilePathURL (kCFAllocatorDefault, relCFURL, &error);
    if (!error) {
      static const CFIndex maxAbsPathCStrBufLen= 4096;
      char                 absPathCStr[maxAbsPathCStrBufLen];
      if (CFURLGetFileSystemRepresentation (
              absCFURL,
              true, // resolveAgainstBase
              reinterpret_cast<UInt8*> (&absPathCStr[0]),
              maxAbsPathCStrBufLen)) {
        localFileQString= QString (absPathCStr);
      }
    }
    CFRelease (absCFURL);
    CFRelease (relCFURL);
    CFRelease (relCFStringRef);
  }
  return localFileQString;
}
#endif // OS_MACOS

/******************************************************************************
 * Style sheets
 ******************************************************************************/

static string current_style_sheet;

void
init_palette (QApplication* app) {
  if (occurs ("dark", tm_style_sheet) ||
      occurs ("liii-night", tm_style_sheet)) {
    QPalette pal= app->style ()->standardPalette ();
    pal.setColor (QPalette::Window, QColor (64, 64, 64));
    pal.setColor (QPalette::WindowText, QColor (224, 224, 224));
    pal.setColor (QPalette::Base, QColor (96, 96, 96));
    pal.setColor (QPalette::Text, QColor (224, 224, 224));
    pal.setColor (QPalette::ButtonText, QColor (224, 224, 224));
    pal.setColor (QPalette::Light, QColor (64, 64, 64));
    pal.setColor (QPalette::Midlight, QColor (96, 96, 96));
    pal.setColor (QPalette::Dark, QColor (112, 112, 112));
    pal.setColor (QPalette::Mid, QColor (128, 128, 128));
    pal.setColor (QPalette::Shadow, QColor (240, 240, 240));
    pal.setColor (QPalette::HighlightedText, QColor (48, 48, 48));
    app->setPalette (pal);
  }
  else if (tm_style_sheet != "" && !occurs ("native", tm_style_sheet)) {
    QPalette pal= app->style ()->standardPalette ();
    pal.setColor (QPalette::Window, QColor (232, 232, 232));
    pal.setColor (QPalette::WindowText, QColor (0, 0, 0));
    pal.setColor (QPalette::Base, QColor (255, 255, 255));
    pal.setColor (QPalette::Text, QColor (0, 0, 0));
    pal.setColor (QPalette::ButtonText, QColor (0, 0, 0));
    pal.setColor (QPalette::Light, QColor (240, 240, 240));
    pal.setColor (QPalette::Midlight, QColor (224, 224, 224));
    pal.setColor (QPalette::Dark, QColor (192, 192, 192));
    pal.setColor (QPalette::Mid, QColor (160, 160, 160));
    pal.setColor (QPalette::Shadow, QColor (0, 0, 0));
    pal.setColor (QPalette::HighlightedText, QColor (255, 255, 255));
    app->setPalette (pal);
  }

  if (occurs ("dark", tm_style_sheet) || occurs ("liii-night", tm_style_sheet))
    tm_background= rgb_color (32, 32, 32);
  else if (occurs ("native", tm_style_sheet)) {
    QPalette pal = app->palette ();
    QColor   col = pal.color (QPalette::Mid);
    tm_background= rgb_color (col.red (), col.green (), col.blue ());
  }
  else if (tm_style_sheet != "") tm_background= rgb_color (160, 160, 160);
}

string
scale_px (string s) {
  string r;
  for (int i= 0; i < N (s);)
    if (s[i] == ':') {
      r << s[i++];
      while (i < N (s) && s[i] == ' ')
        r << s[i++];
      int j= i;
      if (j < N (s) && s[j] == '-') j++;
      while (j < N (s) && (is_numeric (s[j]) || s[j] == '.'))
        j++;
      if (test (s, j, "px;") || test (s, j, "px ")) {
        double x = as_double (s (i, j));
        int    nx= (int) floor (x * retina_scale + 0.5);
        r << as_string (nx);
        i= j;
      }
    }
    else r << s[i++];
  return r;
}

void
init_style_sheet (QApplication* app) {
  string ss;
  url    css (tm_style_sheet);
  if (!exists (css)) {
    if (suffix (css) == "") css= glue (css, ".css");
    url dir ("$TEXMACS_THEME_PATH");
    css= resolve (dir * css);
    if (is_none (css)) return;
  }
  if (tm_style_sheet != "" && !load_string (css, ss, false)) {
    string p= as_string (url ("$TEXMACS_PATH"));
#ifdef Q_OS_WIN
    p= replace (p, "\\", "/");
#endif
    ss= replace (ss, "\n", " ");
    ss= replace (ss, "\t", " ");
    ss= replace (ss, "$TEXMACS_PATH", p);
#if (QT_VERSION < 0x050000)
    ss= replace (ss, "Qt4", "");
#endif
#ifdef OS_MACOS
    ss= replace (ss, "Macos", "");
#else
    ss= replace (ss, "Nomac", "");
#endif
#if defined(OS_MINGW) || defined(OS_WIN)
    ss= replace (ss, "Mingw", "");
#endif
#ifdef OS_GNU_LINUX
    ss= replace (ss, "Linux", "");
#endif
    if (use_unified_toolbar) ss= replace (ss, "Uni", "");
    else ss= replace (ss, "Nonuni", "");
    if (!use_unified_toolbar ||
        get_preference ("main icon bar", "off") != "off")
      ss= replace (ss, "Nounim", "");
    ss                 = scale_px (ss);
    current_style_sheet= ss;
    app->setStyleSheet (to_qstring (current_style_sheet));
  }
}

void
set_standard_style_sheet (QWidget* w) {
  if (current_style_sheet != "")
    w->setStyleSheet (to_qstring (current_style_sheet));
}

string
from_modifiers (Qt::KeyboardModifiers mods) {
  string r;

  if (mods & Qt::ShiftModifier) r= "S-" * r;
  if (mods & Qt::AltModifier) r= "A-" * r;
  if (os_macos ()) {
    if (mods & Qt::MetaModifier) r= "C-" * r;    // The "Control" key
    if (mods & Qt::ControlModifier) r= "M-" * r; // The "Command" key
  }
  else {
    if (mods & Qt::ControlModifier) r= "C-" * r;
    if (mods & Qt::MetaModifier) r= "M-" * r; // The "Windows" key
  }
  return r;
}

static bool
is_printable_key (int key) {
  // 32 is space, 127 is delete
  return (key > 32) && (key < 127);
}

string
from_key_press_event (const QKeyEvent* event) {
  int                   key = event->key ();
  QString               nss = event->text ();
  Qt::KeyboardModifiers mods= event->modifiers ();

#if defined(OS_MINGW) || defined(OS_WIN)
  /* "Qt::Key_AltGr On Windows, when the KeyDown event for this key is sent,
   * the Ctrl+Alt modifiers are also set." (excerpt from Qt doc)
   * However the AltGr key is used to obtain many symbols
   * which should not be regarded as C-A- shortcuts.
   * (e.g. \ or @ on a French keyboard)
   *
   * Hence, when "native modifiers" are (ControlLeft | AltRight)
   * we clear Qt's Ctrl+Alt modifiers
   */
  if ((event->nativeModifiers () & (ControlLeft | AltRight)) ==
      (ControlLeft | AltRight)) {
    if (DEBUG_QT && DEBUG_KEYBOARD)
      debug_qt << "assuming it's an AltGr key code" << LF;
    mods&= ~Qt::AltModifier;
    mods&= ~Qt::ControlModifier;
  }
#endif

  string mods_text= from_modifiers (mods);

  // See https://doc.qt.io/qt-6/qt.html#Key-enum
  // cout << "key:\t0x" << locase_all (as_hexadecimal (key, 8)) << LF;
  // if (is_printable_key (key)) {
  //   cout << "raw:\t" << mods_text << (char) key << LF;
  // }
  // else {
  //   cout << "mods:\t" << mods_text << LF;
  // }
  // cout << "text(" << nss.count () << "):\t" << from_qstring_utf8 (nss) << LF;
  // cout << LF;

  string r= "";
  if (is_printable_key (key)) {
    char key_original= (char) key;
    char key_locased = locase (key);

    // With CapsLock, we should use the text_key
    unsigned short unic    = nss.data ()[0].unicode ();
    char           text_key= (char) unic;

    if (is_empty (mods_text) || mods_text == "S-") {
      // a-z, A-Z, 0-9, and others
      r << text_key;
    }
    else if (is_locase (key_original)) {
      debug_keyboard << mods_text << key_original << " should be invalid" << LF;
      r= "";
    }
    else if (mods_text == "A-") {
      // A-: Alt+key or Option+key
      if (os_macos () && !is_alpha (key_original) && !is_digit (key_original) &&
          !nss.isEmpty ()) {
        // there are special rules for the key combo with Option on macOS:
        // A-<    ->    ≤
        // A->    ->    ≥
        // A-6    ->    §
        // A-p    ->    π
        //
        // case 1: for key which is alpha or number
        // there are also special rules for the key combo with Option on macOS
        // but we have defined A-1, A-2, A-v in TeXmacs
        // for defined A-1 (¡ on macOS), we override it using the TeXmacs one
        // for undefined A-6 (§ on macoS), we ignore it
        //
        // case 2: for key which is not alpha and not number: -, =, ,, ., /, ...
        // we reserve the special rules here
        r << from_qstring_utf8 (nss);
      }
      else {
        r << mods_text << key_locased;
      }
    }
    else if (mods_text == "C-" || mods_text == "M-") {
      // C-: Ctrl+key or Ctrl+key
      // M-: Win+key or Command+key
      r << mods_text << key_locased;
    }
    else if (mods_text == "C-A-") {
      r << mods_text << key_locased;
    }
    else if (mods_text == "A-S-") {
      // A-+, A-{, A-?, ...
      r << "A-" << key_original;
    }
    else if (mods_text == "C-S-") {
      // C-+, C-{, C-?, ...
      r << "C-" << key_original;
    }
    else if (mods_text == "M-S-") {
      if (is_alpha (key_original)) {
        // M-A, M-B, ..., M-Z
        r << "M-" << key_original;
      }
      else {
        if (os_macos ()) {
          // M-+ for Qt 6.5 on macOS is M-S-=
          r << "M-S-" << key_original;
        }
        else {
          r << "M-" << key_original;
        }
      }
    }
    else if (mods_text == "C-A-S-") {
      r << "C-A-" << key_original;
    }
    else {
      // NOTE: Qt 6 on macOS, M-A- and M-C- is handled in keyReleaseEvent
      r << mods_text << key_locased;
    }
  }
  else {
    initkeymap ();
    if (qtkeymap->contains (key)) { // Case 1: in qtkeymap
      r= mods_text * qtkeymap[key];
    }
    else if (qtdeadmap->contains (key)) { // Case 2: in qtdeadmap
      mods&= ~Qt::ShiftModifier;
      r= mods_text * qtdeadmap[key];
    }
    else if (is_empty (mods_text) ||
             mods_text == "S-") { // Case 3: Shift or not
      r= from_qstring_utf8 (nss);
    }
    else { // Case 4: Unicode < 256
      // We need to use text(): Alt-{5,6,7,8,9} are []|{} under MacOS, etc.
      unsigned short unic= nss.data ()[0].unicode ();
      if (unic > 32 && unic < 255 && (mods & Qt::ShiftModifier) != 0 &&
          (mods & Qt::ControlModifier) == 0 && (mods & Qt::AltModifier) == 0 &&
          (mods & Qt::MetaModifier) == 0) {
#ifdef Q_OS_WIN
        if ((unic > 0 && unic < 32 && key > 0 && key < 128) ||
            (unic > 0 && unic < 255 && key > 32 &&
             (mods & Qt::ShiftModifier) != 0 &&
             (mods & Qt::ControlModifier) != 0)) {
#else
        if (unic < 32 && key > 0 && key < 128) {
#endif
          mods&= ~Qt::ShiftModifier;
          r= string ((char) key);
        }
        else {
          switch (unic) {
          case 96:
            r= "`";
            // unicode to cork conversion not appropriate for this case...
#ifdef Q_OS_MAC
            // CHECKME: are these two MAC exceptions really needed?
            if (mods & Qt::AltModifier) r= "grave";
#endif
            break;
          case 168:
            r= "umlaut";
            break;
          case 180:
            r= "acute";
            break;
            // the following combining characters should be caught by qtdeadmap
          case 0x300:
            r= "grave";
            break;
          case 0x301:
            r= "acute";
            break;
          case 0x302:
            r= "hat";
            break;
          case 0x308:
            r= "umlaut";
            break;
          case 0x33e:
            r= "tilde";
            break;
          default:
            r= from_qstring_utf8 (nss);
          }
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
          if (os_macos () && (mods & Qt::AltModifier)) {
            // Alt produces many symbols in Mac keyboards: []|{} etc.
            if ((N (r) != 1 || ((int) (unsigned char) r[0]) < 32 ||
                 ((int) (unsigned char) r[0]) >= 128) &&
                key >= 32 && key < 128 &&
                ((mods & (Qt::MetaModifier + Qt::ControlModifier)) == 0)) {
              if ((mods & Qt::ShiftModifier) == 0 && key >= 65 && key <= 90)
                key+= 32;
              qtcomposemap (key)= r;
              r                 = string ((char) key);
            }
            else mods&= ~Qt::AltModifier; // unset Alt
          }
#endif
          if (!is_empty (r)) {
            r= mods_text * r;
          }
        }
      }
    } // Case 4: Unicode < 256 (endif)
  }
  return r;
}

string
from_key_release_event (const QKeyEvent* event) {
  string r;

  if (os_macos ()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    int                   key      = event->key ();
    Qt::KeyboardModifiers mods     = event->modifiers ();
    string                mods_text= from_modifiers (mods);

    // see https://bugreports.qt.io/browse/QTBUG-115525
    // This branch is only for Option-Command-x or Ctrl-Command-x
    if (mods_text == "M-A-" || mods_text == "M-C-") {
      if (is_printable_key (key)) {
        char key_c= (char) key;
        if ((mods & Qt::ShiftModifier) == 0 && is_upcase (key_c)) {
          key_c= locase (key_c);
        }
        r= string (key_c);
      }
      else if (qtkeymap->contains (key)) {
        r= qtkeymap[key];
      }
      else {
        return "";
      }
      r= mods_text * r;
    }
#endif
  }

  return r;
}

/******************************************************************************
 * Clipboard format detection
 ******************************************************************************/

string
qt_clipboard_format () {
  QCoreApplication::processEvents (); // 处理挂起的事件
  QClipboard*      clipboard= QApplication::clipboard ();
  const QMimeData* mimeData = clipboard->mimeData ();

  if (!mimeData) {
    if (DEBUG_QT)
      debug_qt << "qt_clipboard_format: clipboard is empty or inaccessible\n";
    return "";
  }

  // 定义格式优先级（从高到低）
  // 图像格式优先级最高
  if (mimeData->hasImage ()) {
    // 尝试确定具体的图像格式
    QStringList formats= mimeData->formats ();
    for (const QString& format : formats) {
      if (format.startsWith ("image/")) {
        // 返回具体的图像格式，如 image/jpg, image/png 等
        return from_qstring (format);
      }
    }
    // 如果没有具体的图像格式信息，返回通用的 image
    return "image";
  }

  // 然后是 html
  if (mimeData->hasHtml ()) {
    return "html";
  }

  // 然后是 texmacs-clipboard （内部格式）
  if (mimeData->hasFormat ("application/x-texmacs-clipboard")) {
    return "texmacs-snippet";
  }

  // 然后是 verbatim（纯文本）
  if (mimeData->hasText ()) {
    return "verbatim";
  }

  // 最后是 url
  if (mimeData->hasUrls ()) {
    return "url";
  }

  // 如果都没有找到，返回空字符串
  if (DEBUG_QT) debug_qt << "qt_clipboard_format: no supported format found\n";
  return "";
}

string
qt_clipboard_text () {
  QCoreApplication::processEvents (); // 处理挂起的事件
  QClipboard*      clipboard= QApplication::clipboard ();
  const QMimeData* mimeData = clipboard->mimeData ();
  if (mimeData->hasFormat ("application/x-texmacs-clipboard")) {
    QByteArray buf= mimeData->data ("application/x-texmacs-clipboard");
    return string (buf.constData (), buf.size ());
  }
  if (mimeData->hasImage ()) {
    return "";
  }
  QByteArray buf;
  buf= mimeData->text ().toUtf8 ();
  return string (buf.constData (), buf.size ());
}
