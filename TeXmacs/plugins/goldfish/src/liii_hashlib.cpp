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
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "s7.h"
#include <tbox/tbox.h>

namespace goldfish {

static void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional) {
  s7_pointer cur_env= s7_curlet (sc);
  s7_pointer func   = s7_make_typed_function (sc, name, f, required, optional, false, desc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

static void
hash_bytes_to_hex (const tb_byte_t* bytes, tb_size_t length, tb_char_t* hex_output) {
  static const tb_char_t hex_digits[]= "0123456789abcdef";
  for (tb_size_t i= 0; i < length; ++i) {
    hex_output[i * 2]    = hex_digits[bytes[i] >> 4];
    hex_output[i * 2 + 1]= hex_digits[bytes[i] & 0x0f];
  }
  hex_output[length * 2]= '\0';
}

static bool
md5_file_to_hex (const char* path, tb_char_t* hex_output) {
  if (!path) {
    return false;
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return false;
  }

  tb_md5_t md5;
  tb_md5_init (&md5, 0);

  tb_size_t size  = tb_file_size (file);
  tb_size_t offset= 0;
  tb_byte_t buffer[4096];
  while (offset < size) {
    tb_size_t want     = ((size - offset) > sizeof (buffer)) ? sizeof (buffer) : (size - offset);
    tb_size_t real_size= tb_file_read (file, buffer, want);
    if (real_size == 0) {
      tb_file_exit (file);
      return false;
    }
    tb_md5_spak (&md5, buffer, real_size);
    offset += real_size;
  }

  tb_file_exit (file);

  tb_byte_t digest[16];
  tb_md5_exit (&md5, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return true;
}

static bool
sha_file_to_hex (const char* path, tb_size_t mode, tb_size_t digest_size, tb_char_t* hex_output) {
  if (!path) {
    return false;
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return false;
  }

  tb_sha_t sha;
  tb_sha_init (&sha, mode);

  tb_size_t size  = tb_file_size (file);
  tb_size_t offset= 0;
  tb_byte_t buffer[4096];
  while (offset < size) {
    tb_size_t want     = ((size - offset) > sizeof (buffer)) ? sizeof (buffer) : (size - offset);
    tb_size_t real_size= tb_file_read (file, buffer, want);
    if (real_size == 0) {
      tb_file_exit (file);
      return false;
    }
    tb_sha_spak (&sha, buffer, real_size);
    offset += real_size;
  }

  tb_file_exit (file);

  tb_byte_t digest[32];
  tb_sha_exit (&sha, digest, digest_size);
  hash_bytes_to_hex (digest, digest_size, hex_output);
  return true;
}

static s7_pointer
f_md5 (s7_scheme* sc, s7_pointer args) {
  const char* search_string= s7_string (s7_car (args));
  tb_size_t   len          = tb_strlen (search_string);
  tb_byte_t   digest[16];
  tb_char_t   hex_output[33]= {0};
  tb_md5_t    md5;

  tb_md5_init (&md5, 0);
  if (len > 0) {
    tb_md5_spak (&md5, (tb_byte_t const*) search_string, len);
  }
  tb_md5_exit (&md5, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return s7_make_string (sc, hex_output);
}

static void
glue_md5 (s7_scheme* sc) {
  const char* name= "g_md5";
  const char* desc= "(g_md5 str) => string";
  glue_define (sc, name, desc, f_md5, 1, 0);
}

static s7_pointer
f_md5_file (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  tb_char_t   hex_output[33]= {0};
  if (!md5_file_to_hex (path, hex_output)) {
    return s7_make_boolean (sc, false);
  }
  return s7_make_string (sc, hex_output);
}

static void
glue_md5_file (s7_scheme* sc) {
  const char* name= "g_md5-by-file";
  const char* desc= "(g_md5-by-file path) => string|#f";
  glue_define (sc, name, desc, f_md5_file, 1, 0);
}

static s7_pointer
f_sha1 (s7_scheme* sc, s7_pointer args) {
  const char* search_string= s7_string (s7_car (args));
  tb_size_t   len          = tb_strlen (search_string);
  tb_byte_t   digest[20];
  tb_char_t   hex_output[41]= {0};
  tb_sha_t    sha;

  tb_sha_init (&sha, 160);
  if (len > 0) {
    tb_sha_spak (&sha, (tb_byte_t const*) search_string, len);
  }
  tb_sha_exit (&sha, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return s7_make_string (sc, hex_output);
}

static void
glue_sha1 (s7_scheme* sc) {
  const char* name= "g_sha1";
  const char* desc= "(g_sha1 str) => string";
  glue_define (sc, name, desc, f_sha1, 1, 0);
}

static s7_pointer
f_sha1_file (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  tb_char_t   hex_output[41]= {0};
  if (!sha_file_to_hex (path, 160, 20, hex_output)) {
    return s7_make_boolean (sc, false);
  }
  return s7_make_string (sc, hex_output);
}

static void
glue_sha1_file (s7_scheme* sc) {
  const char* name= "g_sha1-by-file";
  const char* desc= "(g_sha1-by-file path) => string|#f";
  glue_define (sc, name, desc, f_sha1_file, 1, 0);
}

static s7_pointer
f_sha256 (s7_scheme* sc, s7_pointer args) {
  const char* search_string= s7_string (s7_car (args));
  tb_size_t   len          = tb_strlen (search_string);
  tb_byte_t   digest[32];
  tb_char_t   hex_output[65]= {0};
  tb_sha_t    sha;

  tb_sha_init (&sha, 256);
  if (len > 0) {
    tb_sha_spak (&sha, (tb_byte_t const*) search_string, len);
  }
  tb_sha_exit (&sha, digest, sizeof (digest));
  hash_bytes_to_hex (digest, sizeof (digest), hex_output);
  return s7_make_string (sc, hex_output);
}

static void
glue_sha256 (s7_scheme* sc) {
  const char* name= "g_sha256";
  const char* desc= "(g_sha256 str) => string";
  glue_define (sc, name, desc, f_sha256, 1, 0);
}

static s7_pointer
f_sha256_file (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  tb_char_t   hex_output[65]= {0};
  if (!sha_file_to_hex (path, 256, 32, hex_output)) {
    return s7_make_boolean (sc, false);
  }
  return s7_make_string (sc, hex_output);
}

static void
glue_sha256_file (s7_scheme* sc) {
  const char* name= "g_sha256-by-file";
  const char* desc= "(g_sha256-by-file path) => string|#f";
  glue_define (sc, name, desc, f_sha256_file, 1, 0);
}

void
glue_liii_hashlib (s7_scheme* sc) {
  glue_md5 (sc);
  glue_md5_file (sc);
  glue_sha1 (sc);
  glue_sha1_file (sc);
  glue_sha256 (sc);
  glue_sha256_file (sc);
}

} // namespace goldfish
