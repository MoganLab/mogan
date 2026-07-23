/******************************************************************************
 * MODULE     : loro_collab_docs_native.cpp
 * AUTHOR     : Jim Zhou
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes with NO WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "analyze.hpp"
#include "loro_collab.hpp"
#include "url.hpp"

#include <atomic>
#include <cstdlib>
#include <curl/curl.h>
#include <mutex>
#include <thread>
#include <vector>

enum class docs_status { idle, loading, ready, error };

static std::mutex               g_docs_mutex;
static std::atomic<docs_status> g_docs_status{docs_status::idle};
static std::vector<std::string> g_docs_data;

static size_t
http_write_cb (char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* buf= static_cast<std::string*> (userdata);
  buf->append (ptr, size * nmemb);
  return size * nmemb;
}

static string
ws_to_http (string url) {
  if (starts (url, "ws://")) return "http://" * url (5, N (url));
  if (starts (url, "wss://")) return "https://" * url (6, N (url));
  return url;
}

string
loro_collab_server_url () {
  string      url;
  const char* e= getenv ("MOGAN_LORO_SERVER");
  if (e != nullptr) url= string (e);
  if (N (url) == 0) url= "ws://127.0.0.1:8765";
  return url;
}

void
loro_collab_fetch_docs (string server_url) {
  string http_url= ws_to_http (server_url);
  if (!ends (http_url, "/")) http_url= http_url * "/";
  http_url= http_url * "docs";
  if (g_docs_status.load () == docs_status::loading) return;
  g_docs_status.store (docs_status::loading);

  std::string url_std ((const char*) c_string (http_url));
  std::thread ([url_std] () {
    std::string body;
    CURL*       h= curl_easy_init ();
    if (h == nullptr) {
      std::lock_guard<std::mutex> lk (g_docs_mutex);
      g_docs_data.clear ();
      g_docs_status.store (docs_status::error);
      return;
    }
    curl_easy_setopt (h, CURLOPT_URL, url_std.c_str ());
    curl_easy_setopt (h, CURLOPT_WRITEFUNCTION, http_write_cb);
    curl_easy_setopt (h, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt (h, CURLOPT_CONNECTTIMEOUT, 2L);
    curl_easy_setopt (h, CURLOPT_TIMEOUT, 3L);
    curl_easy_setopt (h, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt (h, CURLOPT_SSL_VERIFYHOST, 0L);
    CURLcode rc= curl_easy_perform (h);
    curl_easy_cleanup (h);
    std::vector<std::string> lines;
    if (rc != CURLE_OK) {
      std::lock_guard<std::mutex> lk (g_docs_mutex);
      g_docs_data.clear ();
      g_docs_status.store (docs_status::error);
      return;
    }
    size_t start= 0;
    while (start <= body.size ()) {
      size_t next= body.find ('\n', start);
      if (next == std::string::npos) next= body.size ();
      std::string line= body.substr (start, next - start);
      if (!line.empty () && line.back () == '\r') line.pop_back ();
      if (!line.empty ()) lines.push_back (line);
      start= next + 1;
    }
    std::lock_guard<std::mutex> lk (g_docs_mutex);
    g_docs_data= std::move (lines);
    g_docs_status.store (docs_status::ready);
  }).detach ();
}

string
loro_collab_docs_status () {
  switch (g_docs_status.load ()) {
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
  array<string>               result;
  std::lock_guard<std::mutex> lk (g_docs_mutex);
  for (const auto& s : g_docs_data) {
    string line ((const char*) s.data (), (int) s.size ());
    result << line;
  }
  return result;
}
