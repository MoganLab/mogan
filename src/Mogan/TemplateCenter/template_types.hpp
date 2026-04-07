
/******************************************************************************
 * MODULE     : template_types.hpp
 * DESCRIPTION: Common type definitions for Mogan Template Center
 * COPYRIGHT  : (C) 2026 Yuki Lu
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TEMPLATE_TYPES_HPP
#define TEMPLATE_TYPES_HPP

#include <QDateTime>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

/**
 * @brief Template category structure (liiistem.cn API format)
 */
struct TemplateCategory {
  QString id;          // Unique category identifier
  QString name;        // Display name (localized)
  QString description; // Category description
  QString icon;        // Icon emoji or name
  int     order;       // Display order

  TemplateCategory () : order (0) {}
};

/**
 * @brief Template metadata structure (liiistem.cn API format)
 */
struct TemplateMetadata {
  QString     id;              // Unique template identifier
  QString     name;            // Display name
  QString     description;     // Template description
  QString     category;        // Category ID
  QString     author;          // Author name
  QString     version;         // Template version
  QString     license;         // License info (e.g., "CC-BY-NC-SA 4.0")
  QString     thumbnailUrl;    // Thumbnail image URL (small)
  QString     previewUrl;      // Preview image URL (large)
  QString     fileUrl;         // Template file (.tm) download URL
  qint64      fileSize;        // File size in bytes
  QString     fileMd5;         // File MD5 checksum
  QDateTime   createdAt;       // Creation time
  QDateTime   updatedAt;       // Last update time
  QString     language;        // Language code (e.g., "zh-CN")
  QStringList tags;            // Template tags
  QString     moganMinVersion; // Minimum Mogan version required
  int         downloadCount;   // Download statistics
  double      rating;          // Rating (0-5)
  QString     localPath;       // Local cached file path (if downloaded)
  bool        isLocal;         // Whether template is locally available

  TemplateMetadata ()
      : fileSize (0), downloadCount (0), rating (0.0), isLocal (false) {}
};

using TemplateMetadataPtr= QSharedPointer<TemplateMetadata>;

#endif // TEMPLATE_TYPES_HPP
