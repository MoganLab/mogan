//
// Copyright (C) 2024 The Goldfish Scheme Authors
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
#include <algorithm>
#include <chrono>
#include <cpr/cpr.h>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace goldfish {
using std::string;
using std::vector;

static s7_pointer
error2hashtable (s7_scheme* sc, long status_code, const std::string& url, const std::string& reason) {
  s7_pointer ht= s7_make_hash_table (sc, 4);
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "status-code"), s7_make_integer (sc, status_code));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "url"), s7_make_string (sc, url.c_str ()));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "text"), s7_make_string (sc, ""));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "reason"), s7_make_string (sc, reason.c_str ()));
  return ht;
}

static s7_pointer
response2hashtable (s7_scheme* sc, cpr::Response r) {
  s7_pointer ht= s7_make_hash_table (sc, 8);
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "status-code"), s7_make_integer (sc, r.status_code));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "url"), s7_make_string (sc, r.url.c_str ()));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "elapsed"), s7_make_real (sc, r.elapsed));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "text"), s7_make_string (sc, r.text.c_str ()));
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "reason"), s7_make_string (sc, r.reason.c_str ()));
  s7_pointer headers= s7_make_hash_table (sc, r.header.size ());
  for (const auto& header : r.header) {
    const auto  key      = header.first.c_str ();
    std::string key_lower= header.first;
    std::transform (key_lower.begin (), key_lower.end (), key_lower.begin (), ::tolower);
    const auto value= header.second.c_str ();
    s7_hash_table_set (sc, headers, s7_make_string (sc, key_lower.c_str ()), s7_make_string (sc, value));
  }
  s7_hash_table_set (sc, ht, s7_make_symbol (sc, "headers"), headers);

  return ht;
}

inline cpr::Parameters
to_cpr_parameters (s7_scheme* sc, s7_pointer args) {
  cpr::Parameters params= cpr::Parameters{};
  s7_pointer      iter  = args;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair= s7_car (iter);
    params.Add (cpr::Parameter (s7_string (s7_car (pair)), s7_string (s7_cdr (pair))));
    iter= s7_cdr (iter);
  }
  return params;
}

inline cpr::Header
to_cpr_headers (s7_scheme* sc, s7_pointer args) {
  cpr::Header headers= cpr::Header{};
  s7_pointer  iter   = args;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair= s7_car (iter);
    headers.insert ({s7_string (s7_car (pair)), s7_string (s7_cdr (pair))});
    iter= s7_cdr (iter);
  }
  return headers;
}

inline cpr::Proxies
to_cpr_proxies (s7_scheme* sc, s7_pointer args) {
  std::map<std::string, std::string> proxy_map;
  s7_pointer                         iter= args;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair                     = s7_car (iter);
    proxy_map[s7_string (s7_car (pair))]= s7_string (s7_cdr (pair));
    iter                                = s7_cdr (iter);
  }
  return cpr::Proxies (proxy_map);
}

static cpr::Part
to_cpr_multipart_part (s7_scheme* sc, s7_pointer part_spec) {
  std::string name;
  std::string value;
  std::string file_path;
  std::string filename;
  std::string content_type;
  bool        has_file= false;

  s7_pointer iter= part_spec;
  while (!s7_is_null (sc, iter)) {
    s7_pointer  entry    = s7_car (iter);
    s7_pointer  raw_key  = s7_car (entry);
    const char* key      = s7_is_symbol (raw_key) ? s7_symbol_name (raw_key) : s7_string (raw_key);
    const char* raw_value= s7_string (s7_cdr (entry));

    if (strcmp (key, "name") == 0) {
      name= raw_value;
    }
    else if (strcmp (key, "value") == 0) {
      value= raw_value;
    }
    else if (strcmp (key, "file") == 0) {
      file_path= raw_value;
      has_file = true;
    }
    else if (strcmp (key, "filename") == 0) {
      filename= raw_value;
    }
    else if (strcmp (key, "content-type") == 0) {
      content_type= raw_value;
    }

    iter= s7_cdr (iter);
  }

  if (has_file) {
    cpr::Files files;
    if (filename.empty ()) {
      files.push_back (cpr::File (file_path));
    }
    else {
      files.push_back (cpr::File (file_path, filename));
    }
    return cpr::Part (name, files, content_type);
  }

  return cpr::Part (name, value, content_type);
}

