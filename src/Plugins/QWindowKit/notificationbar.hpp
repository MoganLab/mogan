/*****************************************************************************/
/* MODULE     : notificationbar.hpp                                          */
/* DESCRIPTION: SCM-driven notification bar shell                            */
/*****************************************************************************/

#ifndef NOTIFICATIONBAR_H
#define NOTIFICATIONBAR_H

#include <QFrame>

class QHBoxLayout;
class QPushButton;
class QWidget;

namespace QWK {

class NotificationBar : public QFrame {
  Q_OBJECT

public:
  explicit NotificationBar (QWidget* parent= nullptr);
  ~NotificationBar ();

  void     setContentWidget (QWidget* widget);
  QWidget* contentWidget () const;
  void     clearContent ();

signals:
  void closeRequested ();

private:
  void setupUI ();
  void updateActionStyles ();

private:
  QHBoxLayout* m_layout;
  QWidget*     m_contentHost;
  QHBoxLayout* m_contentLayout;
  QWidget*     m_contentWidget;
  QPushButton* m_closeButton;
};

} // namespace QWK

#endif // NOTIFICATIONBAR_H
