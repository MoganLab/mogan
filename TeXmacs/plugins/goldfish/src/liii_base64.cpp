//
// Copyright (C) 2024-2026 The Goldfish Scheme Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "s7.h"
#include <cstring>

namespace goldfish {

static s7_pointer
base64_error (s7_scheme* sc, const char* caller, const char* kind, const char* msg, s7_pointer arg) {
  return s7_error (sc, s7_make_symbol (sc, kind), s7_list (sc, 2, s7_make_string (sc, msg), arg));
}

static s7_pointer
f_bytevector_base64_decode (s7_scheme* sc, s7_pointer args) {
  s7_pointer arg= s7_car (args);

  if (!s7_is_byte_vector (arg)) {
    return base64_error (sc, "bytevector-base64-decode", "type-error",
                         "bytevector-base64-decode: input must be bytevector", arg);
  }

  s7_int   in_len= s7_integer (s7_cadr (args));
  uint8_t* in    = (uint8_t*) s7_byte_vector_elements (arg);

  if (in_len % 4 != 0) {
    return base64_error (sc, "bytevector-base64-decode", "value-error",
                         "bytevector-base64-decode: length of the input bytevector must be 4X", arg);
  }

  static const uint8_t decode_table[256]= {
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,
      255, 255, 255, 63,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 255, 255, 255, 255, 0,
      1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,
      23,  24,  25,  255, 255, 255, 255, 255, 255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
      39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
      255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};

  const uint8_t PAD= (uint8_t) '=';

  s7_int     out_cap= (in_len / 4) * 3;
  s7_pointer out    = s7_make_byte_vector (sc, out_cap, 1, NULL);
  uint8_t*   out_buf= (uint8_t*) s7_byte_vector_elements (out);
  s7_int     out_len= 0;

  for (s7_int i= 0; i < in_len; i+= 4) {
    uint8_t c1= in[i];
    uint8_t c2= in[i + 1];
    uint8_t c3= in[i + 2];
    uint8_t c4= in[i + 3];

    bool c3_pad= (c3 == PAD);
    bool c4_pad= (c4 == PAD);

    if (c1 == PAD || c2 == PAD) {
      return base64_error (sc, "bytevector-base64-decode", "value-error",
                           "bytevector-base64-decode: Invalid base64 input", arg);
    }

    uint8_t v1= decode_table[c1];
    uint8_t v2= decode_table[c2];
    uint8_t v3= c3_pad ? 0 : decode_table[c3];
    uint8_t v4= c4_pad ? 0 : decode_table[c4];

    if (v1 == 0xFF || v2 == 0xFF || (!c3_pad && v3 == 0xFF) || (!c4_pad && v4 == 0xFF) || (c3_pad && !c4_pad)) {
      return base64_error (sc, "bytevector-base64-decode", "value-error",
                           "bytevector-base64-decode: Invalid base64 input", arg);
    }

    out_buf[out_len++]= (uint8_t) ((v1 << 2) | (v2 >> 4));
    if (!c3_pad) {
      out_buf[out_len++]= (uint8_t) ((v2 << 4) | (v3 >> 2));
      if (!c4_pad) {
        out_buf[out_len++]= (uint8_t) ((v3 << 6) | v4);
      }
    }
  }

  if (out_len != out_cap) {
    s7_pointer final_out= s7_make_byte_vector (sc, out_len, 1, NULL);
    memcpy (s7_byte_vector_elements (final_out), out_buf, out_len);
    return final_out;
  }
  return out;
}

static s7_pointer
f_bytevector_base64_encode (s7_scheme* sc, s7_pointer args) {
  s7_pointer arg= s7_car (args);

  if (!s7_is_byte_vector (arg)) {
    return base64_error (sc, "bytevector-base64-encode", "type-error",
                         "bytevector-base64-encode: input must be bytevector", arg);
  }

  s7_int   in_len= s7_integer (s7_cadr (args));
  uint8_t* in    = (uint8_t*) s7_byte_vector_elements (arg);

  static const uint8_t encode_table[64]= {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
      'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
      's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

  const uint8_t PAD= (uint8_t) '=';

  s7_int     out_len= (in_len == 0) ? 0 : 4 * ((in_len + 2) / 3);
  s7_pointer out    = s7_make_byte_vector (sc, out_len, 1, NULL);
  uint8_t*   out_buf= (uint8_t*) s7_byte_vector_elements (out);

  s7_int i= 0;
  s7_int j= 0;

  while (i + 2 < in_len) {
    uint32_t triple= ((uint32_t) in[i] << 16) | ((uint32_t) in[i + 1] << 8) | in[i + 2];
    out_buf[j]     = encode_table[(triple >> 18) & 0x3F];
    out_buf[j + 1] = encode_table[(triple >> 12) & 0x3F];
    out_buf[j + 2] = encode_table[(triple >> 6) & 0x3F];
    out_buf[j + 3] = encode_table[triple & 0x3F];
    i+= 3;
    j+= 4;
  }

  s7_int rem= in_len - i;
  if (rem == 1) {
    uint32_t triple= (uint32_t) in[i] << 16;
    out_buf[j]     = encode_table[(triple >> 18) & 0x3F];
    out_buf[j + 1] = encode_table[(triple >> 12) & 0x3F];
    out_buf[j + 2] = PAD;
    out_buf[j + 3] = PAD;
  }
  else if (rem == 2) {
    uint32_t triple= ((uint32_t) in[i] << 16) | ((uint32_t) in[i + 1] << 8);
    out_buf[j]     = encode_table[(triple >> 18) & 0x3F];
    out_buf[j + 1] = encode_table[(triple >> 12) & 0x3F];
    out_buf[j + 2] = encode_table[(triple >> 6) & 0x3F];
    out_buf[j + 3] = PAD;
  }

  return out;
}

static void
glue_bytevector_base64_decode (s7_scheme* sc) {
  const char* name= "g_bytevector-base64-decode";
  const char* desc= "(g_bytevector-base64-decode bv len) => bytevector";
  s7_define_function (sc, name, f_bytevector_base64_decode, 2, 0, false, desc);
}

static void
glue_bytevector_base64_encode (s7_scheme* sc) {
  const char* name= "g_bytevector-base64-encode";
  const char* desc= "(g_bytevector-base64-encode bv len) => bytevector";
  s7_define_function (sc, name, f_bytevector_base64_encode, 2, 0, false, desc);
}

void
glue_liii_base64 (s7_scheme* sc) {
  glue_bytevector_base64_decode (sc);
  glue_bytevector_base64_encode (sc);
}

} // namespace goldfish