static void
append_cpr_multipart_file_parts (s7_scheme* sc, s7_pointer files, std::vector<cpr::Part>& parts) {
  s7_pointer iter= files;
  while (!s7_is_null (sc, iter)) {
    parts.push_back (to_cpr_multipart_part (sc, s7_car (iter)));
    iter= s7_cdr (iter);
  }
}

static void
append_cpr_multipart_form_parts (s7_scheme* sc, s7_pointer data, std::vector<cpr::Part>& parts) {
  s7_pointer iter= data;
  while (!s7_is_null (sc, iter)) {
    s7_pointer pair= s7_car (iter);
    parts.push_back (cpr::Part (s7_string (s7_car (pair)), s7_string (s7_cdr (pair))));
    iter= s7_cdr (iter);
  }
}

static cpr::Multipart
to_cpr_post_multipart (s7_scheme* sc, s7_pointer data, s7_pointer files) {
  std::vector<cpr::Part> parts;
  append_cpr_multipart_form_parts (sc, data, parts);
  append_cpr_multipart_file_parts (sc, files, parts);
  return cpr::Multipart (parts);
}

static s7_pointer
f_http_head (s7_scheme* sc, s7_pointer args) {
  const char*  url= s7_string (s7_car (args));
  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  cpr::Response r= session.Head ();
  return response2hashtable (sc, r);
}

inline void
glue_http_head (s7_scheme* sc) {
  s7_pointer  cur_env       = s7_curlet (sc);
  const char* s_http_head   = "g_http-head";
  const char* d_http_head   = "(g_http-head url ...) => hash-table?";
  auto        func_http_head= s7_make_typed_function (sc, s_http_head, f_http_head, 1, 0, false, d_http_head, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, s_http_head), func_http_head);
}

static s7_pointer
f_http_get (s7_scheme* sc, s7_pointer args) {
  const char*     url        = s7_string (s7_car (args));
  s7_pointer      params     = s7_cadr (args);
  s7_pointer      headers    = s7_caddr (args);
  s7_pointer      proxy      = s7_cadddr (args);
  s7_pointer      callback   = s7_car (s7_cddddr (args));
  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header     cpr_headers= to_cpr_headers (sc, headers);
  cpr::Proxies    cpr_proxies= to_cpr_proxies (sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  session.SetHeader (cpr_headers);
  if (s7_is_list (sc, proxy) && !s7_is_null (sc, proxy)) {
    session.SetProxies (cpr_proxies);
  }

  if (s7_is_procedure (callback)) {
    session.SetWriteCallback (cpr::WriteCallback{[sc, callback] (const std::string_view& data, intptr_t) -> bool {
      s7_pointer data_str = s7_make_string_with_length (sc, data.data (), data.length ());
      s7_pointer call_args= s7_cons (sc, data_str, s7_nil (sc));

      s7_pointer ret= s7_call (sc, callback, call_args);
      if (s7_is_boolean (ret)) {
        return s7_boolean (sc, ret);
      }

      return true;
    }});

    try {
      cpr::Response response= session.Get ();
      return response2hashtable (sc, response);
    } catch (const std::exception& e) {
      return error2hashtable (sc, 0, url, e.what ());
    }
  }

  cpr::Response r= session.Get ();
  return response2hashtable (sc, r);
}

inline void
glue_http_get (s7_scheme* sc) {
  s7_pointer  cur_env      = s7_curlet (sc);
  const char* s_http_get   = "g_http-get";
  const char* d_http_get   = "(g_http-get url params headers proxy callback) => hash-table? | undefined";
  auto        func_http_get= s7_make_typed_function (sc, s_http_get, f_http_get, 5, 0, false, d_http_get, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, s_http_get), func_http_get);
}

