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
#include "brush.hpp"
#include "qt_utilities.hpp"
#include "tm_sys_utils.hpp"
#include "url.hpp"

#include <QBuffer>
#include <QColor>
#include <QFileDialog>
#include <QImage>

static QString
resolvePatternPath (const QString& name) {
  if (name.isEmpty ()) return "";
  url u= resolve_pattern (url_system (from_qstring (name)));
  if (is_none (u)) return name;
  return to_qstring (as_string (u));
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
      m_bgColor (bg.isEmpty () ? "white" : bg) {
  reloadSource ();
  updatePreview ();
}

void
GradientSelectorBridge::setPatternName (const QString& name) {
  if (m_patternName != name) {
    m_patternName= name;
    emit patternNameChanged ();
    reloadSource ();
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

// 预览窗格约 330x280 逻辑像素，源图缩到 640 足以覆盖 2x HiDPI，
// 避免用户浏览进来的大图让每次换色都做全分辨率重着色与 PNG 编码。
static const int kPreviewMax= 640;

void
GradientSelectorBridge::reloadSource () {
  QImage img (resolvePatternPath (m_patternName));
  if (img.isNull ()) {
    img= QImage (200, 200, QImage::Format_Grayscale8);
    for (int y= 0; y < 200; ++y) {
      uchar* scan= img.scanLine (y);
      uchar  v   = static_cast<uchar> (255 - (y * 255 / 199));
      memset (scan, v, 200);
    }
  }
  else if (img.width () > kPreviewMax || img.height () > kPreviewMax) {
    img= img.scaled (kPreviewMax, kPreviewMax, Qt::KeepAspectRatio,
                     Qt::SmoothTransformation);
  }
  m_source= img.convertToFormat (QImage::Format_ARGB32);
}

void
GradientSelectorBridge::updatePreview () {
  QColor fg= parseColor (m_fgColor);
  QColor bg= parseColor (m_bgColor);
  if (!fg.isValid ()) fg= QColor (0, 0, 0);
  if (!bg.isValid ()) bg= QColor (255, 255, 255);

  // 灰度只有 256 级，插值结果查表即可：black (0) -> fg, white (255) -> bg
  QRgb lut[256];
  for (int i= 0; i < 256; ++i) {
    int r = (fg.red () * (255 - i) + bg.red () * i) / 255;
    int g = (fg.green () * (255 - i) + bg.green () * i) / 255;
    int b = (fg.blue () * (255 - i) + bg.blue () * i) / 255;
    lut[i]= qRgb (r, g, b);
  }

  QImage dst= m_source; // 隐式共享，写入 scanLine 时才 detach
  int    w  = dst.width ();
  int    h  = dst.height ();
  for (int y= 0; y < h; ++y) {
    QRgb* scan= reinterpret_cast<QRgb*> (dst.scanLine (y));
    for (int x= 0; x < w; ++x) {
      QRgb p = scan[x];
      scan[x]= (lut[qGray (p)] & 0x00FFFFFF) | (p & 0xFF000000);
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
GradientSelectorBridge::pickColor (
    const QString& cur,
    void (GradientSelectorBridge::*setter) (const QString&)) {
  array<tree> proposals;
  proposals << tree (from_qstring (cur));
  tree res= cpp_color_picker_dialog (string ("Pick color"), proposals, false);
  if (N (res) > 0 && is_atomic (res[0])) {
    (this->*setter) (to_qstring (get_label (res[0])));
  }
}

void
GradientSelectorBridge::pickForegroundColor () {
  pickColor (m_fgColor, &GradientSelectorBridge::setForegroundColor);
}

void
GradientSelectorBridge::pickBackgroundColor () {
  pickColor (m_bgColor, &GradientSelectorBridge::setBackgroundColor);
}

void
GradientSelectorBridge::submit () {
  if (m_host) m_host->accept ();
}

void
GradientSelectorBridge::cancel () {
  if (m_host) m_host->reject ();
}
