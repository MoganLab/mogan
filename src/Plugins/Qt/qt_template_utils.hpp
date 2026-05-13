
/******************************************************************************
 * MODULE     : qt_template_utils.hpp
 * DESCRIPTION: Qt helpers for template document operations
 * COPYRIGHT  : (C) 2026 Yuki Lu
 ******************************************************************************/

#ifndef QT_TEMPLATE_UTILS_HPP
#define QT_TEMPLATE_UTILS_HPP

class QString;
class QWidget;

QString qt_generate_document_save_path (const QString& templateName);
QString qt_copy_template_to_documents (const QString& sourcePath,
                                       const QString& templateName);
void    qt_load_document_path (const QString& path);
bool    qt_copy_template_and_load (QWidget* parent, const QString& sourcePath,
                                   const QString& templateName);

#endif // QT_TEMPLATE_UTILS_HPP