static s7_pointer
f_http_post (s7_scheme* sc, s7_pointer args) {
  const char*     url         = s7_string (s7_car (args));
  s7_pointer      params      = s7_cadr (args);
  s7_pointer      body_or_data= s7_caddr (args);
  s7_pointer      headers     = s7_cadddr (args);
  s7_pointer      proxy       = s7_car (s7_cddddr (args));
  s7_pointer      files       = s7_cadr (s7_cddddr (args));
  s7_pointer      callback    = s7_list_ref (sc, args, 6);
  cpr::Parameters cpr_params  = to_cpr_parameters (sc, params);
  cpr::Header     cpr_headers = to_cpr_headers (sc, headers);
  cpr::Proxies    cpr_proxies = to_cpr_proxies (sc, proxy);

  cpr::Session session;
  session.SetUrl (cpr::Url (url));
  session.SetParameters (cpr_params);
  session.SetHeader (cpr_headers);
  if (s7_is_list (sc, proxy) && !s7_is_null (sc, proxy)) {
    session.SetProxies (cpr_proxies);
  }

  if (s7_is_list (sc, files) && !s7_is_null (sc, files)) {
    session.SetMultipart (to_cpr_post_multipart (sc, body_or_data, files));
  }
  else {
    const char* body    = s7_string (body_or_data);
    cpr::Body   cpr_body= cpr::Body (body);
    session.SetBody (cpr_body);
  }

  if (s7_is_procedure (callback)) {
    session.SetWriteCallback (cpr::WriteCallback{[sc, callback] (const std::string_view& data, intptr_t) -> bool {
      s7_pointer data_str = s7_make_string_with_length (sc, data.data (), data.length ());
      s7_pointer call_args= s7_cons (sc, data_str, s7_nil (sc));

      s7_pointer ret= s7_call (sc, callback, call_args);
      if (s7_is_boolean (ret)) {
        return s7_boolean (sc, ret);
      }

      return true;
    }});

    try {
      cpr::Response response= session.Post ();
      return response2hashtable (sc, response);
    } catch (const std::exception& e) {
      return error2hashtable (sc, 0, url, e.what ());
    }
  }

  cpr::Response r= session.Post ();
  return response2hashtable (sc, r);
}

inline void
glue_http_post (s7_scheme* sc) {
  s7_pointer  cur_env= s7_curlet (sc);
  const char* name   = "g_http-post";
  const char* doc    = "(g_http-post url params body-or-data headers proxy files callback) => hash-table? | undefined";
  auto        func_http_post= s7_make_typed_function (sc, name, f_http_post, 7, 0, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func_http_post);
}

void
glue_http (s7_scheme* sc) {
  glue_http_head (sc);
  glue_http_get (sc);
  glue_http_post (sc);
}

// -------------------------------- Async HTTP --------------------------------
// Data structure to store async HTTP request state
struct AsyncHttpRequest {
  s7_scheme*                    sc;
  s7_pointer                    callback;
  int                           gc_loc;
  std::shared_ptr<cpr::Session> session; // Keep session alive
  cpr::AsyncResponse            async_response;
  bool                          completed;
  cpr::Response                 response;
  std::mutex                    mutex;

  AsyncHttpRequest (s7_scheme* scheme, s7_pointer cb, int gc_protect_loc, std::shared_ptr<cpr::Session> sess,
                    cpr::AsyncResponse&& ar)
      : sc (scheme), callback (cb), gc_loc (gc_protect_loc), session (std::move (sess)),
        async_response (std::move (ar)), completed (false) {}
};

// Global list of pending async requests
static std::mutex                                     g_async_requests_mutex;
static std::vector<std::shared_ptr<AsyncHttpRequest>> g_async_requests;

// Check if any async requests have completed and process their callbacks
// This function should be called periodically from the main thread
// Returns the number of callbacks executed
static int
process_async_http_callbacks () {
  std::vector<std::shared_ptr<AsyncHttpRequest>> completed_requests;

  // Find completed requests
  {
    std::lock_guard<std::mutex> lock (g_async_requests_mutex);
    for (auto it= g_async_requests.begin (); it != g_async_requests.end ();) {
      bool is_ready= false;
      {
        std::lock_guard<std::mutex> req_lock ((*it)->mutex);
        if (!(*it)->completed) {
          // Check if the future is ready (non-blocking)
          if ((*it)->async_response.wait_for (std::chrono::seconds (0)) == std::future_status::ready) {
            (*it)->response = (*it)->async_response.get ();
            (*it)->completed= true;
            is_ready        = true;
          }
        }
      }

      if (is_ready) {
        completed_requests.push_back (*it);
        it= g_async_requests.erase (it);
      }
      else {
        ++it;
      }
    }
  }

  // Execute callbacks for completed requests (outside the lock)
  for (auto& req : completed_requests) {
    s7_pointer ht= response2hashtable (req->sc, req->response);
    s7_call (req->sc, req->callback, s7_cons (req->sc, ht, s7_nil (req->sc)));
    s7_gc_unprotect_at (req->sc, req->gc_loc);
  }

  return static_cast<int> (completed_requests.size ());
}

