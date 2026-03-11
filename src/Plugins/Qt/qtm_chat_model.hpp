/******************************************************************************
 * MODULE     : qtm_chat_model.hpp
 * DESCRIPTION: AI Chat Message Model for Mogan
 * COPYRIGHT  : (C) 2026 Mogan Team
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef QTM_CHAT_MODEL_HPP
#define QTM_CHAT_MODEL_HPP

#include "array.hpp"
#include "string.hpp"
#include "tree.hpp"
#include <ctime>
#include <QObject>

/******************************************************************************
 * ChatMessage Data Structure
 ******************************************************************************/

class ChatMessage {
public:
  int         id;           // Unique message ID
  string      role;         // "user" or "assistant"
  string      content;      // Main message content
  long        timestamp;    // Unix timestamp
  tree        metadata;     // Rich content metadata (Markdown, code blocks, etc.)

  ChatMessage()
      : id(0), role(""), content(""), timestamp(0), metadata(false) {}

  ChatMessage(int _id, const string& _role, const string& _content)
      : id(_id), role(_role), content(_content), timestamp(time(NULL)),
        metadata(false) {}
};

/******************************************************************************
 * Observer Interface
 ******************************************************************************/

class ChatObserver {
public:
  virtual ~ChatObserver() {}

  // Called when a new message is added
  virtual void onMessageAdded(const ChatMessage& msg) = 0;

  // Called when a message is removed
  virtual void onMessageRemoved(int msg_id) = 0;

  // Called when all messages are cleared
  virtual void onCleared() = 0;

  // Called when messages are loaded from history
  virtual void onHistoryLoaded(const array<ChatMessage>& messages) = 0;
};

/******************************************************************************
 * ChatModel - Core Data Model
 ******************************************************************************/

class ChatModel : public QObject {
  Q_OBJECT

private:
  array<ChatMessage>    messages;
  array<ChatObserver*>  observers;
  int                   next_id;

public:
  ChatModel();
  ~ChatModel();

  // ========== Observer Management ==========
  void attachObserver(ChatObserver* obs);
  void detachObserver(ChatObserver* obs);

  // ========== Message Operations ==========
  void addMessage(const string& role, const string& content);
  void removeMessage(int msg_id);
  void clearMessages();

  // ========== Query Operations ==========
  ChatMessage getMessage(int msg_id);
  array<ChatMessage> getMessages();
  array<ChatMessage> getMessages(int offset, int limit);
  int getMessageCount() const;

  // ========== Persistence ==========
  bool saveToFile(const string& filepath);
  bool loadFromFile(const string& filepath);

  // ========== Scheme Integration ==========
  // Convert messages to scheme tree for Scheme layer access
  tree messagesToSchemeTree();

private:
  // Notify all observers of message addition
  void notifyMessageAdded(const ChatMessage& msg);

  // Notify all observers of message removal
  void notifyMessageRemoved(int msg_id);

  // Notify all observers of clear
  void notifyCleared();

  // Notify all observers of history load
  void notifyHistoryLoaded();

  // Generate unique message ID
  int generateMessageId();
};

#endif // QTM_CHAT_MODEL_HPP
