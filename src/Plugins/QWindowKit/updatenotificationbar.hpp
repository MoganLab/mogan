/*****************************************************************************/
/* MODULE     : updatenotificationbar.hpp                                    */
/* DESCRIPTION : Version update notification bar (horizontal layout)         */
/* COPYRIGHT   : (C) 2026 Mogan STEM authors                                 */
/*****************************************************************************/

#ifndef UPDATENOTIFICATIONBAR_H
#define UPDATENOTIFICATIONBAR_H

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

namespace QWK {

class UpdateNotificationBar : public QFrame {
  Q_OBJECT

public:
  explicit UpdateNotificationBar (QWidget* parent= nullptr);
  ~UpdateNotificationBar ();

  void setVersionInfo (const QString& currentVersion,
                       const QString& remoteVersion);

signals:
  void updateNowRequested ();
  void snoozeRequested ();
  void closeRequested ();

private:
  void setupUI ();

  QHBoxLayout* m_layout;
  QLabel*      m_iconLabel;
  QLabel*      m_messageLabel;
  QPushButton* m_updateNowBtn;
  QPushButton* m_snoozeBtn;
  QPushButton* m_closeBtn;
};

} // namespace QWK

#endif // UPDATENOTIFICATIONBAR_H
