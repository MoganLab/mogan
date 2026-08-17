;;
;; Copyright (C) 2025 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;; http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;

(define-library (liii http-async)
  (import (liii http-common))
  (export http-async-get http-async-post http-async-head http-poll http-wait-all
    http-ok?
  ) ;export
  (begin

    (define* (http-async-get url callback (params '()) (headers '()) (proxy '()))
      (let ((url (http-require-string "http-async-get" "url" url))
            (callback (http-require-procedure "http-async-get" "callback" callback))
            (params (http-normalize-string-alist "http-async-get" "params" params))
            (headers (http-normalize-string-alist "http-async-get" "headers" headers))
            (proxy (http-normalize-string-alist "http-async-get" "proxy" proxy))
           ) ;
        (g_http-async-get url params headers proxy callback)
      ) ;let
    ) ;define*

    (define* (http-async-post url callback (params '()) (data "") (headers '()) (proxy '()))
      (let* ((url (http-require-string "http-async-post" "url" url))
             (callback (http-require-procedure "http-async-post" "callback" callback))
             (params (http-normalize-string-alist "http-async-post" "params" params))
             (data (http-require-string "http-async-post" "data" data))
             (headers (http-normalize-string-alist "http-async-post" "headers" headers))
             (proxy (http-normalize-string-alist "http-async-post" "proxy" proxy))
            ) ;
        (cond ((and (> (string-length data) 0) (null? headers))
               (g_http-async-post url
                 params
                 data
                 '(("Content-Type" . "text/plain"))
                 proxy
                 callback
               ) ;g_http-async-post
              ) ;
              (else (g_http-async-post url params data headers proxy callback))
        ) ;cond
      ) ;let*
    ) ;define*

    (define* (http-async-head url callback (params '()) (headers '()) (proxy '()))
      (let ((url (http-require-string "http-async-head" "url" url))
            (callback (http-require-procedure "http-async-head" "callback" callback))
            (params (http-normalize-string-alist "http-async-head" "params" params))
            (headers (http-normalize-string-alist "http-async-head" "headers" headers))
            (proxy (http-normalize-string-alist "http-async-head" "proxy" proxy))
           ) ;
        (g_http-async-head url params headers proxy callback)
      ) ;let
    ) ;define*

    (define (http-poll)
      (g_http-poll)
    ) ;define

    (define* (http-wait-all (timeout -1)) (g_http-wait-all timeout))

  ) ;begin
) ;define-library
