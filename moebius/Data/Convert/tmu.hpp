
/******************************************************************************
 * MODULE     : tmu.hpp
 * DESCRIPTION: conversion between TeXmacs trees and the TMU file format
 * COPYRIGHT  : (C) 1999  Joris van der Hoeven
 *                  2024  Darcy Shen
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#ifndef TMU_H
#define TMU_H

#include "tree.hpp"

/******************************************************************************
 * Conversion of TMU strings to TeXmacs trees
 ******************************************************************************/

tree tmu_to_tree (string s);
tree tmu_to_tree (string s, string version);
tree tmu_document_to_tree (string s);

/******************************************************************************
 * Conversion of TeXmacs trees to TMU strings
 ******************************************************************************/

string tree_to_tmu (tree t);

#endif // defined TMU_H
