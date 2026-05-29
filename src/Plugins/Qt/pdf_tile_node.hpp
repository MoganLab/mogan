/******************************************************************************
 * MODULE     : pdf_tile_node.hpp
 * DESCRIPTION: Quadtree tile node for PDF tiled rendering
 * COPYRIGHT  : (C) 2026 Da Shen
 *
 * Adapted from Okular's TileNode:
 *   SPDX-FileCopyrightText: 2012 Mailson Menezes <mailson@gmail.com>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/

#ifndef PDF_TILE_NODE_HPP
#define PDF_TILE_NODE_HPP

#include <QRectF>

class QPixmap;

/**
 * Node in the quadtree structure used by PdfTilesManager.
 *
 * The page is initially divided into a 4x4 grid (16 tiles).
 * Each node stores a pixmap and its location on the page in
 * normalized [0,1] coordinates. Tiles larger than TILES_MAXSIZE
 * are recursively split into four children.
 */
class TileNode {
public:
  TileNode ();

  bool isValid () const;

  /** Location on the page in normalized [0,1] coordinates */
  QRectF rect;

  /** Associated pixmap or nullptr */
  QPixmap* pixmap;

  /** Whether the tile needs to be repainted */
  bool dirty;

  /** Whether the tile contains a partially rendered pixmap */
  bool partial;

  /** Distance from viewport center (Manhattan distance, for eviction) */
  double distance;

  /** Children tiles: nTiles is 0 (leaf) or 4 (split) */
  TileNode* tiles;
  int       nTiles;
  TileNode* parent;
};

#endif // PDF_TILE_NODE_HPP
