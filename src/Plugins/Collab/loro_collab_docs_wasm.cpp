/******************************************************************************
 * MODULE     : loro_collab_docs_wasm.cpp
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "loro_collab.hpp"
#include "url.hpp"
#include <emscripten.h>
#include <string>

enum class docs_status { idle, loading, ready, error };

static docs_status g_docs_status= docs_status::idle;
static std::string g_docs_data;

extern "C" EMSCRIPTEN_KEEPALIVE void
mogan_collab_docs_received (const char* text, int ok) {
  if (ok == 2) {
    g_docs_status= docs_status::ready;
    g_docs_data  = (text != nullptr) ? std::string (text) : std::string ();
  }
  else {
    g_docs_status= docs_status::error;
    g_docs_data.clear ();
  }
}

EM_JS_DEPS (mogan_collab, "$ccall");

EM_JS (char*, collab_read_server_url_js, (), {
  try {
    var s= window.MOGAN_LORO_SERVER;
    if (!s) {
      var p= new URLSearchParams (window.location.search);
      s    = p.get ('loro_server');
    }
    s      = s || '';
    var len= lengthBytesUTF8 (s) + 1;
    var buf= _malloc (len);
    stringToUTF8 (s, buf, len);
    return buf;
  } catch (e) {
    return 0;
  }
});

EM_JS (void, collab_fetch_docs_js, (const char* url_cstr), {
  try {
    var url= UTF8ToString (url_cstr);
    fetch (url)
        .then (function (r) { return r.text (); })
        .then (function (text) {
          var len= lengthBytesUTF8 (text);
          var buf= _malloc (len + 1);
          stringToUTF8 (text, buf, len + 1);
          ccall ('mogan_collab_docs_received', null, [ 'number', 'number' ],
                 [ buf, 2 ]);
          _free (buf);
        })
        .catch (function (e) {
          ccall ('mogan_collab_docs_received', null, [ 'number', 'number' ],
                 [ 0, 3 ]);
        });
  } catch (e) {
    ccall ('mogan_collab_docs_received', null, [ 'number', 'number' ],
           [ 0, 3 ]);
  }
});

static string
ws_to_http (string url) {
  if (starts (url, "ws://")) return "http://" * url (5, N (url));
  if (starts (url, "wss://")) return "https://" * url (6, N (url));
  return url;
}

string
loro_collab_server_url () {
  string url;
  char*  buf= collab_read_server_url_js ();
  if (buf != nullptr) {
    int len= (int) strlen (buf);
    url    = string (buf, len);
    free (buf);
  }
  if (N (url) == 0) url= "ws://127.0.0.1:8765";
  return url;
}

void
loro_collab_fetch_docs (string server_url) {
  string http_url= ws_to_http (server_url);
  if (!ends (http_url, "/")) http_url= http_url * "/";
  http_url= http_url * "docs";
  if (g_docs_status == docs_status::loading) return;
  g_docs_status= docs_status::loading;
  c_string u (http_url);
  collab_fetch_docs_js ((const char*) u);
}

string
loro_collab_docs_status () {
  switch (g_docs_status) {
  case docs_status::idle:
    return "idle";
  case docs_status::loading:
    return "loading";
  case docs_status::ready:
    return "ready";
  case docs_status::error:
    return "error";
  }
  return "idle";
}

array<string>
loro_collab_docs () {
  array<string>      result;
  const std::string& body = g_docs_data;
  size_t             start= 0;
  while (start <= body.size ()) {
    size_t next= body.find ('\n', start);
    if (next == std::string::npos) next= body.size ();
    std::string line= body.substr (start, next - start);
    if (!line.empty () && line.back () == '\r') line.pop_back ();
    if (!line.empty ()) result << string (line.data (), (int) line.size ());
    start= next + 1;
  }
  return result;
}
