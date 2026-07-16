/** \file loro.cpp
 *  \copyright GPLv3
 *  \details tree <-> Loro CRDT snapshot 的 FFI 胶水层。
 *            扁平编解码在 loro_ir.{hpp,cpp}（loro_ir_encode/decode，与
 *            3rdparty/mogan-loro-ffi/src/lib.rs 一致）。
 *            LORO_ENABLED 关时为空桩（见 loro.hpp）。
 *  \author Jim Zhou
 *  \date   2026
 */

#include "loro.hpp"

#include "loro_ir.hpp"

#include <cstdint>
#include <cstddef>

#ifdef LORO_ENABLED
// mogan-loro-ffi 暴露的 C ABI（见 3rdparty/mogan-loro-ffi/src/lib.rs）
extern "C" {
int32_t mogan_loro_encode (const uint8_t* ir, size_t ir_len, uint8_t** out,
                           size_t* out_len);
int32_t mogan_loro_decode (const uint8_t* snap, size_t snap_len, uint8_t** out,
                           size_t* out_len);
void    mogan_loro_free (uint8_t* ptr, size_t len);
}
#endif

string
tree_to_loro (tree t) {
#ifdef LORO_ENABLED
  string ir_bytes= loro_ir_encode (tree_to_loro_ir (t));

  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  int rc = mogan_loro_encode (reinterpret_cast<const uint8_t*> (ir_bytes.begin ()),
                              (size_t) N (ir_bytes), &out, &out_len);
  if (rc != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return string ();
  }
  string snapshot ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return snapshot;
#else
  (void) t;
  return string ();
#endif
}

tree
loro_to_tree (string snapshot) {
#ifdef LORO_ENABLED
  uint8_t* out    = nullptr;
  size_t   out_len= 0;
  int rc = mogan_loro_decode (
      reinterpret_cast<const uint8_t*> (snapshot.begin ()), (size_t) N (snapshot),
      &out, &out_len);
  if (rc != 0 || out == nullptr) {
    if (out) mogan_loro_free (out, out_len);
    return tree ("");
  }
  string ir_bytes ((const char*) out, (int) out_len);
  mogan_loro_free (out, out_len);
  return loro_ir_to_tree (loro_ir_decode (ir_bytes));
#else
  (void) snapshot;
  return tree ("");
#endif
}
