//
// Copyright (C) 2026 The Goldfish Scheme Authors
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

#include "liii_date_time_embedded_tz.h"
#include "s7.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>

#include <tbox/tbox.h>

#ifdef TB_CONFIG_OS_WINDOWS
#include <windows.h>
#else
#include <sys/stat.h>
#endif

namespace goldfish {

static s7_pointer
f_system_timezone_offset_minutes (s7_scheme* sc, s7_pointer args) {
#ifdef TB_CONFIG_OS_WINDOWS
  TIME_ZONE_INFORMATION tzi;
  DWORD                 res = GetTimeZoneInformation (&tzi);
  long                  bias= tzi.Bias;
  if (res == TIME_ZONE_ID_DAYLIGHT) {
    bias+= tzi.DaylightBias;
  }
  else if (res == TIME_ZONE_ID_STANDARD) {
    bias+= tzi.StandardBias;
  }
  return s7_make_integer (sc, -bias);
#else
  time_t    t= time (NULL);
  struct tm lt;
  localtime_r (&t, &lt);
  return s7_make_integer (sc, lt.tm_gmtoff / 60);
#endif
}

static s7_pointer
make_bytevector_from_data (s7_scheme* sc, const unsigned char* data, size_t len) {
  s7_pointer bv      = s7_make_byte_vector (sc, (s7_int) len, 1, NULL);
  uint8_t*   elements= s7_byte_vector_elements (bv);
  memcpy (elements, data, len);
  return bv;
}

static s7_pointer
f_get_zoneinfo (s7_scheme* sc, s7_pointer args) {
  s7_pointer name_arg= s7_car (args);
  if (!s7_is_string (name_arg)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_cons (sc, s7_make_string (sc, "g_get_zoneinfo: expected a string"), args));
  }
  const char* name= s7_string (name_arg);

  // 1. Try reading from filesystem if path exists
  std::string full_path= std::string ("/usr/share/zoneinfo/") + name;
  const char* tzpath   = getenv ("TZPATH");
  if (tzpath && strlen (tzpath) > 0) {
    full_path= std::string (tzpath) + "/" + name;
  }

  FILE* fp= fopen (full_path.c_str (), "rb");
  if (fp) {
    fseek (fp, 0, SEEK_END);
    long sz= ftell (fp);
    fseek (fp, 0, SEEK_SET);
    if (sz >= 4) {
      std::vector<unsigned char> buf (sz);
      size_t                     read_bytes= fread (buf.data (), 1, sz, fp);
      fclose (fp);
      if (read_bytes == (size_t) sz) {
        return make_bytevector_from_data (sc, buf.data (), buf.size ());
      }
    }
    else {
      fclose (fp);
    }
  }

  // 2. Fallback to embedded timezone data (e.g. for Windows or minimal containers)
  for (size_t i= 0; i < sizeof (goldfish_embedded_tzs) / sizeof (goldfish_embedded_tzs[0]); ++i) {
    if (strcmp (name, goldfish_embedded_tzs[i].name) == 0) {
      return make_bytevector_from_data (sc, goldfish_embedded_tzs[i].data, goldfish_embedded_tzs[i].len);
    }
  }

  return s7_f (sc);
}

static s7_pointer
f_zoneinfo_available_p (s7_scheme* sc, s7_pointer args) {
#ifdef TB_CONFIG_OS_WINDOWS
  return s7_f (sc);
#else
  struct stat st;
  if (stat ("/usr/share/zoneinfo", &st) == 0 && S_ISDIR (st.st_mode)) {
    return s7_t (sc);
  }
  return s7_f (sc);
#endif
}

static s7_pointer
f_embedded_zoneinfo_names (s7_scheme* sc, s7_pointer args) {
  s7_pointer names= s7_nil (sc);
  for (int i= (int) (sizeof (goldfish_embedded_tzs) / sizeof (goldfish_embedded_tzs[0])) - 1; i >= 0; --i) {
    names= s7_cons (sc, s7_make_string (sc, goldfish_embedded_tzs[i].name), names);
  }
  return names;
}

void
glue_liii_date_time (s7_scheme* sc) {
  s7_define_function (sc, "g_system_timezone_offset_minutes", goldfish::f_system_timezone_offset_minutes, 0, 0, false,
                      "(g_system_timezone_offset_minutes) => integer");
  s7_define_function (sc, "g_get_zoneinfo", goldfish::f_get_zoneinfo, 1, 0, false,
                      "(g_get_zoneinfo name) => bytevector or #f");
  s7_define_function (sc, "g_zoneinfo_available?", goldfish::f_zoneinfo_available_p, 0, 0, false,
                      "(g_zoneinfo_available?) => boolean");
  s7_define_function (sc, "g_embedded_zoneinfo_names", goldfish::f_embedded_zoneinfo_names, 0, 0, false,
                      "(g_embedded_zoneinfo_names) => list of embedded timezone names");
}

} // namespace goldfish
