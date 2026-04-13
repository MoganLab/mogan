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
#include "qt_utilities.hpp"
#include "tm_file.hpp"
#include "tree_helper.hpp"

#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStatusBar>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <moebius/data/scheme.hpp>

using moebius::data::block_to_scheme_tree;
using moebius::data::scm_unquote;

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

tree
normalizeTuple (tree t) {
  while (is_func (t, TUPLE, 1))
    t= t[0];
  return t;
}

bool
treeToQString (tree t, QString& out) {
  if (!is_atomic (t)) return false;

  string label= t->label;
  if (is_quoted (label)) label= scm_unquote (label);
  out= to_qstring (label);
  return true;
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

QString
readField (tree entries, const QString& key, bool* found= nullptr) {
  if (found != nullptr) *found= false;

  for (int i= 0; i < N (entries); ++i) {
    if (!is_func (entries[i], TUPLE, 2)) continue;

    QString currentKey;
    if (!treeToQString (entries[i][0], currentKey)) continue;
    if (currentKey != key) continue;

    QString value;
    if (!treeToQString (entries[i][1], value)) return QString ();
    if (found != nullptr) *found= true;
    return value;
  }

  return QString ();
}

bool
parseStepEntry (tree entry, QWK::TutorialStepConfig& step,
                QString* errorMessage) {
  if (!is_func (entry, TUPLE, 2)) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial step entry must be a tuple";
    return false;
  }

  QString entryKey;
  if (!treeToQString (entry[0], entryKey) || entryKey != "step") {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config contains invalid step key";
    return false;
  }

  tree stepTree= normalizeTuple (entry[1]);
  if (!is_tuple (stepTree)) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial step payload must be a tuple";
    return false;
  }

  bool found= false;
  step.id   = readField (stepTree, "id", &found);
  if (!found || step.id.isEmpty ()) {
    if (errorMessage != nullptr) *errorMessage= "Tutorial step is missing id";
    return false;
  }

  step.title= readField (stepTree, "title", &found);
  if (!found || step.title.isEmpty ()) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 is missing title").arg (step.id);
    return false;
  }

  step.description= readField (stepTree, "description", &found);
  if (!found || step.description.isEmpty ()) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 is missing description").arg (step.id);
    return false;
  }

  step.targetId= readField (stepTree, "target-id", &found);
  if (!found || step.targetId.isEmpty ()) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 is missing target-id").arg (step.id);
    return false;
  }

  const QString placement= readField (stepTree, "placement", &found);
  if (found && !placement.isEmpty () &&
      !parsePlacement (placement, step.placement)) {
    if (errorMessage != nullptr)
      *errorMessage=
          QString ("Tutorial step %1 has invalid placement").arg (step.id);
    return false;
  }

  const QString padding= readField (stepTree, "highlight-padding", &found);
  if (found && !padding.isEmpty ()) {
    bool ok= false;
    int  v = padding.toInt (&ok);
    if (!ok || v < 0) {
      if (errorMessage != nullptr)
        *errorMessage=
            QString ("Tutorial step %1 has invalid highlight-padding")
                .arg (step.id);
      return false;
    }
    step.highlightPadding= v;
  }

  const QString skip= readField (stepTree, "skip-if-missing", &found);
  if (found && !skip.isEmpty () && !parseBoolLike (skip, step.skipIfMissing)) {
    if (errorMessage != nullptr)
      *errorMessage= QString ("Tutorial step %1 has invalid skip-if-missing")
                         .arg (step.id);
    return false;
  }

  return true;
}

