#ifndef QT_TUTORIAL_HPP
#define QT_TUTORIAL_HPP

#include "url.hpp"

#include <QHash>
#include <QMainWindow>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QString>
#include <QVector>
#include <QWidget>

#include <functional>

class QLabel;
class QMovie;
class QPushButton;
class QEvent;
class QMouseEvent;
class QPaintEvent;
class QWheelEvent;

namespace QWK {

enum class TutorialPlacement { Auto, Top, Bottom, Left, Right };
enum class TutorialBubbleSize { Small, Medium, Large };

enum class TutorialFinishReason { Completed, Skipped, Cancelled, HostClosed };

struct TutorialStepConfig {
  QString            id;
  QString            title;
  QString            targetId;
  TutorialPlacement  placement       = TutorialPlacement::Auto;
  int                highlightPadding= 8;
  bool               skipIfMissing   = true;
  QString            topText;
  QString            mediaPath;
  QString            bottomText;
  QString            onEnterCommand;
  TutorialBubbleSize bubbleSize= TutorialBubbleSize::Medium;
  int                offsetX   = 0;
  int                offsetY   = 0;
};

struct TutorialFlowConfig {
  QString                     flowId;
  int                         version= 1;
  QVector<TutorialStepConfig> steps;

  bool isValid () const { return !flowId.isEmpty () && !steps.isEmpty (); }
};

class TutorialTargetRegistry {
public:
  using RectProvider= std::function<QRect (QMainWindow*)>;

  void registerWidget (const QString& id, QWidget* widget);
  void registerRectProvider (const QString& id, RectProvider provider);
  bool resolve (const QString& id, QMainWindow* window, QRect& rect) const;

private:
  QHash<QString, QPointer<QWidget>> m_widgetAnchors;
  QHash<QString, RectProvider>      m_rectProviders;
};

class TutorialConfigLoader {
public:
  static bool loadFlow (url path, TutorialFlowConfig& config,
                        QString* errorMessage= nullptr);
};

class TutorialBubble : public QWidget {
  Q_OBJECT

public:
  explicit TutorialBubble (QWidget* parent= nullptr);

  void setStep (const TutorialStepConfig& step, int index, int total);
  void setFirstStep (bool first);
  void setLastStep (bool last);

signals:
  void previousRequested ();
  void nextRequested ();
  void finishRequested ();

private:
  QLabel*      m_titleLabel;
  QLabel*      m_topTextLabel;
  QWidget*     m_mediaContainer;
  QLabel*      m_mediaLabel;
  QLabel*      m_bottomTextLabel;
  QLabel*      m_progressLabel;
  QPushButton* m_previousButton;
  QPushButton* m_nextButton;
  QMovie*      m_mediaMovie;
  QString      m_currentMediaPath;
};

class TutorialOverlay : public QWidget {
  Q_OBJECT

public:
  explicit TutorialOverlay (QMainWindow* parentWindow);

  void setStep (const TutorialStepConfig& step, int index, int total);
  void setHighlightedRect (const QRect& rect, int padding);
  void clearHighlight ();
  void repositionBubble (TutorialPlacement placement);

signals:
  void previousRequested ();
  void nextRequested ();
  void finishRequested ();

protected:
  void paintEvent (QPaintEvent* event) override;
  void mousePressEvent (QMouseEvent* event) override;
  void mouseReleaseEvent (QMouseEvent* event) override;
  void mouseMoveEvent (QMouseEvent* event) override;
  void wheelEvent (QWheelEvent* event) override;

private:
  void updateInputMask ();
  void refreshExposedArea (const QRect& rect);

  QRect bubbleRectForPlacement (TutorialPlacement placement) const;

  QPointer<QMainWindow> m_parentWindow;
  TutorialBubble*       m_bubble;
  QRect                 m_highlightRect;
  TutorialStepConfig    m_currentStep;
  bool                  m_hasHighlight;
};

class TutorialEngine : public QObject {
  Q_OBJECT

public:
  explicit TutorialEngine (QObject* parent= nullptr);

  bool start (QMainWindow* hostWindow, const TutorialFlowConfig& config,
              const TutorialTargetRegistry& registry);
  void stop (TutorialFinishReason reason= TutorialFinishReason::Cancelled);
  void next ();
  void previous ();

  bool isActive () const;
  bool isActiveForWindow (QMainWindow* mainWindow) const;

signals:
  void finished (TutorialFinishReason reason);
  void stepChanged (const QString& stepId, int index, int total);

protected:
  bool eventFilter (QObject* watched, QEvent* event) override;

private:
  void executeOnEnter (const TutorialStepConfig& step);
  void refreshCurrentStepGeometry ();
  void updateOverlayGeometry ();
  void showStep (int index, int retryCount= 0, int fallbackDirection= 0,
                 int requestId= -1);
  void showNextAvailableStep (int startIndex, int direction);

  static constexpr int kMaxResolveRetries= 10;

  QPointer<QMainWindow>     m_hostWindow;
  QPointer<TutorialOverlay> m_overlay;
  TutorialTargetRegistry    m_registry;
  TutorialFlowConfig        m_config;
  int                       m_currentIndex;
  int                       m_displayedIndex;
  int                       m_stepRequestId;
};

class FirstLaunchTutorialController : public QObject {
  Q_OBJECT

public:
  static FirstLaunchTutorialController* instance ();

  void maybeStartForMainWindow (QMainWindow* mainWindow);

private:
  explicit FirstLaunchTutorialController (QObject* parent= nullptr);

  TutorialFlowConfig     loadFirstLaunchFlow () const;
  TutorialFlowConfig     buildFallbackFlow () const;
  TutorialTargetRegistry buildRegistry (QMainWindow* mainWindow) const;
  bool                   shouldStart (const TutorialFlowConfig& flow) const;
  QString                versionString (int version) const;
  QString preferenceKey (const QString& prefix, const QString& flowId) const;

  TutorialEngine* m_engine;
  bool            m_startedThisSession;
};

} // namespace QWK

#endif // QT_TUTORIAL_HPP
