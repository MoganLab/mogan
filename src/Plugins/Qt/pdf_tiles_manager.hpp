/******************************************************************************
 * MODULE     : pdf_tiles_manager.hpp
 * DESCRIPTION: Tiled rendering manager for PDF pages
 * COPYRIGHT  : (C) 2026 Da Shen
 *
 * Adapted from Okular's TilesManager:
 *   SPDX-FileCopyrightText: 2012 Mailson Menezes <mailson@gmail.com>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Manages a quadtree of tiles for a single PDF page. Initially divides the
 * page into a 4x4 grid. Tiles exceeding TILES_MAXSIZE pixels are recursively
 * split into 4 children. Only visible tiles are rendered; non-visible tiles
 * can be evicted by distance-based priority.
 ******************************************************************************/

#ifndef PDF_TILES_MANAGER_HPP
#define PDF_TILES_MANAGER_HPP

#include "pdf_tile.hpp"
#include "pdf_tile_node.hpp"

#include <QList>
#include <QRectF>

class QPixmap;

class PdfTilesManager {
public:
  enum TileLeaf {
    TerminalTile, // return leaf tiles (no children)
    PixmapTile    // return only tiles with pixmap
  };

  PdfTilesManager (int pageNumber, int width, int height);
  ~PdfTilesManager ();

  PdfTilesManager (const PdfTilesManager&)            = delete;
  PdfTilesManager& operator= (const PdfTilesManager&) = delete;

  /** Distribute a rendered pixmap to tiles covered by @p rect */
  void setPixmap (const QPixmap* pixmap, const QRectF& rect,
                  bool isPartialPixmap);

  /** Check if all tiles intersecting @p rect have valid pixmaps */
  bool hasPixmap (const QRectF& rect);

  /** Return all tiles intersecting @p rect */
  QList<PdfTile> tilesAt (const QRectF& rect, TileLeaf tileLeaf);

  /** Total memory consumed by tile pixmaps (bytes) */
  qulonglong totalMemory () const;

  /** Evict at least @p numberOfBytes from least-priority tiles.
   *  Visible tiles (intersecting @p visibleRect) are protected. */
  void cleanupPixmapMemory (qulonglong numberOfBytes,
                            const QRectF& visibleRect,
                            int           visiblePageNumber);

  /** Update page size and mark all tiles dirty */
  void setSize (int width, int height);

  void markDirty ();

  int width () const { return width_; }
  int height () const { return height_; }

private:
  bool  hasPixmapImpl (const QRectF& rect, const TileNode& tile) const;
  void  tilesAtImpl (const QRectF& rect, TileNode& tile,
                     QList<PdfTile>& result, TileLeaf tileLeaf);
  void  setPixmapImpl (const QPixmap* pixmap, const QRectF& rect,
                       TileNode& tile, bool isPartialPixmap);
  void  split (TileNode& tile, const QRectF& rect);
  bool  splitBigTiles (TileNode& tile, const QRectF& rect);
  void  deleteTiles (const TileNode& tile);
  void  markDirtyImpl (TileNode& tile);
  void  markParentDirty (const TileNode& tile);
  void  rankTiles (TileNode& tile, QList<TileNode*>& rankedTiles,
                   const QRectF& visibleRect, int visiblePageNumber);

  TileNode  tiles_[16]; // 4x4 initial grid
  int       width_;
  int       height_;
  int       pageNumber_;
  qulonglong totalPixels_;
};

#endif // PDF_TILES_MANAGER_HPP