// Start an async HTTP GET request
static s7_pointer
f_http_async_get (s7_scheme* sc, s7_pointer args) {
  const char* url     = s7_string (s7_car (args));
  s7_pointer  params  = s7_cadr (args);
  s7_pointer  headers = s7_caddr (args);
  s7_pointer  proxy   = s7_cadddr (args);
  s7_pointer  callback= s7_car (s7_cddddr (args));

  if (!s7_is_procedure (callback)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 2, s7_make_string (sc, "http-async-get: callback must be a procedure"), callback));
  }

  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header     cpr_headers= to_cpr_headers (sc, headers);
  cpr::Proxies    cpr_proxies= to_cpr_proxies (sc, proxy);

  // Protect callback from GC
  int gc_loc= s7_gc_protect (sc, callback);

  // Create session on heap with shared_ptr to keep it alive
  auto session= std::make_shared<cpr::Session> ();
  session->SetUrl (cpr::Url (url));
  session->SetParameters (cpr_params);
  session->SetHeader (cpr_headers);
  if (s7_is_list (sc, proxy) && !s7_is_null (sc, proxy)) {
    session->SetProxies (cpr_proxies);
  }

  // Start async request using libcpr's built-in thread pool
  // Session is captured by shared_ptr, so it stays alive until async operation completes
  auto async_resp= session->GetAsync ();

  // Store the request (session is also stored to keep reference)
  auto req= std::make_shared<AsyncHttpRequest> (sc, callback, gc_loc, session, std::move (async_resp));
  {
    std::lock_guard<std::mutex> lock (g_async_requests_mutex);
    g_async_requests.push_back (req);
  }

  return s7_make_boolean (sc, true);
}

inline void
glue_http_async_get (s7_scheme* sc) {
  s7_pointer  cur_env= s7_curlet (sc);
  const char* name   = "g_http-async-get";
  const char* doc = "(g_http-async-get url params headers proxy callback) => boolean, start async http get. callback "
                    "receives response hashtable. Use g_http-poll to check for completion.";
  auto        func= s7_make_typed_function (sc, name, f_http_async_get, 5, 0, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

// Start an async HTTP POST request
static s7_pointer
f_http_async_post (s7_scheme* sc, s7_pointer args) {
  const char* url     = s7_string (s7_car (args));
  s7_pointer  params  = s7_cadr (args);
  const char* body    = s7_string (s7_caddr (args));
  s7_pointer  headers = s7_cadddr (args);
  s7_pointer  proxy   = s7_car (s7_cddddr (args));
  s7_pointer  callback= s7_cadr (s7_cddddr (args));

  if (!s7_is_procedure (callback)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 2, s7_make_string (sc, "http-async-post: callback must be a procedure"), callback));
  }

  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header     cpr_headers= to_cpr_headers (sc, headers);
  cpr::Proxies    cpr_proxies= to_cpr_proxies (sc, proxy);

  // Protect callback from GC
  int gc_loc= s7_gc_protect (sc, callback);

  // Create session on heap with shared_ptr to keep it alive
  auto session= std::make_shared<cpr::Session> ();
  session->SetUrl (cpr::Url (url));
  session->SetParameters (cpr_params);
  session->SetBody (cpr::Body (body));
  session->SetHeader (cpr_headers);
  if (s7_is_list (sc, proxy) && !s7_is_null (sc, proxy)) {
    session->SetProxies (cpr_proxies);
  }

  // Start async request using libcpr's built-in thread pool
  auto async_resp= session->PostAsync ();

  // Store the request (session is also stored to keep reference)
  auto req= std::make_shared<AsyncHttpRequest> (sc, callback, gc_loc, session, std::move (async_resp));
  {
    std::lock_guard<std::mutex> lock (g_async_requests_mutex);
    g_async_requests.push_back (req);
  }

  return s7_make_boolean (sc, true);
}

