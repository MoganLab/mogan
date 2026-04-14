/******************************************************************************
 * MODULE     : qt_tutorial.cpp
 * DESCRIPTION: Reusable spotlight tutorial infrastructure for Qt windows
 * COPYRIGHT  : (C) 2026
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qt_tutorial.hpp"

#include "boot.hpp"
#include "file.hpp"
#include "preferences.hpp"
#include "qt_gui.hpp"
#include "qt_utilities.hpp"
#include "scheme.hpp"
#include "tm_file.hpp"

#include <QApplication>
#include <QDir>
#include <QEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QMovie>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPushButton>
#include <QRegion>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace {

QRect
mapRectToWindow (QWidget* widget, QMainWindow* window) {
  if (widget == nullptr || window == nullptr) return QRect ();

  QPoint topLeft= widget->mapTo (window, QPoint (0, 0));
  if (!window->rect ().contains (topLeft)) {
    const QPoint globalTopLeft= widget->mapToGlobal (QPoint (0, 0));
    topLeft                   = window->mapFromGlobal (globalTopLeft);
  }

  return QRect (topLeft, widget->size ());
}

bool
parsePlacement (const QString& value, QWK::TutorialPlacement& placement) {
  const QString normalized= value.trimmed ().toLower ();
  if (normalized == "auto") placement= QWK::TutorialPlacement::Auto;
  else if (normalized == "top") placement= QWK::TutorialPlacement::Top;
  else if (normalized == "bottom") placement= QWK::TutorialPlacement::Bottom;
  else if (normalized == "left") placement= QWK::TutorialPlacement::Left;
  else if (normalized == "right") placement= QWK::TutorialPlacement::Right;
  else return false;

  return true;
}

bool
parseBubbleSize (const QString& value, QWK::TutorialBubbleSize& bubbleSize) {
  const QString normalized= value.trimmed ().toLower ();
  if (normalized == "small") bubbleSize= QWK::TutorialBubbleSize::Small;
  else if (normalized == "medium") bubbleSize= QWK::TutorialBubbleSize::Medium;
  else if (normalized == "large") bubbleSize= QWK::TutorialBubbleSize::Large;
  else return false;

  return true;
}

bool
parseBoolLike (const QString& value, bool& out) {
  const QString normalized= value.trimmed ().toLower ();
  if (normalized == "on" || normalized == "true" || normalized == "1" ||
      normalized == "yes") {
    out= true;
    return true;
  }
  if (normalized == "off" || normalized == "false" || normalized == "0" ||
      normalized == "no") {
    out= false;
    return true;
  }
  return false;
}

url firstLaunchTutorialConfigPath ();

QString
resolveTutorialMediaPath (const QString& rawPath) {
  const QString normalizedPath= rawPath.trimmed ();
  if (normalizedPath.isEmpty ()) return QString ();
  if (normalizedPath.startsWith (":/")) return normalizedPath;

  QFileInfo fileInfo (normalizedPath);
  if (fileInfo.isAbsolute () && fileInfo.exists ())
    return fileInfo.absoluteFilePath ();
  if (fileInfo.exists ()) return fileInfo.absoluteFilePath ();

  const QFileInfo configFileInfo (
      to_qstring (as_string (firstLaunchTutorialConfigPath ())));
  const QFileInfo relativeFileInfo (configFileInfo.dir (), normalizedPath);
  if (relativeFileInfo.exists ()) return relativeFileInfo.absoluteFilePath ();

  return normalizedPath;
}

bool
parseStepEntry (const json& stepJson, QWK::TutorialStepConfig& step,
                QString* errorMessage) {
  if (!stepJson.is_object ()) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial step entry must be an object";
    return false;
  }

  const auto readStringField= [&stepJson] (const char* key, QString& out,
                                           bool* found= nullptr) {
    if (found != nullptr) *found= false;
    const auto it= stepJson.find (key);
    if (it == stepJson.end () || it->is_null ()) return true;
    if (!it->is_string ()) return false;
    out= QString::fromStdString (it->get<std::string> ());
    if (found != nullptr) *found= true;
    return true;
  };

  bool found= false;
  if (!readStringField ("id", step.id, &found)) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial step id must be a string";
    return false;
  }
  if (!found || step.id.isEmpty ()) {
    if (errorMessage != nullptr) *errorMessage= "Tutorial step is missing id";
    return false;
  }

  if (!readStringField ("title", step.title, &found)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 title must be a string").arg (step.id);
    return false;
  }
  if (!found || step.title.isEmpty ()) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 is missing title").arg (step.id);
    return false;
  }

  if (!readStringField ("top-text", step.topText, &found)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 top-text must be a string").arg (step.id);
    return false;
  }
  const bool hasTopTextField= found && !step.topText.isEmpty ();

  if (!readStringField ("media-path", step.mediaPath)) {
    if (errorMessage != nullptr)
      *errorMessage= QString ("Tutorial step %1 media-path must be a string")
                         .arg (step.id);
    return false;
  }
  if (!readStringField ("bottom-text", step.bottomText)) {
    if (errorMessage != nullptr)
      *errorMessage= QString ("Tutorial step %1 bottom-text must be a string")
                         .arg (step.id);
    return false;
  }
  if (!readStringField ("on-enter", step.onEnterCommand)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 on-enter must be a string").arg (step.id);
    return false;
  }

  if (!hasTopTextField && step.mediaPath.isEmpty () &&
      step.bottomText.isEmpty ()) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 is missing content").arg (step.id);
    return false;
  }

  if (!readStringField ("target-id", step.targetId, &found)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 target-id must be a string").arg (step.id);
    return false;
  }

  QString placement;
  if (!readStringField ("placement", placement, &found)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 placement must be a string").arg (step.id);
    return false;
  }
  if (found && !placement.isEmpty () &&
      !parsePlacement (placement, step.placement)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 has invalid placement").arg (step.id);
    return false;
  }

  QString bubbleSize;
  if (!readStringField ("bubble-size", bubbleSize, &found)) {
    if (errorMessage != nullptr)
      *errorMessage= QString ("Tutorial step %1 bubble-size must be a string")
                         .arg (step.id);
    return false;
  }
  if (found && !bubbleSize.isEmpty () &&
      !parseBubbleSize (bubbleSize, step.bubbleSize)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 has invalid bubble-size").arg (step.id);
    return false;
  }

  const auto offsetXIt= stepJson.find ("offset-x");
  if (offsetXIt != stepJson.end () && !offsetXIt->is_null ()) {
    if (!offsetXIt->is_number_integer ()) {
      if (errorMessage != nullptr)
        *errorMessage=
            QString ("Tutorial step %1 has invalid offset-x").arg (step.id);
      return false;
    }
    step.offsetX= offsetXIt->get<int> ();
  }

  const auto offsetYIt= stepJson.find ("offset-y");
  if (offsetYIt != stepJson.end () && !offsetYIt->is_null ()) {
    if (!offsetYIt->is_number_integer ()) {
      if (errorMessage != nullptr)
        *errorMessage=
            QString ("Tutorial step %1 has invalid offset-y").arg (step.id);
      return false;
    }
    step.offsetY= offsetYIt->get<int> ();
  }

  const auto paddingIt= stepJson.find ("highlight-padding");
  if (paddingIt != stepJson.end () && !paddingIt->is_null ()) {
    if (!paddingIt->is_number_integer ()) {
      if (errorMessage != nullptr)
        *errorMessage=
            QString ("Tutorial step %1 has invalid highlight-padding")
                .arg (step.id);
      return false;
    }
    const int v= paddingIt->get<int> ();
    if (v < 0) {
      if (errorMessage != nullptr)
        *errorMessage=
            QString ("Tutorial step %1 has invalid highlight-padding")
                .arg (step.id);
      return false;
    }
    step.highlightPadding= v;
  }

  const auto skipIt= stepJson.find ("skip-if-missing");
  if (skipIt != stepJson.end () && !skipIt->is_null ()) {
    if (skipIt->is_boolean ()) step.skipIfMissing= skipIt->get<bool> ();
    else if (skipIt->is_string ()) {
      const QString skip= QString::fromStdString (skipIt->get<std::string> ());
      if (!skip.isEmpty () && !parseBoolLike (skip, step.skipIfMissing)) {
        if (errorMessage != nullptr)
          *errorMessage=
              QString ("Tutorial step %1 has invalid skip-if-missing")
                  .arg (step.id);
        return false;
      }
    }
    else {
      if (errorMessage != nullptr)
        *errorMessage= QString ("Tutorial step %1 has invalid skip-if-missing")
                           .arg (step.id);
      return false;
    }
  }

  return true;
}

url
firstLaunchTutorialConfigPath () {
  return url_system (
      "$TEXMACS_PATH/plugins/tutorial/data/first-launch-tutorial.json");
}

} // namespace

namespace QWK {

void
TutorialTargetRegistry::registerWidget (const QString& id, QWidget* widget) {
  m_widgetAnchors[id]= widget;
}

void
TutorialTargetRegistry::registerRectProvider (const QString& id,
                                              RectProvider   provider) {
  m_rectProviders[id]= std::move (provider);
}

bool
TutorialTargetRegistry::resolve (const QString& id, QMainWindow* window,
                                 QRect& rect) const {
  rect= QRect ();
  if (window == nullptr) return false;

  if (m_rectProviders.contains (id)) {
    rect= m_rectProviders.value (id) (window);
    if (rect.isValid ()) return true;
  }

  if (m_widgetAnchors.contains (id)) {
    QWidget* widget= m_widgetAnchors.value (id);
    if (widget != nullptr && !widget->isHidden () &&
        widget->size ().isValid ()) {
      rect= mapRectToWindow (widget, window);
      return rect.isValid ();
    }
  }

  QWidget* widget=
      window->findChild<QWidget*> (id, Qt::FindChildrenRecursively);
  if (widget != nullptr && !widget->isHidden () && widget->size ().isValid ()) {
    rect= mapRectToWindow (widget, window);
    return rect.isValid ();
  }

  return false;
}

bool
TutorialConfigLoader::loadFlow (url path, TutorialFlowConfig& config,
                                QString* errorMessage) {
  config= TutorialFlowConfig ();
  if (!exists (path)) {
    if (errorMessage != nullptr)
      *errorMessage= QString ("Tutorial config not found: %1")
                         .arg (to_qstring (as_string (path)));
    return false;
  }

  json root;
  try {
    const string      configTextTm= string_load (path);
    const c_string    configTextC (configTextTm);
    const std::string configText= (char*) configTextC;
    root                        = json::parse (configText);
  } catch (const std::exception& e) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial config JSON parse error: %1").arg (e.what ());
    return false;
  }

  if (!root.is_object ()) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config root must be an object";
    return false;
  }

  const auto flowIdIt= root.find ("flow-id");
  if (flowIdIt == root.end () || !flowIdIt->is_string ()) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config flow-id is invalid";
    return false;
  }
  config.flowId= QString::fromStdString (flowIdIt->get<std::string> ());
  if (config.flowId.isEmpty ()) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config flow-id is invalid";
    return false;
  }

  const auto versionIt= root.find ("version");
  if (versionIt == root.end () || !versionIt->is_number_integer ()) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config version is invalid";
    return false;
  }
  config.version= versionIt->get<int> ();
  if (config.version <= 0) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config version must be positive";
    return false;
  }

  const auto stepsIt= root.find ("steps");
  if (stepsIt == root.end () || !stepsIt->is_array ()) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config steps must be an array";
    return false;
  }

  for (const auto& stepJson : *stepsIt) {
    TutorialStepConfig step;
    if (!parseStepEntry (stepJson, step, errorMessage)) return false;
    config.steps << step;
  }

  if (config.steps.isEmpty ()) {
    if (errorMessage != nullptr) *errorMessage= "Tutorial config has no steps";
    return false;
  }

  return true;
}

TutorialBubble::TutorialBubble (QWidget* parent)
    : QWidget (parent), m_titleLabel (new QLabel (this)),
      m_topTextLabel (new QLabel (this)), m_mediaContainer (new QWidget (this)),
      m_mediaLabel (new QLabel (this)), m_bottomTextLabel (new QLabel (this)),
      m_progressLabel (new QLabel (this)),
      m_previousButton (new QPushButton (this)),
      m_nextButton (new QPushButton (this)), m_mediaMovie (nullptr),
      m_currentMediaPath () {
  setObjectName ("tutorialBubble");
  setAttribute (Qt::WA_StyledBackground, true);
  setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred);

  m_titleLabel->setObjectName ("tutorialTitle");
  m_topTextLabel->setObjectName ("tutorialBodyText");
  m_mediaLabel->setObjectName ("tutorialMedia");
  m_bottomTextLabel->setObjectName ("tutorialBodyText");
  m_progressLabel->setObjectName ("tutorialProgress");

  m_titleLabel->setWordWrap (true);
  m_topTextLabel->setWordWrap (true);
  m_bottomTextLabel->setWordWrap (true);
  m_mediaContainer->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Fixed);
  m_mediaContainer->setVisible (false);
  m_mediaContainer->setFixedSize (0, 0);
  m_mediaLabel->setAlignment (Qt::AlignCenter);
  m_mediaLabel->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);

  auto* mediaLayout= new QVBoxLayout (m_mediaContainer);
  mediaLayout->setContentsMargins (0, 0, 0, 0);
  mediaLayout->setSpacing (0);
  mediaLayout->addWidget (m_mediaLabel, 0, Qt::AlignCenter);

  m_previousButton->setText (qt_translate ("上一步"));
  m_nextButton->setText (qt_translate ("下一步"));

  auto* footerLayout= new QHBoxLayout ();
  footerLayout->setContentsMargins (0, 0, 0, 0);
  footerLayout->setSpacing (10);
  footerLayout->addWidget (m_progressLabel);
  footerLayout->addStretch ();
  footerLayout->addWidget (m_previousButton);
  footerLayout->addWidget (m_nextButton);

  auto* mainLayout= new QVBoxLayout (this);
  mainLayout->setContentsMargins (18, 18, 18, 18);
  mainLayout->setSpacing (12);
  mainLayout->setSizeConstraint (QLayout::SetFixedSize);
  mainLayout->addWidget (m_titleLabel);
  mainLayout->addWidget (m_topTextLabel);
  mainLayout->addWidget (m_mediaContainer, 0, Qt::AlignHCenter);
  mainLayout->addWidget (m_bottomTextLabel);
  mainLayout->addLayout (footerLayout);

  setLayout (mainLayout);
  setFixedWidth (360);
  setStyleSheet (QStringLiteral (R"(
    QWidget#tutorialBubble {
      background-color: #fffef8;
      border: 1px solid rgba(24, 42, 67, 0.18);
      border-radius: 14px;
    }
    QLabel#tutorialTitle {
      color: #122033;
      font-size: 17px;
      font-weight: 700;
    }
    QLabel#tutorialBodyText {
      color: #334155;
      font-size: 13px;
      line-height: 1.5;
    }
    QLabel#tutorialProgress {
      color: #6b7280;
      font-size: 12px;
      font-weight: 600;
    }
    QPushButton {
      border-radius: 8px;
      padding: 8px 14px;
      font-size: 12px;
      font-weight: 600;
      min-width: 72px;
    }
    QPushButton:hover {
      background-color: #eef4ff;
    }
  )"));

  m_previousButton->setStyleSheet (QStringLiteral (
      "QPushButton { background: #f3f4f6; color: #111827; border: 1px solid "
      "#d1d5db; }"));
  m_nextButton->setStyleSheet (QStringLiteral (
      "QPushButton { background: #0f766e; color: white; border: 1px solid "
      "#0f766e; }"));

  connect (m_previousButton, &QPushButton::clicked, this,
           &TutorialBubble::previousRequested);
  connect (m_nextButton, &QPushButton::clicked, this, [this] () {
    if (m_nextButton->text () == qt_translate ("完成")) emit finishRequested ();
    else emit nextRequested ();
  });
}

void
TutorialBubble::setStep (const TutorialStepConfig& step, int index, int total) {
  const QString mediaPath= resolveTutorialMediaPath (step.mediaPath);
  QSize         mediaSize (300, 180);

  switch (step.bubbleSize) {
  case TutorialBubbleSize::Small:
    setFixedWidth (300);
    mediaSize= QSize (240, 144);
    break;
  case TutorialBubbleSize::Medium:
    setFixedWidth (360);
    mediaSize= QSize (300, 180);
    break;
  case TutorialBubbleSize::Large:
    setFixedWidth (440);
    mediaSize= QSize (380, 228);
    break;
  }

  m_titleLabel->setText (step.title);
  m_topTextLabel->setText (step.topText);
  m_topTextLabel->setVisible (!step.topText.isEmpty ());
  m_bottomTextLabel->setText (step.bottomText);
  m_bottomTextLabel->setVisible (!step.bottomText.isEmpty ());

  if (mediaPath != m_currentMediaPath) {
    if (m_mediaMovie != nullptr) {
      m_mediaLabel->setMovie (nullptr);
      m_mediaMovie->stop ();
      delete m_mediaMovie;
      m_mediaMovie= nullptr;
    }

    m_mediaLabel->clear ();
    m_mediaLabel->setFixedSize (0, 0);
    m_mediaContainer->setVisible (false);
    m_mediaContainer->setFixedSize (0, 0);
    m_currentMediaPath= mediaPath;

    if (!mediaPath.isEmpty ()) {
      if (mediaPath.endsWith (".gif", Qt::CaseInsensitive)) {
        m_mediaMovie= new QMovie (mediaPath, QByteArray (), this);
        if (m_mediaMovie->isValid ()) {
          m_mediaLabel->setFixedSize (mediaSize);
          m_mediaContainer->setFixedSize (mediaSize);
          m_mediaContainer->setVisible (true);
          connect (m_mediaMovie, &QMovie::frameChanged, this,
                   [this, mediaSize] (int) {
                     if (m_mediaMovie == nullptr) return;
                     const QPixmap frame= m_mediaMovie->currentPixmap ();
                     if (frame.isNull ()) return;
                     m_mediaLabel->setPixmap (
                         frame.scaled (mediaSize, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
                   });
          m_mediaMovie->start ();
        }
        else {
          delete m_mediaMovie;
          m_mediaMovie= nullptr;
          m_currentMediaPath.clear ();
        }
      }
      else {
        QPixmap pixmap (mediaPath);
        if (!pixmap.isNull ()) {
          const QPixmap scaledPixmap= pixmap.scaled (
              mediaSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
          m_mediaLabel->setPixmap (scaledPixmap);
          m_mediaLabel->setFixedSize (mediaSize);
          m_mediaContainer->setFixedSize (mediaSize);
          m_mediaContainer->setVisible (true);
        }
        else {
          m_currentMediaPath.clear ();
        }
      }
    }
  }
  else if (!mediaPath.isEmpty ()) {
    m_mediaContainer->setVisible (true);
  }
  else {
    m_mediaContainer->setVisible (false);
    m_mediaContainer->setFixedSize (0, 0);
    m_mediaLabel->setFixedSize (0, 0);
  }

  m_progressLabel->setText (
      QString ("%1 / %2").arg (index + 1).arg (qMax (total, 1)));
  adjustSize ();
}

void
TutorialBubble::setFirstStep (bool first) {
  m_previousButton->setVisible (!first);
}

void
TutorialBubble::setLastStep (bool last) {
  m_nextButton->setText (last ? qt_translate ("完成")
                              : qt_translate ("下一步"));
}

TutorialOverlay::TutorialOverlay (QMainWindow* parentWindow)
    : QWidget (parentWindow), m_parentWindow (parentWindow),
      m_bubble (new TutorialBubble (this)), m_hasHighlight (false) {
  setObjectName ("tutorialOverlay");
  setAttribute (Qt::WA_StyledBackground, true);
  setFocusPolicy (Qt::NoFocus);
  setMouseTracking (true);
  setGeometry (parentWindow->rect ());

  connect (m_bubble, &TutorialBubble::previousRequested, this,
           &TutorialOverlay::previousRequested);
  connect (m_bubble, &TutorialBubble::nextRequested, this,
           &TutorialOverlay::nextRequested);
  connect (m_bubble, &TutorialBubble::finishRequested, this,
           &TutorialOverlay::finishRequested);
}

void
TutorialOverlay::setStep (const TutorialStepConfig& step, int index,
                          int total) {
  m_currentStep= step;
  m_bubble->setStep (step, index, total);
  m_bubble->setFirstStep (index == 0);
  m_bubble->setLastStep (index == total - 1);
}

void
TutorialOverlay::setHighlightedRect (const QRect& rect, int padding) {
  const QRect previousHighlightRect= m_highlightRect;
  m_highlightRect= rect.adjusted (-padding, -padding, padding, padding)
                       .intersected (this->rect ().adjusted (8, 8, -8, -8));
  m_hasHighlight= true;
  repositionBubble (m_currentStep.placement);
  refreshExposedArea (previousHighlightRect.united (m_highlightRect));
  update ();
}

void
TutorialOverlay::clearHighlight () {
  const QRect previousHighlightRect= m_highlightRect;
  m_highlightRect                  = QRect ();
  m_hasHighlight                   = false;
  clearMask ();
  repositionBubble (TutorialPlacement::Auto);
  refreshExposedArea (previousHighlightRect);
  update ();
}

void
TutorialOverlay::updateInputMask () {
  if (!m_hasHighlight || !m_highlightRect.isValid ()) {
    clearMask ();
    return;
  }

  QRegion overlayRegion (rect ());
  overlayRegion= overlayRegion.subtracted (QRegion (m_highlightRect));
  overlayRegion= overlayRegion.united (QRegion (m_bubble->geometry ()));
  setMask (overlayRegion);
}

void
TutorialOverlay::refreshExposedArea (const QRect& rect) {
  if (m_parentWindow == nullptr || !rect.isValid ()) return;

  const QRect clippedRect= rect.intersected (m_parentWindow->rect ());
  if (!clippedRect.isValid ()) return;

  const QList<QWidget*> widgets= m_parentWindow->findChildren<QWidget*> (
      QString (), Qt::FindChildrenRecursively);
  for (QWidget* widget : widgets) {
    if (widget == nullptr || widget == this || widget == m_bubble ||
        widget->isHidden () || !widget->size ().isValid ())
      continue;

    const QRect widgetRect= mapRectToWindow (widget, m_parentWindow);
    if (!widgetRect.isValid () || !widgetRect.intersects (clippedRect))
      continue;

    widget->repaint ();
  }

  m_parentWindow->repaint (clippedRect);
}

QRect
TutorialOverlay::bubbleRectForPlacement (TutorialPlacement placement) const {
  const int spacing= 18;
  const int margin = 20;
  QSize     size   = m_bubble->sizeHint ();
  QRect     safe   = rect ().adjusted (margin, margin, -margin, -margin);

  auto clampRect= [&safe, &size] (QRect candidate) {
    int x= qBound (safe.left (), candidate.x (), safe.right () - size.width ());
    int y=
        qBound (safe.top (), candidate.y (), safe.bottom () - size.height ());
    return QRect (QPoint (x, y), size);
  };

  if (!m_hasHighlight) {
    QPoint center=
        safe.center () - QPoint (size.width () / 2, size.height () / 2);
    return clampRect (
        QRect (center, size)
            .translated (m_currentStep.offsetX, m_currentStep.offsetY));
  }

  auto candidateFor= [this, &size, spacing] (TutorialPlacement p) {
    switch (p) {
    case TutorialPlacement::Top:
      return QRect (QPoint (m_highlightRect.center ().x () - size.width () / 2,
                            m_highlightRect.top () - size.height () - spacing),
                    size);
    case TutorialPlacement::Bottom:
      return QRect (QPoint (m_highlightRect.center ().x () - size.width () / 2,
                            m_highlightRect.bottom () + spacing),
                    size);
    case TutorialPlacement::Left:
      return QRect (
          QPoint (m_highlightRect.left () - size.width () - spacing,
                  m_highlightRect.center ().y () - size.height () / 2),
          size);
    case TutorialPlacement::Right:
      return QRect (
          QPoint (m_highlightRect.right () + spacing,
                  m_highlightRect.center ().y () - size.height () / 2),
          size);
    case TutorialPlacement::Auto:
      break;
    }
    return QRect ();
  };

  QList<TutorialPlacement> placements;
  if (placement == TutorialPlacement::Auto) {
    placements << TutorialPlacement::Bottom << TutorialPlacement::Top
               << TutorialPlacement::Right << TutorialPlacement::Left;
  }
  else {
    placements << placement << TutorialPlacement::Bottom
               << TutorialPlacement::Top << TutorialPlacement::Right
               << TutorialPlacement::Left;
  }

  for (TutorialPlacement p : placements) {
    QRect candidate= candidateFor (p).translated (m_currentStep.offsetX,
                                                  m_currentStep.offsetY);
    if (safe.contains (candidate)) return candidate;
  }

  return clampRect (
      candidateFor (placements.front ())
          .translated (m_currentStep.offsetX, m_currentStep.offsetY));
}

void
TutorialOverlay::repositionBubble (TutorialPlacement placement) {
  m_bubble->adjustSize ();
  m_bubble->setGeometry (bubbleRectForPlacement (placement));
  updateInputMask ();
  m_bubble->raise ();
}

void
TutorialOverlay::paintEvent (QPaintEvent* event) {
  (void) event;

  QPainter painter (this);
  painter.setRenderHint (QPainter::Antialiasing, true);

  QPainterPath overlayPath;
  overlayPath.addRect (rect ());

  if (m_hasHighlight) {
    QPainterPath hole;
    hole.addRoundedRect (m_highlightRect, 14, 14);
    overlayPath= overlayPath.subtracted (hole);
  }

  painter.fillPath (overlayPath, QColor (10, 18, 28, 180));
}

void
TutorialOverlay::mousePressEvent (QMouseEvent* event) {
  event->accept ();
}

void
TutorialOverlay::mouseReleaseEvent (QMouseEvent* event) {
  event->accept ();
}

void
TutorialOverlay::mouseMoveEvent (QMouseEvent* event) {
  event->accept ();
}

void
TutorialOverlay::wheelEvent (QWheelEvent* event) {
  event->accept ();
}

TutorialEngine::TutorialEngine (QObject* parent)
    : QObject (parent), m_currentIndex (-1), m_displayedIndex (-1),
      m_stepRequestId (0) {}

bool
TutorialEngine::start (QMainWindow*                  hostWindow,
                       const TutorialFlowConfig&     config,
                       const TutorialTargetRegistry& registry) {
  if (hostWindow == nullptr || !config.isValid ()) return false;

  if (isActive ()) stop (TutorialFinishReason::Cancelled);

  m_hostWindow    = hostWindow;
  m_registry      = registry;
  m_config        = config;
  m_currentIndex  = -1;
  m_displayedIndex= -1;
  m_stepRequestId = 0;
  m_overlay       = new TutorialOverlay (hostWindow);
  m_overlay->show ();
  m_overlay->raise ();

  connect (m_overlay, &TutorialOverlay::previousRequested, this,
           &TutorialEngine::previous);
  connect (m_overlay, &TutorialOverlay::nextRequested, this,
           &TutorialEngine::next);
  connect (m_overlay, &TutorialOverlay::finishRequested, this,
           [this] () { stop (TutorialFinishReason::Completed); });

  m_hostWindow->installEventFilter (this);
  updateOverlayGeometry ();
  showNextAvailableStep (0, 1);
  return true;
}

void
TutorialEngine::stop (TutorialFinishReason reason) {
  if (m_hostWindow != nullptr) m_hostWindow->removeEventFilter (this);

  if (m_overlay != nullptr) {
    m_overlay->hide ();
    m_overlay->deleteLater ();
  }

  m_overlay       = nullptr;
  m_hostWindow    = nullptr;
  m_currentIndex  = -1;
  m_displayedIndex= -1;
  m_stepRequestId = 0;
  m_config        = TutorialFlowConfig ();
  m_registry      = TutorialTargetRegistry ();

  emit finished (reason);
}

void
TutorialEngine::next () {
  if (!isActive ()) return;
  showNextAvailableStep (m_currentIndex + 1, 1);
}

void
TutorialEngine::previous () {
  if (!isActive ()) return;
  showNextAvailableStep (m_currentIndex - 1, -1);
}

bool
TutorialEngine::isActive () const {
  return m_overlay != nullptr && m_hostWindow != nullptr;
}

bool
TutorialEngine::isActiveForWindow (QMainWindow* mainWindow) const {
  return isActive () && m_hostWindow == mainWindow;
}

bool
TutorialEngine::eventFilter (QObject* watched, QEvent* event) {
  if (watched == m_hostWindow) {
    switch (event->type ()) {
    case QEvent::Resize:
    case QEvent::Move:
    case QEvent::LayoutRequest:
    case QEvent::WindowStateChange:
      updateOverlayGeometry ();
      refreshCurrentStepGeometry ();
      break;
    case QEvent::Close:
      stop (TutorialFinishReason::HostClosed);
      break;
    default:
      break;
    }
  }

  return QObject::eventFilter (watched, event);
}

void
TutorialEngine::executeOnEnter (const TutorialStepConfig& step) {
  if (step.onEnterCommand.trimmed ().isEmpty ()) return;
  exec_delayed (scheme_cmd (from_qstring (step.onEnterCommand)));
}

void
TutorialEngine::updateOverlayGeometry () {
  if (m_overlay == nullptr || m_hostWindow == nullptr) return;
  m_overlay->setGeometry (m_hostWindow->rect ());
  m_overlay->raise ();
}

void
TutorialEngine::refreshCurrentStepGeometry () {
  if (!isActive ()) return;
  if (m_currentIndex < 0 || m_currentIndex >= m_config.steps.size ()) return;

  const TutorialStepConfig& step= m_config.steps[m_currentIndex];
  if (step.targetId.trimmed ().isEmpty ()) {
    m_overlay->clearHighlight ();
    m_overlay->show ();
    m_overlay->raise ();
    return;
  }

  QRect rect;
  if (!m_registry.resolve (step.targetId, m_hostWindow, rect)) {
    m_overlay->clearHighlight ();
    m_overlay->show ();
    m_overlay->raise ();
    return;
  }

  m_overlay->setHighlightedRect (rect, step.highlightPadding);
  m_overlay->show ();
  m_overlay->raise ();
}

void
TutorialEngine::showStep (int index, int retryCount, int fallbackDirection,
                          int requestId) {
  if (!isActive ()) return;
  if (index < 0 || index >= m_config.steps.size ()) return;

  if (requestId < 0) requestId= ++m_stepRequestId;
  if (requestId != m_stepRequestId) return;

  m_currentIndex= index;

  QRect                     rect;
  const TutorialStepConfig& step= m_config.steps[index];
  if (step.targetId.trimmed ().isEmpty ()) {
    m_overlay->setStep (step, index, m_config.steps.size ());
    m_overlay->clearHighlight ();
    m_overlay->show ();
    m_overlay->raise ();
    if (m_displayedIndex != index) {
      executeOnEnter (step);
      m_displayedIndex= index;
    }
    emit stepChanged (step.id, index, m_config.steps.size ());
    return;
  }

  if (!m_registry.resolve (step.targetId, m_hostWindow, rect)) {
    if (retryCount < kMaxResolveRetries) {
      QTimer::singleShot (
          150, m_overlay,
          [this, index, retryCount, fallbackDirection, requestId] () {
            showStep (index, retryCount + 1, fallbackDirection, requestId);
          });
      return;
    }

    if (step.skipIfMissing && fallbackDirection != 0) {
      showNextAvailableStep (index + fallbackDirection, fallbackDirection);
      return;
    }

    m_overlay->setStep (step, index, m_config.steps.size ());
    m_overlay->clearHighlight ();
    m_overlay->show ();
    m_overlay->raise ();
    if (m_displayedIndex != index) {
      executeOnEnter (step);
      m_displayedIndex= index;
    }
    emit stepChanged (step.id, index, m_config.steps.size ());
    return;
  }

  m_overlay->setStep (step, index, m_config.steps.size ());
  m_overlay->setHighlightedRect (rect, step.highlightPadding);
  m_overlay->show ();
  m_overlay->raise ();
  if (m_displayedIndex != index) {
    executeOnEnter (step);
    m_displayedIndex= index;
  }
  emit stepChanged (step.id, index, m_config.steps.size ());
}

void
TutorialEngine::showNextAvailableStep (int startIndex, int direction) {
  if (m_config.steps.isEmpty ()) {
    stop (TutorialFinishReason::Completed);
    return;
  }

  for (int i= startIndex; i >= 0 && i < m_config.steps.size (); i+= direction) {
    showStep (i, 0, direction);
    return;
  }

  if (direction > 0) stop (TutorialFinishReason::Completed);
}

FirstLaunchTutorialController*
FirstLaunchTutorialController::instance () {
  static FirstLaunchTutorialController* controller=
      new FirstLaunchTutorialController (qApp);
  return controller;
}

FirstLaunchTutorialController::FirstLaunchTutorialController (QObject* parent)
    : QObject (parent), m_engine (new TutorialEngine (this)),
      m_startedThisSession (false) {
  connect (m_engine, &TutorialEngine::finished, this,
           [this] (TutorialFinishReason reason) {
             if (reason != TutorialFinishReason::Completed &&
                 reason != TutorialFinishReason::Skipped) {
               return;
             }

             TutorialFlowConfig flow= loadFirstLaunchFlow ();
             if (!flow.isValid ()) flow= buildFallbackFlow ();
             if (!flow.isValid ()) return;

             const QString prefix= (reason == TutorialFinishReason::Completed)
                                       ? "tutorial:completed-version"
                                       : "tutorial:skipped-version";
             set_user_preference (
                 from_qstring (preferenceKey (prefix, flow.flowId)),
                 from_qstring (versionString (flow.version)));
             save_user_preferences ();
           });
}

TutorialFlowConfig
FirstLaunchTutorialController::loadFirstLaunchFlow () const {
  TutorialFlowConfig flow;
  QString            errorMessage;
  if (TutorialConfigLoader::loadFlow (firstLaunchTutorialConfigPath (), flow,
                                      &errorMessage)) {
    return flow;
  }

  std_warning << "Unable to load tutorial config: "
              << from_qstring (errorMessage) << LF;
  return TutorialFlowConfig ();
}

TutorialFlowConfig
FirstLaunchTutorialController::buildFallbackFlow () const {
  TutorialFlowConfig flow;
  flow.flowId = "first-launch";
  flow.version= 1;
  flow.steps  = {
      {"welcome", qt_translate ("认识一下主窗口"), "mainWindowSafeArea",
         TutorialPlacement::Bottom, 12, true,
         qt_translate ("这是 Liii STEM "
                         "的主工作区。教程会依次指出最常用的几个区域，帮助你快速建"
                         "立基本认知。"),
         QString (), QString ()},
      {"windowbar", qt_translate ("这里是窗口顶部"), "windowbar",
         TutorialPlacement::Bottom, 10, true,
         qt_translate ("这里包含窗口切换、标签页和常用入口。你以后会频繁从这里切"
                         "换文档和访问全局功能。"),
         QString (), QString ()},
      {"toolbar", qt_translate ("这里是主工具栏"), "toolbarArea",
         TutorialPlacement::Bottom, 10, true,
         qt_translate ("常见的格式、插入和排版操作会集中在这一带。不同编辑场景下"
                         "，这里的按钮也会变化。"),
         QString (), QString ()},
      {"editor", qt_translate ("这里是编辑区"), "editorArea",
         TutorialPlacement::Top, 12, true,
         qt_translate ("文档内容主要在这里输入、排版和修改。无论是公式、文本还是"
                         "结构化内容，核心操作都围绕这个区域展开。"),
         QString (), QString ()},
      {"assistant", qt_translate ("这里是扩展能力入口"), "assistantEntry",
         TutorialPlacement::Left, 10, true,
         qt_translate ("这一侧用于放置辅助能力或扩展面板；如果当前面板未显示，教"
                         "程会退化到登录与能力入口，帮助你找到后续探索的位置。"),
         QString (), QString ()},
  };
  return flow;
}

TutorialTargetRegistry
FirstLaunchTutorialController::buildRegistry (QMainWindow* mainWindow) const {
  TutorialTargetRegistry registry;

  registry.registerRectProvider (
      "mainWindowSafeArea", [] (QMainWindow* hostWindow) {
        return hostWindow->rect ().adjusted (24, 24, -24, -24);
      });

  registry.registerRectProvider ("toolbarArea", [] (QMainWindow* hostWindow) {
    const QStringList toolbarIds= {"mainToolBar", "modeToolBar", "focusToolBar",
                                   "menuToolBar"};

    QRect rect;
    for (const QString& id : toolbarIds) {
      QWidget* widget=
          hostWindow->findChild<QWidget*> (id, Qt::FindChildrenRecursively);
      if (widget == nullptr || widget->isHidden () ||
          !widget->size ().isValid ())
        continue;
      return mapRectToWindow (widget, hostWindow);
    }

    QWidget* windowbar= hostWindow->findChild<QWidget*> (
        "windowbar", Qt::FindChildrenRecursively);
    QWidget* editor= hostWindow->findChild<QWidget*> (
        "editorCanvas", Qt::FindChildrenRecursively);
    if (windowbar != nullptr && editor != nullptr && !windowbar->isHidden () &&
        windowbar->size ().isValid () && !editor->isHidden () &&
        editor->size ().isValid ()) {
      QRect     windowbarRect= mapRectToWindow (windowbar, hostWindow);
      QRect     editorRect   = mapRectToWindow (editor, hostWindow);
      const int top          = windowbarRect.bottom () + 8;
      const int bottom       = qMin (editorRect.top () - 8, top + 72);
      if (bottom > top) {
        return QRect (QPoint (32, top),
                      QPoint (hostWindow->rect ().right () - 32, bottom));
      }
    }

    return QRect ();
  });

  registry.registerRectProvider ("editorArea", [] (QMainWindow* hostWindow) {
    QWidget* editor= hostWindow->findChild<QWidget*> (
        "editorCanvas", Qt::FindChildrenRecursively);
    if (editor != nullptr && !editor->isHidden () &&
        editor->size ().isValid ()) {
      return mapRectToWindow (editor, hostWindow);
    }

    QWidget* centralWidget= hostWindow->centralWidget ();
    if (centralWidget == nullptr) return QRect ();

    QRect centralRect= mapRectToWindow (centralWidget, hostWindow);
    if (!centralRect.isValid ()) return QRect ();

    QWidget* guestBar= hostWindow->findChild<QWidget*> (
        "guestNotificationBar", Qt::FindChildrenRecursively);
    if (guestBar != nullptr && !guestBar->isHidden () &&
        guestBar->size ().isValid ()) {
      QRect guestRect= mapRectToWindow (guestBar, hostWindow);
      centralRect.setTop (
          qMin (centralRect.bottom (), guestRect.bottom () + 8));
    }

    return centralRect.adjusted (8, 8, -8, -8);
  });

  registry.registerRectProvider (
      "assistantEntry", [] (QMainWindow* hostWindow) {
        const QStringList ids= {"sideTools", "auxiliaryWidget",
                                "guestNotificationBar", "login-button",
                                "statusBar"};
        for (const QString& id : ids) {
          QWidget* widget= (id == "statusBar")
                               ? hostWindow->statusBar ()
                               : hostWindow->findChild<QWidget*> (
                                     id, Qt::FindChildrenRecursively);
          if (widget == nullptr || widget->isHidden () ||
              !widget->size ().isValid ())
            continue;
          return mapRectToWindow (widget, hostWindow);
        }
        return QRect ();
      });

  const QStringList widgetIds= {
      "windowbar",    "mainToolBar",    "modeToolBar", "focusToolBar",
      "menuToolBar",  "editorCanvas",   "sideTools",   "guestNotificationBar",
      "login-button", "auxiliaryWidget"};
  for (const QString& id : widgetIds) {
    registry.registerWidget (
        id, mainWindow->findChild<QWidget*> (id, Qt::FindChildrenRecursively));
  }
  registry.registerWidget ("statusBar", mainWindow->statusBar ());

  return registry;
}

bool
FirstLaunchTutorialController::shouldStart (
    const TutorialFlowConfig& flow) const {
  if (install_status != 1) return false;
  if (m_startedThisSession) return false;
  if (!flow.isValid ()) return false;

  const QString completedKey=
      preferenceKey ("tutorial:completed-version", flow.flowId);
  const QString skippedKey=
      preferenceKey ("tutorial:skipped-version", flow.flowId);
  const QString currentVersion= versionString (flow.version);

  if (get_preference (from_qstring (completedKey), "0") ==
      from_qstring (currentVersion))
    return false;
  if (get_preference (from_qstring (skippedKey), "0") ==
      from_qstring (currentVersion))
    return false;
  return true;
}

QString
FirstLaunchTutorialController::versionString (int version) const {
  return QString::number (version);
}

QString
FirstLaunchTutorialController::preferenceKey (const QString& prefix,
                                              const QString& flowId) const {
  return prefix + ":" + flowId;
}

void
FirstLaunchTutorialController::maybeStartForMainWindow (
    QMainWindow* mainWindow) {
  if (mainWindow == nullptr) return;

  TutorialFlowConfig flow= loadFirstLaunchFlow ();
  if (!flow.isValid ()) flow= buildFallbackFlow ();
  if (!shouldStart (flow)) return;
  if (m_engine->isActiveForWindow (mainWindow)) return;
  if (mainWindow->property ("tutorialScheduled").toBool ()) return;

  mainWindow->setProperty ("tutorialScheduled", true);
  QPointer<QMainWindow> target= mainWindow;
  QTimer::singleShot (0, mainWindow, [this, target, flow] () {
    if (target == nullptr) return;
    target->setProperty ("tutorialScheduled", false);
    if (!shouldStart (flow)) return;

    m_startedThisSession= true;
    m_engine->start (target, flow, buildRegistry (target));
  });
}

} // namespace QWK
