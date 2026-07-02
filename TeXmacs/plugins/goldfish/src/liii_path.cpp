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
// WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//

#include "s7.h"
#include <cstring>
#include <string>

#include <tbox/platform/file.h>
#include <tbox/tbox.h>

namespace goldfish {

using std::string;

inline void
glue_define (s7_scheme* sc, const char* name, const char* desc, s7_function f, s7_int required, s7_int optional) {
  s7_pointer cur_env= s7_curlet (sc);
  s7_pointer func   = s7_make_typed_function (sc, name, f, required, optional, false, desc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

static s7_pointer
f_isdir (s7_scheme* sc, s7_pointer args) {
  const char*    dir_c= s7_string (s7_car (args));
  tb_file_info_t info;
  bool           ret= false;
  if (tb_file_info (dir_c, &info)) {
    switch (info.type) {
    case TB_FILE_TYPE_DIRECTORY:
    case TB_FILE_TYPE_DOT:
    case TB_FILE_TYPE_DOT2:
      ret= true;
    }
  }
  return s7_make_boolean (sc, ret);
}

static void
glue_isdir (s7_scheme* sc) {
  const char* name= "g_isdir";
  const char* desc= "(g_isdir string) => boolean";
  glue_define (sc, name, desc, f_isdir, 1, 0);
}

static s7_pointer
f_isfile (s7_scheme* sc, s7_pointer args) {
  const char*    dir_c= s7_string (s7_car (args));
  tb_file_info_t info;
  bool           ret= false;
  if (tb_file_info (dir_c, &info)) {
    switch (info.type) {
    case TB_FILE_TYPE_FILE:
      ret= true;
    }
  }
  return s7_make_boolean (sc, ret);
}

static void
glue_isfile (s7_scheme* sc) {
  const char* name= "g_isfile";
  const char* desc= "(g_isfile string) => boolean";
  glue_define (sc, name, desc, f_isfile, 1, 0);
}

static s7_pointer
f_path_getsize (s7_scheme* sc, s7_pointer args) {
  const char*    path_c= s7_string (s7_car (args));
  tb_file_info_t info;
  if (tb_file_info (path_c, &info)) {
    return s7_make_integer (sc, (int) info.size);
  }
  else {
    return s7_make_integer (sc, (int) -1);
  }
}

static void
glue_path_getsize (s7_scheme* sc) {
  const char* name= "g_path-getsize";
  const char* desc= "(g_path_getsize string): string => integer";
  glue_define (sc, name, desc, f_path_getsize, 1, 0);
}

static s7_pointer
f_path_read_text (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_boolean (sc, false);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return s7_make_boolean (sc, false);
  }

  tb_file_sync (file);

  tb_size_t size= tb_file_size (file);
  if (size == 0) {
    tb_file_exit (file);
    return s7_make_string (sc, "");
  }

  tb_byte_t* buffer   = new tb_byte_t[size + 1];
  tb_size_t  real_size= tb_file_read (file, buffer, size);
  buffer[real_size]   = '\0';

  tb_file_exit (file);
  std::string content (reinterpret_cast<char*> (buffer), real_size);
  delete[] buffer;

  // Normalize line endings: convert \r\n to \n (also handles standalone \r)
  std::string normalized;
  normalized.reserve (content.size ());
  for (size_t i= 0; i < content.size (); ++i) {
    if (content[i] == '\r') {
      if (i + 1 < content.size () && content[i + 1] == '\n') {
        continue;
      }
      normalized.push_back ('\n');
    }
    else {
      normalized.push_back (content[i]);
    }
  }

  return s7_make_string (sc, normalized.c_str ());
}

static void
glue_path_read_text (s7_scheme* sc) {
  const char* name= "g_path-read-text";
  const char* desc= "(g_path-read-text path) => string, read the content of the file at the given path";
  s7_define_function (sc, name, f_path_read_text, 1, 0, false, desc);
}

static s7_pointer
f_path_read_bytes (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_boolean (sc, false);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_RO);
  if (file == tb_null) {
    return s7_make_boolean (sc, false);
  }

  tb_file_sync (file);
  tb_size_t size= tb_file_size (file);

  if (size == 0) {
    tb_file_exit (file);
    return s7_make_byte_vector (sc, 0, 1, NULL);
  }

  tb_byte_t* buffer   = new tb_byte_t[size];
  tb_size_t  real_size= tb_file_read (file, buffer, size);
  tb_file_exit (file);

  if (real_size != size) {
    delete[] buffer;
    return s7_make_boolean (sc, false);
  }

  s7_pointer bytevector     = s7_make_byte_vector (sc, real_size, 1, NULL);
  tb_byte_t* bytevector_data= s7_byte_vector_elements (bytevector);
  memcpy (bytevector_data, buffer, real_size);

  delete[] buffer;
  return bytevector;
}

static void
glue_path_read_bytes (s7_scheme* sc) {
  const char* name= "g_path-read-bytes";
  const char* desc= "(g_path-read-bytes path) => bytevector, read the binary content of the file at the given path";
  s7_define_function (sc, name, f_path_read_bytes, 1, 0, false, desc);
}

static s7_pointer
f_path_write_text (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_integer (sc, -1);
  }