url
firstLaunchTutorialConfigPath () {
  return url_system (
      "$TEXMACS_PATH/progs/startup-tab/first-launch-tutorial.scm");
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

  tree root= normalizeTuple (block_to_scheme_tree (string_load (path)));
  if (!is_tuple (root)) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config root must be a tuple";
    return false;
  }

  bool flowIdFound = false;
  bool versionFound= false;
  for (int i= 0; i < N (root); ++i) {
    if (!is_func (root[i], TUPLE, 2)) continue;

    QString key;
    if (!treeToQString (root[i][0], key)) continue;

    if (key == "flow-id") {
      if (!treeToQString (root[i][1], config.flowId) ||
          config.flowId.isEmpty ()) {
        if (errorMessage != nullptr)
          *errorMessage= "Tutorial config flow-id is invalid";
        return false;
      }
      flowIdFound= true;
      continue;
    }

    if (key == "version") {
      QString versionValue;
      if (!treeToQString (root[i][1], versionValue)) {
        if (errorMessage != nullptr)
          *errorMessage= "Tutorial config version is invalid";
        return false;
      }
      bool ok= false;
      int  v = versionValue.toInt (&ok);
      if (!ok || v <= 0) {
        if (errorMessage != nullptr)
          *errorMessage= "Tutorial config version must be positive";
        return false;
      }
      config.version= v;
      versionFound  = true;
      continue;
    }

    if (key == "step") {
      TutorialStepConfig step;
      if (!parseStepEntry (root[i], step, errorMessage)) return false;
      config.steps << step;
    }
  }

  if (!flowIdFound) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config is missing flow-id";
    return false;
  }
  if (!versionFound) {
    if (errorMessage != nullptr)
      *errorMessage= "Tutorial config is missing version";
    return false;
  }
  if (config.steps.isEmpty ()) {
    if (errorMessage != nullptr) *errorMessage= "Tutorial config has no steps";
    return false;
  }

  return true;
}

TutorialBubble::TutorialBubble (QWidget* parent)
    : QWidget (parent), m_titleLabel (new QLabel (this)),
      m_descriptionLabel (new QLabel (this)),
      m_progressLabel (new QLabel (this)),
      m_previousButton (new QPushButton (this)),
      m_nextButton (new QPushButton (this)) {
  setObjectName ("tutorialBubble");
  setAttribute (Qt::WA_StyledBackground, true);
  setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);

  m_titleLabel->setObjectName ("tutorialTitle");
  m_descriptionLabel->setObjectName ("tutorialDescription");
  m_progressLabel->setObjectName ("tutorialProgress");

  m_titleLabel->setWordWrap (true);
  m_descriptionLabel->setWordWrap (true);

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
  mainLayout->addWidget (m_titleLabel);
  mainLayout->addWidget (m_descriptionLabel);
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
    QLabel#tutorialDescription {
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
  m_titleLabel->setText (step.title);
  m_descriptionLabel->setText (step.description);
  m_progressLabel->setText (
      QString ("%1 / %2").arg (index + 1).arg (qMax (total, 1)));
  adjustSize ();
}

void
TutorialBubble::setFirstStep (bool first) {
  m_previousButton->setEnabled (!first);
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
  setFocusPolicy (Qt::StrongFocus);
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
  m_highlightRect= rect.adjusted (-padding, -padding, padding, padding)
                       .intersected (this->rect ().adjusted (8, 8, -8, -8));
  m_hasHighlight= true;
  repositionBubble (m_currentStep.placement);
  update ();
}

void
TutorialOverlay::clearHighlight () {
  m_highlightRect= QRect ();
  m_hasHighlight = false;
  repositionBubble (TutorialPlacement::Auto);
  update ();
}

QRect
TutorialOverlay::bubbleRectForPlacement (TutorialPlacement placement) const {
  const int spacing= 18;
  const int margin = 20;
  QSize     size   = m_bubble->sizeHint ();
  QRect     safe   = rect ().adjusted (margin, margin, -margin, -margin);

  if (!m_hasHighlight) {
    QPoint center=
        safe.center () - QPoint (size.width () / 2, size.height () / 2);
    return QRect (center, size);
  }

  auto clampRect= [&safe, &size] (QRect candidate) {
    int x= qBound (safe.left (), candidate.x (), safe.right () - size.width ());
    int y=
        qBound (safe.top (), candidate.y (), safe.bottom () - size.height ());
    return QRect (QPoint (x, y), size);
  };

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
    QRect candidate= candidateFor (p);
    if (safe.contains (candidate)) return candidate;
  }

  return clampRect (candidateFor (placements.front ()));
}

