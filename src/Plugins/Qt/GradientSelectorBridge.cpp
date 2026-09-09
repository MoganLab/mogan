/******************************************************************************
 * MODULE      : GradientSelectorBridge.cpp
 * DESCRIPTION : QML 渐变选择器对话框 Bridge 实现。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#include "GradientSelectorBridge.hpp"
#include "QTMQmlDialog.hpp"
#include "qt_utilities.hpp"
#include "tm_sys_utils.hpp"

#include <QBuffer>
#include <QColor>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>

static QString
resolvePatternPath (const QString& name) {
  if (name.isEmpty ()) return "";
  QFileInfo fi (name);
  if (fi.isAbsolute () && fi.exists ()) return name;

  string  tm_path= as_string (get_texmacs_path ());
  QString prefix = to_qstring (tm_path);
  QString c1     = prefix + "/misc/pictures/gradients/" + name;
  if (QFileInfo::exists (c1)) return c1;
  QString c2= prefix + "/misc/patterns/" + name;
  if (QFileInfo::exists (c2)) return c2;
  return name;
}

static QColor
parseColor (const QString& str) {
  if (str.isEmpty ()) return QColor (0, 0, 0);
  if (str.startsWith ('#') || QColor::isValidColorName (str)) {
    QColor c (str);
    if (c.isValid ()) return c;
  }
  return to_qcolor (from_qstring (str));
}

GradientSelectorBridge::GradientSelectorBridge (
    QDialog* host, const QString& name, const QString& width,
    const QString& height, const QString& fg, const QString& bg,
    QObject* parent)
    : QObject (parent), m_host (host),
      m_patternName (name.isEmpty () ? "vertical-white-black.png" : name),
      m_width (width.isEmpty () ? "100%" : width),
      m_height (height.isEmpty () ? "100%" : height),
      m_fgColor (fg.isEmpty () ? "black" : fg),
      m_bgColor (bg.isEmpty () ? "white" : bg), m_submitted (false) {
  updatePreview ();
}

void
GradientSelectorBridge::setPatternName (const QString& name) {
  if (m_patternName != name) {
    m_patternName= name;
    emit patternNameChanged ();
    updatePreview ();
  }
}

void
GradientSelectorBridge::setWidth (const QString& w) {
  if (m_width != w) {
    m_width= w;
    emit widthChanged ();
  }
}

void
GradientSelectorBridge::setHeight (const QString& h) {
  if (m_height != h) {
    m_height= h;
    emit heightChanged ();
  }
}

void
GradientSelectorBridge::setForegroundColor (const QString& c) {
  if (m_fgColor != c) {
    m_fgColor= c;
    emit foregroundColorChanged ();
    updatePreview ();
  }
}

void
GradientSelectorBridge::setBackgroundColor (const QString& c) {
  if (m_bgColor != c) {
    m_bgColor= c;
    emit backgroundColorChanged ();
    updatePreview ();
  }
}

void
GradientSelectorBridge::updatePreview () {
  QString resolved= resolvePatternPath (m_patternName);
  QImage  src (resolved);
  if (src.isNull ()) {
    src= QImage (200, 200, QImage::Format_Grayscale8);
    for (int y= 0; y < 200; ++y) {
      uchar* scan= src.scanLine (y);
      uchar  v   = static_cast<uchar> (255 - (y * 255 / 199));
      memset (scan, v, 200);
    }
  }

  QImage dst= src.convertToFormat (QImage::Format_ARGB32);
  QColor fg = parseColor (m_fgColor);
  QColor bg = parseColor (m_bgColor);
  if (!fg.isValid ()) fg= QColor (0, 0, 0);
  if (!bg.isValid ()) bg= QColor (255, 255, 255);

  int w= dst.width ();
  int h= dst.height ();
  for (int y= 0; y < h; ++y) {
    QRgb* scan= reinterpret_cast<QRgb*> (dst.scanLine (y));
    for (int x= 0; x < w; ++x) {
      QRgb p   = scan[x];
      int  gray= qGray (p); // 0 (black) to 255 (white)
      // black (0) -> fg, white (255) -> bg
      int r  = (fg.red () * (255 - gray) + bg.red () * gray) / 255;
      int g  = (fg.green () * (255 - gray) + bg.green () * gray) / 255;
      int b  = (fg.blue () * (255 - gray) + bg.blue () * gray) / 255;
      scan[x]= qRgba (r, g, b, qAlpha (p));
    }
  }

  QByteArray bytes;
  QBuffer    buffer (&bytes);
  buffer.open (QIODevice::WriteOnly);
  dst.save (&buffer, "PNG");
  m_previewUrl= "data:image/png;base64," + bytes.toBase64 ();
  emit previewUrlChanged ();
}

QStringList
GradientSelectorBridge::patternOptions () const {
  return {
      "vertical-white-black.png", "horizontal-white-black.png",
      "corner-gradient.jpg",      "black-corner-gradient.jpg",
      "black-gradient-frame.jpg", "grey-gradient-background.jpg",
  };
}

QStringList
GradientSelectorBridge::patternOptionsTr () const {
  return {
      qt_translate ("Vertical (vertical-white-black.png)"),
      qt_translate ("Horizontal (horizontal-white-black.png)"),
      qt_translate ("Corner (corner-gradient.jpg)"),
      qt_translate ("Black Corner (black-corner-gradient.jpg)"),
      qt_translate ("Frame (black-gradient-frame.jpg)"),
      qt_translate ("Radial Grey (grey-gradient-background.jpg)"),
  };
}

QStringList
GradientSelectorBridge::colorOptions () const {
  return {"black",  "white", "grey",    "red",    "green", "blue",
          "yellow", "cyan",  "magenta", "orange", "brown"};
}

QStringList
GradientSelectorBridge::colorOptionsTr () const {
  QStringList trs;
  for (const QString& c : colorOptions ()) {
    trs << qt_translate (from_qstring (c));
  }
  return trs;
}

QVariantMap
GradientSelectorBridge::labels () const {
  QVariantMap m;
  m["title"]     = qt_translate ("Gradient selector");
  m["pattern"]   = qt_translate ("Pattern");
  m["width"]     = qt_translate ("Width");
  m["height"]    = qt_translate ("Height");
  m["foreground"]= qt_translate ("Foreground");
  m["background"]= qt_translate ("Background");
  m["preview"]   = qt_translate ("Live preview");
  m["browse"]    = qt_translate ("Browse");
  m["pickColor"] = qt_translate ("Pick");
  m["ok"]        = qt_translate ("OK");
  m["cancel"]    = qt_translate ("Cancel");
  return m;
}

void
GradientSelectorBridge::browsePatternFile () {
  string  tm_path = as_string (get_texmacs_path ());
  QString startDir= to_qstring (tm_path) + "/misc/pictures/gradients";
  QString selected= QFileDialog::getOpenFileName (
      m_host, qt_translate ("Select gradient pattern"), startDir,
      qt_translate ("Images (*.png *.jpg *.jpeg *.bmp *.svg);;All files (*)"));
  if (!selected.isEmpty ()) {
    setPatternName (selected);
  }
}

void
GradientSelectorBridge::pickForegroundColor () {
  array<tree> proposals;
  proposals << tree (from_qstring (m_fgColor));
  tree res= cpp_color_picker_dialog (string ("Pick color"), proposals, false);
  if (N (res) > 0 && is_atomic (res[0])) {
    setForegroundColor (to_qstring (get_label (res[0])));
  }
}

void
GradientSelectorBridge::pickBackgroundColor () {
  array<tree> proposals;
  proposals << tree (from_qstring (m_bgColor));
  tree res= cpp_color_picker_dialog (string ("Pick color"), proposals, false);
  if (N (res) > 0 && is_atomic (res[0])) {
    setBackgroundColor (to_qstring (get_label (res[0])));
  }
}

void
GradientSelectorBridge::submit () {
  m_submitted= true;
  if (m_host) m_host->accept ();
}

void
GradientSelectorBridge::cancel () {
  m_submitted= false;
  if (m_host) m_host->reject ();
}
