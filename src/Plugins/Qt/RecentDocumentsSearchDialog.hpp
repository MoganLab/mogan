/******************************************************************************
 * MODULE      : RecentDocumentsSearchDialog.hpp
 * DESCRIPTION : 最近打开文档搜索 QML 对话框的入口。
 * COPYRIGHT   : (C) 2026 Mogan STEM
 *
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY whatsoever. Details see LICENSE.
 ******************************************************************************/

#ifndef RECENT_DOCUMENTS_SEARCH_DIALOG_HPP
#define RECENT_DOCUMENTS_SEARCH_DIALOG_HPP

#include "string.hpp"

/**
 * @brief Open the QML search dialog for recently opened documents.
 *
 * @return The selected document path, or an empty string after Cancel, Esc,
 * window close, or QML loading failure.
 */
string cpp_search_recent_documents_dialog ();

#endif // defined RECENT_DOCUMENTS_SEARCH_DIALOG_HPP
