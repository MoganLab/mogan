;;
;; COPYRIGHT: (C) 2025  Liii Network Inc
;; All rights reverved.
;;

(define-library (liii http)
(import (liii hash-table)
        (liii alist)
) ;import
(export http-head http-get http-post http-ok?
        http-stream-get http-stream-post
        http-async-get http-async-post http-async-head http-poll http-wait-all
) ;export
(begin

(define (http-ok? r)
  (let ((status-code (r 'status-code))
        (reason (r 'reason))
        (url (r 'url)))
    (cond ((and (>= status-code 400) (< status-code 500))
           (error 'http-error
             (string-append (number->string status-code)
                            " Client Error: " reason " for url: " url)
             ) ;string-append
           ) ;error
          ((and (>= status-code 500) (< status-code 600))
           (error 'http-error
             (string-append (number->string status-code)
                            " Server Error: " reason " for url: " url
             ) ;string-append
           ) ;error
          ) ;
          (else #t)
    ) ;cond
  ) ;let
) ;define

(define* (http-head url)
  (let ((r (g_http-head url)))
        r
  ) ;let
) ;define*

(define* (http-get url (params '()) (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list")
  ) ;when
  (when (not (alist? proxy))
    (type-error proxy "is not a association list")
  ) ;when
  (let ((r (g_http-get url params headers proxy)))
        r
  ) ;let
) ;define*

(define* (http-post url (params '()) (data "") (headers '()) (proxy '()))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list")
  ) ;when
  (cond ((and (string? data) (> (string-length data) 0) (null? headers))
         (g_http-post url params data '(("Content-Type" . "text/plain")) proxy))
        (else (g_http-post url params data headers proxy))
  ) ;cond
) ;define*

;; Streaming API wrapper functions

(define* (http-stream-get url callback (userdata '()) (params '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list")
  ) ;when
  (when (not (alist? proxy))
    (type-error proxy "is not a association list")
  ) ;when
  (when (not (procedure? callback))
    (type-error callback "is not a procedure")
  ) ;when
  (g_http-stream-get url params proxy userdata callback)
) ;define*

(define* (http-stream-post url callback (userdata '()) (params '()) (data "") (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list")
  ) ;when
  (when (not (alist? proxy))
    (type-error proxy "is not a association list")
  ) ;when
  (when (not (procedure? callback))
    (type-error callback "is not a procedure")
  ) ;when
  (cond ((and (string? data) (> (string-length data) 0) (null? headers))
         (g_http-stream-post url params data '(("Content-Type" . "text/plain")) proxy userdata callback))
        (else (g_http-stream-post url params data headers proxy userdata callback))
  ) ;cond
) ;define*

;; Async HTTP API wrapper functions

(define* (http-async-get url callback (params '()) (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list")
  ) ;when
  (when (not (alist? proxy))
    (type-error proxy "is not a association list")
  ) ;when
  (when (not (procedure? callback))
    (type-error callback "is not a procedure")
  ) ;when
  (g_http-async-get url params headers proxy callback)
) ;define*

(define* (http-async-post url callback (params '()) (data "") (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list")
  ) ;when
  (when (not (alist? proxy))
    (type-error proxy "is not a association list")
  ) ;when
  (when (not (procedure? callback))
    (type-error callback "is not a procedure")
  ) ;when
  (cond ((and (string? data) (> (string-length data) 0) (null? headers))
         (g_http-async-post url params data '(("Content-Type" . "text/plain")) proxy callback))
        (else (g_http-async-post url params data headers proxy callback))
  ) ;cond
) ;define*

(define* (http-async-head url callback (params '()) (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list")
  ) ;when
  (when (not (alist? proxy))
    (type-error proxy "is not a association list")
  ) ;when
  (when (not (procedure? callback))
    (type-error callback "is not a procedure")
  ) ;when
  (g_http-async-head url params headers proxy callback)
) ;define*

(define (http-poll)
  (g_http-poll)
) ;define

(define* (http-wait-all (timeout -1))
  (g_http-wait-all timeout)
) ;define*

) ;begin
) ;define-library

