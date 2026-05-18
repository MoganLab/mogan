
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

/**
 * @brief LLM Chat tab widget for Mogan STEM.
 *
 * Provides a side-bar based chat interface with support for multiple
 * conversations. Each conversation owns an input area and a message
 * display area backed by embedded TeXmacs widgets.
 */
class QTChatTabWidget : public QWidget {
  Q_OBJECT

public:
  /**
   * @brief Constructs the chat tab widget.
   * @param parent Parent widget.
   */
  explicit QTChatTabWidget (QWidget* parent= nullptr);

  /**
   * @brief Destroys the widget and all conversation panels.
   */
  ~QTChatTabWidget () override;

protected:
  /**
   * @brief Forwards key press events to the Scheme layer.
   * @param event The key event.
   */
  void keyPressEvent (QKeyEvent* event) override;

  /**
   * @brief Forwards key release events to the Scheme layer.
   * @param event The key event.
   */
  void keyReleaseEvent (QKeyEvent* event) override;

private:
  /**
   * @brief Internal data for a single conversation panel.
   *
   * Holds all Qt widgets and TeXmacs buffers associated with one
   * conversation round.
   */
  struct ChatConversationPanel;

  /**
   * @brief Builds the left sidebar (title, new-chat button, conversation list).
   * @param sidebarLayout Layout to populate.
   */
  void setup_left_sidebar (QVBoxLayout* sidebarLayout);

  /**
   * @brief Builds the right content area (stacked conversation pages).
   * @param mainLayout Main horizontal layout to insert into.
   */
  void setup_right_content (QHBoxLayout* mainLayout);

  /**
   * @brief Creates a new conversation panel with widgets and buffers.
   * @param title Display title for the conversation.
   * @return Pointer to the newly created panel.
   */
  ChatConversationPanel* create_conversation (const QString& title);

  /**
   * @brief Creates and activates a new conversation with an auto-generated title.
   */
  void create_new_conversation ();

  /**
   * @brief Switches the visible page to the given conversation.
   * @param panel Conversation panel to activate.
   */
  void activate_conversation (ChatConversationPanel* panel);

  /**
   * @brief Updates sidebar labels and checked states.
   */
  void refresh_sidebar ();

  /**
   * @brief Transitions the given panel from welcome state to conversation state.
   *
   * Plays fade and spacer animations.
   * @param panel Target conversation panel.
   */
  void enter_conversation_mode (ChatConversationPanel* panel);

  /**
   * @brief Reads input, delegates to Scheme, and triggers mode transition.
   * @param panel Conversation panel to send from.
   */
  void handle_send (ChatConversationPanel* panel);

  /**
   * @brief Retrieves the document tree from the input buffer.
   * @param panel Conversation panel whose input is read.
   * @return The input body as a TeXmacs tree.
   */
  tree read_input_message (const ChatConversationPanel* panel) const;

  /**
   * @brief Sets keyboard focus to the input editor of the given panel.
   * @param panel Target conversation panel.
   */
  void focus_input_editor (ChatConversationPanel* panel);

private:
  QWidget*                      sidebarWidget_;         ///< Left sidebar container.
  QWidget*                      contentWidget_;         ///< Right content container.
  QLabel*                       conversationCountLabel_;///< Label showing conversation count.
  QWidget*                      conversationListWidget_;///< Widget holding the conversation list.
  QVBoxLayout*                  conversationListLayout_;///< Layout for conversation buttons.
  QPushButton*                  newChatButton_;         ///< Button to create a new conversation.
  QStackedWidget*               conversationStack_;     ///< Stacked widget for conversation pages.
  QList<ChatConversationPanel*> conversations_;         ///< List of all conversation panels.
  ChatConversationPanel*        activeConversation_;    ///< Currently active conversation.
  int                           nextConversationTitleId_;///< ID counter for auto-naming conversations.
};

#endif // QT_CHAT_TAB_WIDGET_HPP
