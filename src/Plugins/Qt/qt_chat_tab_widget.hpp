
/******************************************************************************
 * MODULE     : qt_chat_tab_widget.hpp
 * DESCRIPTION: LLM Chat tab widget for Mogan STEM
 * COPYRIGHT  : (C) 2026 Mogan STEM
 ******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QT_CHAT_TAB_WIDGET_HPP
#define QT_CHAT_TAB_WIDGET_HPP

#include "url.hpp"
#include <QList>
#include <QWidget>

#include "widget.hpp"

class QHBoxLayout;
class QLabel;
class QPushButton;
class QSpacerItem;
class QStackedWidget;
class QString;
class QVBoxLayout;

class QTChatTabWidget : public QWidget {
  Q_OBJECT

public:
  explicit QTChatTabWidget (QWidget* parent= nullptr);
  ~QTChatTabWidget () override;

protected:
  void keyPressEvent (QKeyEvent* event) override;
  void keyReleaseEvent (QKeyEvent* event) override;

private:
  struct ChatConversationPanel;

  void                   setup_left_sidebar (QVBoxLayout* sidebarLayout);
  void                   setup_right_content (QHBoxLayout* mainLayout);
  ChatConversationPanel* create_conversation (const QString& title);
  void                   create_new_conversation ();
  void                   activate_conversation (ChatConversationPanel* panel);
  void                   refresh_sidebar ();
  void                   enter_conversation_mode (ChatConversationPanel* panel);
  void                   handle_send (ChatConversationPanel* panel);
  tree read_input_message (const ChatConversationPanel* panel) const;
  void focus_input_editor (ChatConversationPanel* panel);

private:
  QWidget*                      sidebarWidget_;
  QWidget*                      contentWidget_;
  QLabel*                       conversationCountLabel_;
  QWidget*                      conversationListWidget_;
  QVBoxLayout*                  conversationListLayout_;
  QPushButton*                  newChatButton_;
  QStackedWidget*               conversationStack_;
  QList<ChatConversationPanel*> conversations_;
  ChatConversationPanel*        activeConversation_;
  int                           nextConversationTitleId_;
};

#endif // QT_CHAT_TAB_WIDGET_HPP
