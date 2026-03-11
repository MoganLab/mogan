/******************************************************************************
 * MODULE     : qtm_chat_model.cpp
 * DESCRIPTION: AI Chat Message Model Implementation
 * COPYRIGHT  : (C) 2026 Mogan Team
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "qtm_chat_model.hpp"
#include "moebius/tree_label.hpp"
using namespace moebius;
#include <QDebug>
#include <fstream>
#include <sstream>

ChatModel::ChatModel() : QObject(), next_id(1) {}

ChatModel::~ChatModel() {
  messages= array<ChatMessage>(0);
  observers= array<ChatObserver*>(0);
}

void
ChatModel::attachObserver(ChatObserver* obs) {
  if (obs) {
    observers << obs;
  }
}

void
ChatModel::detachObserver(ChatObserver* obs) {
  for (int i= 0; i < N(observers); ++i) {
    if (observers[i] == obs) {
      // Remove by creating new array without this element
      array<ChatObserver*> new_observers;
      for (int j= 0; j < N(observers); ++j) {
        if (j != i) new_observers << observers[j];
      }
      observers= new_observers;
      return;
    }
  }
}

void
ChatModel::addMessage(const string& role, const string& content) {
  ChatMessage msg(next_id++, role, content);
  messages << msg;

  qDebug() << "[ChatModel] Added message:" << msg.id << "-" << as_charp(role);

  notifyMessageAdded(msg);
}

void
ChatModel::removeMessage(int msg_id) {
  for (int i= 0; i < N(messages); ++i) {
    if (messages[i].id == msg_id) {
      // Remove by creating new array without this element
      array<ChatMessage> new_messages;
      for (int j= 0; j < N(messages); ++j) {
        if (j != i) new_messages << messages[j];
      }
      messages= new_messages;
      notifyMessageRemoved(msg_id);
      return;
    }
  }
}

void
ChatModel::clearMessages() {
  messages= array<ChatMessage>(0);
  next_id= 1;
  notifyCleared();
  qDebug() << "[ChatModel] All messages cleared";
}

ChatMessage
ChatModel::getMessage(int msg_id) {
  for (int i= 0; i < N(messages); ++i) {
    if (messages[i].id == msg_id) {
      return messages[i];
    }
  }
  return ChatMessage();
}

array<ChatMessage>
ChatModel::getMessages() {
  return messages;
}

array<ChatMessage>
ChatModel::getMessages(int offset, int limit) {
  array<ChatMessage> result;
  int                start= offset;
  int                end= min(offset + limit, N(messages));

  if (start < 0) start= 0;
  if (start >= N(messages)) return result;

  for (int i= start; i < end; ++i) {
    result << messages[i];
  }

  return result;
}

int
ChatModel::getMessageCount() const {
  return N(messages);
}

tree
ChatModel::messagesToSchemeTree() {
  // Convert messages to scheme list: ((role content) ...)
  tree result= tree(TUPLE);

  for (int i= 0; i < N(messages); ++i) {
    tree msg_pair= tree(TUPLE);
    msg_pair << tree(messages[i].role);
    msg_pair << tree(messages[i].content);
    result << msg_pair;
  }

  return result;
}

bool
ChatModel::saveToFile(const string& filepath) {
  try {
    // Simple JSON-like format for easy parsing
    std::ofstream file(as_charp(filepath));
    if (!file.is_open()) {
      qDebug() << "[ChatModel] Failed to open file for writing:" << as_charp(filepath);
      return false;
    }

    file << "[\n";
    for (int i= 0; i < N(messages); ++i) {
      const ChatMessage& msg= messages[i];
      if (i > 0) file << ",\n";

      file << "  {\"id\":" << msg.id << ",\"role\":\"" << as_charp(msg.role)
           << "\",\"content\":\"";

      // Escape special characters
      for (int j= 0; j < N(msg.content); ++j) {
        char c= ((string&)msg.content)[j];
        if (c == '"') file << "\\\"";
        else if (c == '\\') file << "\\\\";
        else if (c == '\n') file << "\\n";
        else if (c == '\r') file << "\\r";
        else file << c;
      }

      file << "\",\"timestamp\":" << msg.timestamp << "}";
    }
    file << "\n]";
    file.close();

    qDebug() << "[ChatModel] Saved" << N(messages) << "messages to"
             << as_charp(filepath);

    return true;
  } catch (...) {
    qDebug() << "[ChatModel] Exception: Failed to save to" << as_charp(filepath);
    return false;
  }
}

bool
ChatModel::loadFromFile(const string& filepath) {
  try {
    std::ifstream file(as_charp(filepath));
    if (!file.is_open()) {
      qDebug() << "[ChatModel] File not found:" << as_charp(filepath);
      return false;
    }

    // For now: simple line-based parsing
    // TODO: Implement proper JSON parsing if needed
    messages= array<ChatMessage>(0);
    next_id= 1;

    std::string line;
    int         msg_count= 0;

    while (std::getline(file, line)) {
      // Very basic check - just count non-empty lines with "id"
      if (line.find("\"id\":") != std::string::npos) {
        msg_count++;
      }
    }

    file.close();

    // In a real implementation, parse JSON properly
    // For now, just log that we attempted to load
    qDebug() << "[ChatModel] Attempted to load messages from" << as_charp(filepath);

    notifyHistoryLoaded();
    return true;
  } catch (...) {
    qDebug() << "[ChatModel] Exception: Failed to load from" << as_charp(filepath);
    return false;
  }
}

void
ChatModel::notifyMessageAdded(const ChatMessage& msg) {
  for (int i= 0; i < N(observers); ++i) {
    if (observers[i]) {
      observers[i]->onMessageAdded(msg);
    }
  }
}

void
ChatModel::notifyMessageRemoved(int msg_id) {
  for (int i= 0; i < N(observers); ++i) {
    if (observers[i]) {
      observers[i]->onMessageRemoved(msg_id);
    }
  }
}

void
ChatModel::notifyCleared() {
  for (int i= 0; i < N(observers); ++i) {
    if (observers[i]) {
      observers[i]->onCleared();
    }
  }
}

void
ChatModel::notifyHistoryLoaded() {
  for (int i= 0; i < N(observers); ++i) {
    if (observers[i]) {
      observers[i]->onHistoryLoaded(messages);
    }
  }
}

int
ChatModel::generateMessageId() {
  return next_id++;
}