  const char* content= s7_string (s7_cadr (args));
  if (!content) {
    return s7_make_integer (sc, -1);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC);
  if (file == tb_null) {
    return s7_make_integer (sc, -1);
  }

  tb_filelock_ref_t lock= tb_filelock_init (file);
  if (tb_filelock_enter (lock, TB_FILELOCK_MODE_EX) == tb_false) {
    tb_filelock_exit (lock);
    tb_file_exit (file);
    return s7_make_integer (sc, -1);
  }

  tb_size_t content_size= strlen (content);
  tb_size_t written_size= tb_file_writ (file, reinterpret_cast<const tb_byte_t*> (content), content_size);

  bool release_success= tb_filelock_leave (lock);
  tb_filelock_exit (lock);
  bool exit_success= tb_file_exit (file);

  if (written_size == content_size && release_success && exit_success) {
    return s7_make_integer (sc, written_size);
  }
  else {
    return s7_make_integer (sc, -1);
  }
}

static void
glue_path_write_text (s7_scheme* sc) {
  const char* name= "g_path-write-text";
  const char* desc= "(g_path-write-text path content) => integer,\
write content to the file at the given path and return the number of bytes written, or -1 on failure";
  s7_define_function (sc, name, f_path_write_text, 2, 0, false, desc);
}

static s7_pointer
f_path_write_bytes (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_integer (sc, -1);
  }

  s7_pointer bv= s7_cadr (args);
  if (!s7_is_byte_vector (bv)) {
    return s7_make_integer (sc, -1);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC);
  if (file == tb_null) {
    return s7_make_integer (sc, -1);
  }

  tb_filelock_ref_t lock= tb_filelock_init (file);
  if (tb_filelock_enter (lock, TB_FILELOCK_MODE_EX) == tb_false) {
    tb_filelock_exit (lock);
    tb_file_exit (file);
    return s7_make_integer (sc, -1);
  }

  tb_byte_t* data        = s7_byte_vector_elements (bv);
  tb_size_t  content_size= s7_vector_length (bv);
  tb_size_t  written_size= tb_file_writ (file, data, content_size);

  bool release_success= tb_filelock_leave (lock);
  tb_filelock_exit (lock);
  bool exit_success= tb_file_exit (file);

  if (written_size == content_size && release_success && exit_success) {
    return s7_make_integer (sc, written_size);
  }
  else {
    return s7_make_integer (sc, -1);
  }
}

static void
glue_path_write_bytes (s7_scheme* sc) {
  const char* name= "g_path-write-bytes";
  const char* desc= "(g_path-write-bytes path bytevector) => integer,\
write bytevector to the file at the given path in binary mode and return the number of bytes written, or -1 on failure";
  s7_define_function (sc, name, f_path_write_bytes, 2, 0, false, desc);
}