void
TutorialOverlay::repositionBubble (TutorialPlacement placement) {
  m_bubble->adjustSize ();
  m_bubble->setGeometry (bubbleRectForPlacement (placement));
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

  if (m_hasHighlight) {
    QPen borderPen (QColor (255, 244, 214), 2);
    painter.setPen (borderPen);
    painter.setBrush (Qt::NoBrush);
    painter.drawRoundedRect (m_highlightRect, 14, 14);

    QPen glowPen (QColor (255, 199, 94, 120), 4);
    painter.setPen (glowPen);
    painter.drawRoundedRect (m_highlightRect.adjusted (1, 1, -1, -1), 14, 14);
  }
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

void
TutorialOverlay::keyPressEvent (QKeyEvent* event) {
  QWidget::keyPressEvent (event);
  event->accept ();
}

TutorialEngine::TutorialEngine (QObject* parent)
    : QObject (parent), m_currentIndex (-1) {}

bool
TutorialEngine::start (QMainWindow*                  hostWindow,
                       const TutorialFlowConfig&     config,
                       const TutorialTargetRegistry& registry) {
  if (hostWindow == nullptr || !config.isValid ()) return false;

  if (isActive ()) stop (TutorialFinishReason::Cancelled);

  m_hostWindow  = hostWindow;
  m_registry    = registry;
  m_config      = config;
  m_currentIndex= -1;
  m_overlay     = new TutorialOverlay (hostWindow);
  m_overlay->show ();
  m_overlay->raise ();
  m_overlay->activateWindow ();
  m_overlay->setFocus ();

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

  m_overlay     = nullptr;
  m_hostWindow  = nullptr;
  m_currentIndex= -1;
  m_config      = TutorialFlowConfig ();
  m_registry    = TutorialTargetRegistry ();

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
      if (m_currentIndex >= 0) showStep (m_currentIndex, 0, 0);
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
TutorialEngine::updateOverlayGeometry () {
  if (m_overlay == nullptr || m_hostWindow == nullptr) return;
  m_overlay->setGeometry (m_hostWindow->rect ());
  m_overlay->raise ();
}

void
TutorialEngine::showStep (int index, int retryCount, int fallbackDirection) {
  if (!isActive ()) return;
  if (index < 0 || index >= m_config.steps.size ()) return;

  m_currentIndex= index;

  QRect                     rect;
  const TutorialStepConfig& step= m_config.steps[index];
  if (!m_registry.resolve (step.targetId, m_hostWindow, rect)) {
    if (retryCount < kMaxResolveRetries) {
      QTimer::singleShot (150, m_overlay,
                          [this, index, retryCount, fallbackDirection] () {
                            showStep (index, retryCount + 1, fallbackDirection);
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
    m_overlay->setFocus ();
    emit stepChanged (step.id, index, m_config.steps.size ());
    return;
  }

  m_overlay->setStep (step, index, m_config.steps.size ());
  m_overlay->setHighlightedRect (rect, step.highlightPadding);
  m_overlay->show ();
  m_overlay->raise ();
  m_overlay->setFocus ();
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
      {"welcome", qt_translate ("认识一下主窗口"),
         qt_translate ("这是 Liii STEM "
                         "的主工作区。教程会依次指出最常用的几个区域，帮助你快速建"
                         "立基本认知。"),
         "mainWindowSafeArea", TutorialPlacement::Bottom, 12, true},
      {"windowbar", qt_translate ("这里是窗口顶部"),
         qt_translate ("这里包含窗口切换、标签页和常用入口。你以后会频繁从这里切"
                         "换文档和访问全局功能。"),
         "windowbar", TutorialPlacement::Bottom, 10, true},
      {"toolbar", qt_translate ("这里是主工具栏"),
         qt_translate ("常见的格式、插入和排版操作会集中在这一带。不同编辑场景下"
                         "，这里的按钮也会变化。"),
         "toolbarArea", TutorialPlacement::Bottom, 10, true},
      {"editor", qt_translate ("这里是编辑区"),
         qt_translate ("文档内容主要在这里输入、排版和修改。无论是公式、文本还是"
                         "结构化内容，核心操作都围绕这个区域展开。"),
         "editorArea", TutorialPlacement::Top, 12, true},
      {"assistant", qt_translate ("这里是扩展能力入口"),
         qt_translate ("这一侧用于放置辅助能力或扩展面板；如果当前面板未显示，教"
                         "程会退化到登录与能力入口，帮助你找到后续探索的位置。"),
         "assistantEntry", TutorialPlacement::Left, 10, true},
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