inline void
glue_http_async_post (s7_scheme* sc) {
  s7_pointer  cur_env= s7_curlet (sc);
  const char* name   = "g_http-async-post";
  const char* doc    = "(g_http-async-post url params body headers proxy callback) => boolean, start async http post. "
                       "callback receives response hashtable. Use g_http-poll to check for completion.";
  auto        func   = s7_make_typed_function (sc, name, f_http_async_post, 6, 0, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

// Start an async HTTP HEAD request
static s7_pointer
f_http_async_head (s7_scheme* sc, s7_pointer args) {
  const char* url     = s7_string (s7_car (args));
  s7_pointer  params  = s7_cadr (args);
  s7_pointer  headers = s7_caddr (args);
  s7_pointer  proxy   = s7_cadddr (args);
  s7_pointer  callback= s7_car (s7_cddddr (args));

  if (!s7_is_procedure (callback)) {
    return s7_error (sc, s7_make_symbol (sc, "type-error"),
                     s7_list (sc, 2, s7_make_string (sc, "http-async-head: callback must be a procedure"), callback));
  }

  cpr::Parameters cpr_params = to_cpr_parameters (sc, params);
  cpr::Header     cpr_headers= to_cpr_headers (sc, headers);
  cpr::Proxies    cpr_proxies= to_cpr_proxies (sc, proxy);

  // Protect callback from GC
  int gc_loc= s7_gc_protect (sc, callback);

  // Create session on heap with shared_ptr to keep it alive
  auto session= std::make_shared<cpr::Session> ();
  session->SetUrl (cpr::Url (url));
  session->SetParameters (cpr_params);
  session->SetHeader (cpr_headers);
  if (s7_is_list (sc, proxy) && !s7_is_null (sc, proxy)) {
    session->SetProxies (cpr_proxies);
  }

  // Start async request using libcpr's built-in thread pool
  auto async_resp= session->HeadAsync ();

  // Store the request (session is also stored to keep reference)
  auto req= std::make_shared<AsyncHttpRequest> (sc, callback, gc_loc, session, std::move (async_resp));
  {
    std::lock_guard<std::mutex> lock (g_async_requests_mutex);
    g_async_requests.push_back (req);
  }

  return s7_make_boolean (sc, true);
}

inline void
glue_http_async_head (s7_scheme* sc) {
  s7_pointer  cur_env= s7_curlet (sc);
  const char* name   = "g_http-async-head";
  const char* doc = "(g_http-async-head url params headers proxy callback) => boolean, start async http head. callback "
                    "receives response hashtable. Use g_http-poll to check for completion.";
  auto        func= s7_make_typed_function (sc, name, f_http_async_head, 5, 0, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

// Poll for completed async HTTP requests and execute their callbacks
static s7_pointer
f_http_poll (s7_scheme* sc, s7_pointer args) {
  int executed= process_async_http_callbacks ();
  return s7_make_integer (sc, executed);
}

inline void
glue_http_poll (s7_scheme* sc) {
  s7_pointer  cur_env= s7_curlet (sc);
  const char* name   = "g_http-poll";
  const char* doc    = "(g_http-poll) => integer, check for completed async http requests and execute their callbacks. "
                       "Returns number of callbacks executed.";
  auto        func   = s7_make_typed_function (sc, name, f_http_poll, 0, 0, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

// Wait for all pending async HTTP requests to complete (blocking)
static s7_pointer
f_http_wait_all (s7_scheme* sc, s7_pointer args) {
  s7_double timeout_sec= -1.0; // -1 means wait forever
  if (s7_is_real (s7_car (args))) {
    timeout_sec= s7_real (s7_car (args));
  }

  auto start         = std::chrono::steady_clock::now ();
  bool has_pending   = true;
  int  total_executed= 0;

  while (has_pending) {
    int executed= process_async_http_callbacks ();
    total_executed+= executed;

    // Check if there are still pending requests
    {
      std::lock_guard<std::mutex> lock (g_async_requests_mutex);
      has_pending= !g_async_requests.empty ();
    }

    if (has_pending) {
      // Check timeout
      if (timeout_sec >= 0) {
        auto elapsed=
            std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::steady_clock::now () - start).count () /
            1000.0;
        if (elapsed >= timeout_sec) {
          break; // Timeout
        }
      }
      // Small sleep to avoid busy waiting
      std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
  }

  return s7_make_integer (sc, total_executed);
}

inline void
glue_http_wait_all (s7_scheme* sc) {
  s7_pointer  cur_env= s7_curlet (sc);
  const char* name   = "g_http-wait-all";
  const char* doc    = "(g_http-wait-all [timeout-seconds]) => integer, wait for all pending async http requests to "
                       "complete. timeout < 0 means wait forever. Returns number of callbacks executed.";
  auto        func   = s7_make_typed_function (sc, name, f_http_wait_all, 0, 1, false, doc, NULL);
  s7_define (sc, cur_env, s7_make_symbol (sc, name), func);
}

void
glue_http_async (s7_scheme* sc) {
  glue_http_async_get (sc);
  glue_http_async_post (sc);
  glue_http_async_head (sc);
  glue_http_poll (sc);
  glue_http_wait_all (sc);
}

} // namespace goldfish