static s7_pointer
f_path_append_text (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_integer (sc, -1);
  }

  const char* content= s7_string (s7_cadr (args));
  if (!content) {
    return s7_make_integer (sc, -1);
  }

  tb_file_ref_t file= tb_file_init (path, TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_APPEND);
  if (file == tb_null) {
    return s7_make_integer (sc, -1);
  }

  tb_filelock_ref_t lock= tb_filelock_init (file);
  if (tb_filelock_enter (lock, TB_FILELOCK_MODE_EX) == tb_false) {
    tb_filelock_exit (lock);
    tb_file_exit (file);
    return s7_make_integer (sc, -1);
  }

  tb_size_t content_size= strlen (content);
  tb_size_t written_size= tb_file_writ (file, reinterpret_cast<const tb_byte_t*> (content), content_size);

  bool release_success= tb_filelock_leave (lock);
  tb_filelock_exit (lock);
  bool exit_success= tb_file_exit (file);

  if (written_size == content_size && release_success && exit_success) {
    return s7_make_integer (sc, written_size);
  }
  else {
    return s7_make_integer (sc, -1);
  }
}

static void
glue_path_append_text (s7_scheme* sc) {
  const char* name= "g_path-append-text";
  const char* desc= "(g_path-append-text path content) => integer,\
append content to the file at the given path and return the number of bytes written, or -1 on failure";
  s7_define_function (sc, name, f_path_append_text, 2, 0, false, desc);
}

static s7_pointer
f_path_touch (s7_scheme* sc, s7_pointer args) {
  const char* path= s7_string (s7_car (args));
  if (!path) {
    return s7_make_boolean (sc, false);
  }

  tb_bool_t success= tb_file_touch (path, 0, 0);

  if (success == tb_true) {
    return s7_make_boolean (sc, true);
  }
  else {
    return s7_make_boolean (sc, false);
  }
}

static void
glue_path_touch (s7_scheme* sc) {
  const char* name= "g_path-touch";
  const char* desc= "(g_path-touch path) => boolean, create empty file or update modification time";
  s7_define_function (sc, name, f_path_touch, 1, 0, false, desc);
}

static s7_pointer
f_path_copy (s7_scheme* sc, s7_pointer args) {
  const char* source= s7_string (s7_car (args));
  const char* target= s7_string (s7_cadr (args));

  if (!source || !target) {
    return s7_make_boolean (sc, false);
  }

  tb_file_ref_t src_file= tb_file_init (source, TB_FILE_MODE_RO);
  if (src_file == tb_null) {
    return s7_make_boolean (sc, false);
  }

  tb_file_sync (src_file);
  tb_size_t size= tb_file_size (src_file);

  tb_file_ref_t dst_file= tb_file_init (target, TB_FILE_MODE_WO | TB_FILE_MODE_CREAT | TB_FILE_MODE_TRUNC);
  if (dst_file == tb_null) {
    tb_file_exit (src_file);
    return s7_make_boolean (sc, false);
  }

  bool success= true;

  if (size > 0) {
    tb_byte_t* buffer   = new tb_byte_t[size];
    tb_size_t  read_size= tb_file_read (src_file, buffer, size);

    if (read_size != size) {
      success= false;
    }
    else {
      tb_size_t written_size= tb_file_writ (dst_file, buffer, read_size);
      if (written_size != read_size) {
        success= false;
      }
    }

    delete[] buffer;
  }

  tb_file_exit (src_file);
  tb_file_exit (dst_file);

  return s7_make_boolean (sc, success);
}

static void
glue_path_copy (s7_scheme* sc) {
  const char* name= "g_path-copy";
  const char* desc= "(g_path-copy source target) => boolean, copy file from source to target";
  s7_define_function (sc, name, f_path_copy, 2, 0, false, desc);
}

void
glue_liii_path (s7_scheme* sc) {
  glue_isfile (sc);
  glue_isdir (sc);
  glue_path_getsize (sc);
  glue_path_read_text (sc);
  glue_path_read_bytes (sc);
  glue_path_write_text (sc);
  glue_path_write_bytes (sc);
  glue_path_append_text (sc);
  glue_path_touch (sc);
  glue_path_copy (sc);
}

} // namespace goldfish
