/******************************************************************************
 * MODULE     : pdf_tiles_manager.cpp
 * DESCRIPTION: Tiled rendering manager for PDF pages
 * COPYRIGHT  : (C) 2026 Da Shen
 *
 * Adapted from Okular's TilesManager:
 *   SPDX-FileCopyrightText: 2012 Mailson Menezes <mailson@gmail.com>
 *   SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/

#include "pdf_tiles_manager.hpp"

#include <QPainter>
#include <QPixmap>
#include <algorithm>

#define TILES_MAXSIZE 2000000

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QRect
normalizedToPixelRect (const QRectF& norm, int pageWidth, int pageHeight) {
  int l= (int) (norm.left () * pageWidth);
  int t= (int) (norm.top () * pageHeight);
  int r= (int) (norm.right () * pageWidth);
  int b= (int) (norm.bottom () * pageHeight);
  return QRect (l, t, r - l + 1, b - t + 1);
}

static bool
rankedTilesLessThan (const TileNode* t1, const TileNode* t2) {
  if (t1->dirty == t2->dirty) { return t1->distance < t2->distance; }
  return !t1->dirty;
}

// ---------------------------------------------------------------------------
// TileNode
// ---------------------------------------------------------------------------

TileNode::TileNode ()
    : pixmap (nullptr), dirty (true), partial (true), distance (-1),
      tiles (nullptr), nTiles (0), parent (nullptr) {}

bool
TileNode::isValid () const {
  return pixmap && !dirty;
}

// ---------------------------------------------------------------------------
// PdfTilesManager
// ---------------------------------------------------------------------------

PdfTilesManager::PdfTilesManager (int pageNumber, int width, int height)
    : width_ (width), height_ (height), pageNumber_ (pageNumber),
      totalPixels_ (0) {
  const double dim= 0.25;
  for (int i= 0; i < 16; ++i) {
    int x   = i % 4;
    int y   = i / 4;
    tiles_[i].rect= QRectF (x * dim, y * dim, dim, dim);
  }
}

PdfTilesManager::~PdfTilesManager () {
  for (int i= 0; i < 16; ++i) {
    deleteTiles (tiles_[i]);
  }
}

void
PdfTilesManager::deleteTiles (const TileNode& tile) {
  if (tile.pixmap) {
    totalPixels_ -= (qulonglong) tile.pixmap->width () *
                    (qulonglong) tile.pixmap->height ();
    delete tile.pixmap;
  }
  if (tile.nTiles > 0) {
    for (int i= 0; i < tile.nTiles; ++i) {
      deleteTiles (tile.tiles[i]);
    }
    delete[] tile.tiles;
  }
}

void
PdfTilesManager::setSize (int width, int height) {
  if (width == width_ && height == height_) { return; }
  width_ = width;
  height_= height;
  markDirty ();
}

void
PdfTilesManager::markDirty () {
  for (int i= 0; i < 16; ++i) {
    markDirtyImpl (tiles_[i]);
  }
}

void
PdfTilesManager::markDirtyImpl (TileNode& tile) {
  tile.dirty= true;
  for (int i= 0; i < tile.nTiles; ++i) {
    markDirtyImpl (tile.tiles[i]);
  }
}

// ---------------------------------------------------------------------------
// setPixmap
// ---------------------------------------------------------------------------

void
PdfTilesManager::setPixmap (const QPixmap* pixmap, const QRectF& rect,
                            bool isPartialPixmap) {
  for (int i= 0; i < 16; ++i) {
    setPixmapImpl (pixmap, rect, tiles_[i], isPartialPixmap);
  }
}

void
PdfTilesManager::setPixmapImpl (const QPixmap* pixmap, const QRectF& rect,
                                TileNode& tile, bool isPartialPixmap) {
  QRect pixmapRect= normalizedToPixelRect (rect, width_, height_);

  if (!tile.rect.intersects (rect)) { return; }

  // avoid painting partial pixmaps over fully rendered tiles
  if (isPartialPixmap && tile.pixmap != nullptr && !tile.partial) { return; }

  // tile is not entirely within the rendered rect — recurse into children
  if (!((tile.rect & rect) == tile.rect)) {
    if (tile.nTiles > 0) {
      for (int i= 0; i < tile.nTiles; ++i) {
        setPixmapImpl (pixmap, rect, tile.tiles[i], isPartialPixmap);
      }
      delete tile.pixmap;
      tile.pixmap= nullptr;
    }
    return;
  }

  // tile lies entirely within the rendered rect
  if (tile.nTiles == 0) {
    tile.dirty = isPartialPixmap;
    tile.partial= isPartialPixmap;

    if (!splitBigTiles (tile, rect)) {
      if (tile.pixmap) {
        totalPixels_ -= (qulonglong) tile.pixmap->width () *
                        (qulonglong) tile.pixmap->height ();
        delete tile.pixmap;
      }
      if (pixmap) {
        QRect  tilePixelRect=
            normalizedToPixelRect (tile.rect, width_, height_);
        QPoint offset= tilePixelRect.topLeft () - pixmapRect.topLeft ();
        tile.pixmap=
            new QPixmap (pixmap->copy (QRect (offset.x (), offset.y (),
                                              tilePixelRect.width (),
                                              tilePixelRect.height ())));
        totalPixels_+= (qulonglong) tile.pixmap->width () *
                       (qulonglong) tile.pixmap->height ();
      }
      else {
        tile.pixmap= nullptr;
      }
    }
    else {
      // was split — distribute to new children
      if (tile.pixmap) {
        totalPixels_ -= (qulonglong) tile.pixmap->width () *
                        (qulonglong) tile.pixmap->height ();
        delete tile.pixmap;
        tile.pixmap= nullptr;
      }
      for (int i= 0; i < tile.nTiles; ++i) {
        setPixmapImpl (pixmap, rect, tile.tiles[i], isPartialPixmap);
      }
    }
  }
  else {
    QRect tileRect= normalizedToPixelRect (tile.rect, width_, height_);
    if (tileRect.width () * tileRect.height () >= TILES_MAXSIZE ||
        isPartialPixmap) {
      tile.dirty  = isPartialPixmap;
      tile.partial= isPartialPixmap;
      if (tile.pixmap) {
        totalPixels_ -= (qulonglong) tile.pixmap->width () *
                        (qulonglong) tile.pixmap->height ();
        delete tile.pixmap;
        tile.pixmap= nullptr;
      }
      for (int i= 0; i < tile.nTiles; ++i) {
        setPixmapImpl (pixmap, rect, tile.tiles[i], isPartialPixmap);
      }
    }
    else {
      // merge children back into this tile
      for (int i= 0; i < tile.nTiles; ++i) {
        deleteTiles (tile.tiles[i]);
      }
      delete[] tile.tiles;
      tile.tiles  = nullptr;
      tile.nTiles = 0;

      if (tile.pixmap) {
        totalPixels_ -= (qulonglong) tile.pixmap->width () *
                        (qulonglong) tile.pixmap->height ();
        delete tile.pixmap;
      }
      if (pixmap) {
        QRect  tilePixelRect=
            normalizedToPixelRect (tile.rect, width_, height_);
        QPoint offset= tilePixelRect.topLeft () - pixmapRect.topLeft ();
        tile.pixmap=
            new QPixmap (pixmap->copy (QRect (offset.x (), offset.y (),
                                              tilePixelRect.width (),
                                              tilePixelRect.height ())));
        totalPixels_+= (qulonglong) tile.pixmap->width () *
                       (qulonglong) tile.pixmap->height ();
      }
      else {
        tile.pixmap= nullptr;
      }
      tile.dirty  = isPartialPixmap;
      tile.partial= isPartialPixmap;
    }
  }
}

// ---------------------------------------------------------------------------
// hasPixmap
// ---------------------------------------------------------------------------

bool
PdfTilesManager::hasPixmap (const QRectF& rect) {
  for (int i= 0; i < 16; ++i) {
    if (!hasPixmapImpl (rect, tiles_[i])) { return false; }
  }
  return true;
}

bool
PdfTilesManager::hasPixmapImpl (const QRectF&     rect,
                                const TileNode& tile) const {
  QRectF intersection= tile.rect.intersected (rect);
  if (intersection.width () <= 0 || intersection.height () <= 0) {
    return true;
  }

  if (tile.nTiles == 0) { return tile.isValid (); }
  if (!tile.dirty) { return true; }

  for (int i= 0; i < tile.nTiles; ++i) {
    if (!hasPixmapImpl (rect, tile.tiles[i])) { return false; }
  }
  return true;
}

// ---------------------------------------------------------------------------
// tilesAt
// ---------------------------------------------------------------------------

QList<PdfTile>
PdfTilesManager::tilesAt (const QRectF& rect, TileLeaf tileLeaf) {
  QList<PdfTile> result;
  for (int i= 0; i < 16; ++i) {
    tilesAtImpl (rect, tiles_[i], result, tileLeaf);
  }
  return result;
}

void
PdfTilesManager::tilesAtImpl (const QRectF& rect, TileNode& tile,
                              QList<PdfTile>& result, TileLeaf tileLeaf) {
  if (!tile.rect.intersects (rect)) { return; }

  splitBigTiles (tile, rect);

  if ((tileLeaf == TerminalTile && tile.nTiles == 0) ||
      (tileLeaf == PixmapTile && tile.pixmap)) {
    result.append (PdfTile (tile.rect, tile.pixmap, tile.isValid ()));
  }
  else {
    for (int i= 0; i < tile.nTiles; ++i) {
      tilesAtImpl (rect, tile.tiles[i], result, tileLeaf);
    }
  }
}

// ---------------------------------------------------------------------------
// split
// ---------------------------------------------------------------------------

void
PdfTilesManager::split (TileNode& tile, const QRectF& rect) {
  if (tile.nTiles != 0) { return; }
  if (rect.isNull () || !tile.rect.intersects (rect)) { return; }

  tile.nTiles= 4;
  tile.tiles = new TileNode[4];

  double hCenter= (tile.rect.left () + tile.rect.right ()) / 2.0;
  double vCenter= (tile.rect.top () + tile.rect.bottom ()) / 2.0;

  tile.tiles[0].rect=
      QRectF (tile.rect.left (), tile.rect.top (), hCenter - tile.rect.left (),
              vCenter - tile.rect.top ());
  tile.tiles[1].rect=
      QRectF (hCenter, tile.rect.top (), tile.rect.right () - hCenter,
              vCenter - tile.rect.top ());
  tile.tiles[2].rect=
      QRectF (tile.rect.left (), vCenter, hCenter - tile.rect.left (),
              tile.rect.bottom () - vCenter);
  tile.tiles[3].rect=
      QRectF (hCenter, vCenter, tile.rect.right () - hCenter,
              tile.rect.bottom () - vCenter);

  for (int i= 0; i < tile.nTiles; ++i) {
    tile.tiles[i].parent= &tile;
    splitBigTiles (tile.tiles[i], rect);
  }
}

bool
PdfTilesManager::splitBigTiles (TileNode& tile, const QRectF& rect) {
  QRect tileRect= normalizedToPixelRect (tile.rect, width_, height_);
  if (tileRect.width () * tileRect.height () < TILES_MAXSIZE) { return false; }
  split (tile, rect);
  return true;
}

// ---------------------------------------------------------------------------
// Memory management
// ---------------------------------------------------------------------------

qulonglong
PdfTilesManager::totalMemory () const {
  return 4 * totalPixels_;
}

void
PdfTilesManager::cleanupPixmapMemory (qulonglong    numberOfBytes,
                                      const QRectF& visibleRect,
                                      int           visiblePageNumber) {
  QList<TileNode*> rankedTiles;
  for (int i= 0; i < 16; ++i) {
    rankTiles (tiles_[i], rankedTiles, visibleRect, visiblePageNumber);
  }
  std::sort (rankedTiles.begin (), rankedTiles.end (), rankedTilesLessThan);

  while (numberOfBytes > 0 && !rankedTiles.isEmpty ()) {
    TileNode* tile= rankedTiles.takeLast ();
    if (!tile->pixmap) { continue; }
    if (tile->rect.intersects (visibleRect)) { continue; }

    qulonglong pixels=
        (qulonglong) tile->pixmap->width () *
        (qulonglong) tile->pixmap->height ();
    totalPixels_-= pixels;
    if (numberOfBytes < 4 * pixels) {
      numberOfBytes= 0;
    }
    else {
      numberOfBytes-= 4 * pixels;
    }

    delete tile->pixmap;
    tile->pixmap= nullptr;
    tile->partial= true;

    markParentDirty (*tile);
  }
}

void
PdfTilesManager::markParentDirty (const TileNode& tile) {
  if (!tile.parent) { return; }
  if (!tile.parent->dirty) {
    tile.parent->dirty= true;
    markParentDirty (*tile.parent);
  }
}

void
PdfTilesManager::rankTiles (TileNode& tile, QList<TileNode*>& rankedTiles,
                            const QRectF& visibleRect,
                            int           visiblePageNumber) {
  if (visibleRect.isNull () && visiblePageNumber < 0) { return; }

  if (tile.pixmap) {
    if (!visibleRect.isNull ()) {
      QPointF viewportCenter= visibleRect.center ();
      QPointF tileCenter    = tile.rect.center ();
      tile.distance         = qAbs (viewportCenter.x () - tileCenter.x ()) +
                      qAbs (viewportCenter.y () - tileCenter.y ());
    }
    else {
      if (pageNumber_ < visiblePageNumber) {
        tile.distance= 1.0 - tile.rect.bottom ();
      }
      else {
        tile.distance= tile.rect.top ();
      }
    }
    rankedTiles.append (&tile);
  }
  else {
    for (int i= 0; i < tile.nTiles; ++i) {
      rankTiles (tile.tiles[i], rankedTiles, visibleRect,
                 visiblePageNumber);
    }
  }
}
