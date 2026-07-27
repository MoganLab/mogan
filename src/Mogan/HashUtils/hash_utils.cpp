
/******************************************************************************
 * MODULE     : hash_utils.cpp
 * DESCRIPTION: Binary-safe hashing utilities (MD5 etc.)
 * COPYRIGHT  : (C) 2026  Mogan STEM authors
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "hash_utils.hpp"

#include <QByteArray>
#include <QCryptographicHash>

string
md5_binary (string s) {
  QByteArray input (as_charp (s), N (s));
  QByteArray hash= QCryptographicHash::hash (input, QCryptographicHash::Md5);
  QByteArray hex = hash.toHex ();
  return string (hex.constData (), hex.size ());
}
