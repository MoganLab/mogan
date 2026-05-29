/******************************************************************************
 * MODULE     : pdf_tile.hpp
 * DESCRIPTION: Lightweight tile wrapper for PDF tiled rendering
 * COPYRIGHT  : (C) 2026 Da Shen
 *
 * Adapted from Okular's Tile class:
 *   SPDX-FileCopyrightText: 2012 Fabio D'Urso <fabiodurso@hotmail.it>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/

#ifndef PDF_TILE_HPP
#define PDF_TILE_HPP

#include <QPixmap>
#include <QRectF>

/**
 * Represents a rectangular portion of a PDF page.
 * Does not take ownership of the pixmap.
 */
class PdfTile {
public:
  PdfTile (const QRectF& rect, QPixmap* pixmap, bool isValid)
      : m_rect (rect), m_pixmap (pixmap), m_isValid (isValid) {}

  QRectF   rect () const { return m_rect; }
  QPixmap* pixmap () const { return m_pixmap; }
  bool     isValid () const { return m_isValid; }

private:
  QRectF   m_rect;
  QPixmap* m_pixmap; // not owned
  bool     m_isValid;
};

#endif // PDF_TILE_HPP
