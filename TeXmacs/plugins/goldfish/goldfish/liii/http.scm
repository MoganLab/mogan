;;
;; COPYRIGHT: (C) 2025  Liii Network Inc
;; All rights reverved.
;;

(define-library (liii http)
(import (liii hash-table)
        (liii alist))
(export http-head http-get http-post http-ok?
        http-stream-get http-stream-post
        http-async-get http-async-post http-async-head http-poll http-wait-all)
(begin

(define (http-ok? r)
  (let ((status-code (r 'status-code))
        (reason (r 'reason))
        (url (r 'url)))
    (cond ((and (>= status-code 400) (< status-code 500))
           (error 'http-error
             (string-append (number->string status-code)
                            " Client Error: " reason " for url: " url)))
          ((and (>= status-code 500) (< status-code 600))
           (error 'http-error
             (string-append (number->string status-code)
                            " Server Error: " reason " for url: " url)))
          (else #t))))

(define* (http-head url)
  (let1 r (g_http-head url)
        r))

(define* (http-get url (params '()) (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list"))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list"))
  (let1 r (g_http-get url params headers proxy)
        r))

(define* (http-post url (params '()) (data "") (headers '()) (proxy '()))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list"))
  (cond ((and (string? data) (> (string-length data) 0) (null? headers))
         (g_http-post url params data '(("Content-Type" . "text/plain")) proxy))
        (else (g_http-post url params data headers proxy))))

;; Streaming API wrapper functions

(define* (http-stream-get url callback (userdata '()) (params '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list"))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list"))
  (when (not (procedure? callback))
    (type-error callback "is not a procedure"))
  (g_http-stream-get url params proxy userdata callback))

(define* (http-stream-post url callback (userdata '()) (params '()) (data "") (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list"))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list"))
  (when (not (procedure? callback))
    (type-error callback "is not a procedure"))
  (cond ((and (string? data) (> (string-length data) 0) (null? headers))
         (g_http-stream-post url params data '(("Content-Type" . "text/plain")) proxy userdata callback))
        (else (g_http-stream-post url params data headers proxy userdata callback))))

;; Async HTTP API wrapper functions

(define* (http-async-get url callback (params '()) (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list"))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list"))
  (when (not (procedure? callback))
    (type-error callback "is not a procedure"))
  (g_http-async-get url params headers proxy callback))

(define* (http-async-post url callback (params '()) (data "") (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list"))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list"))
  (when (not (procedure? callback))
    (type-error callback "is not a procedure"))
  (cond ((and (string? data) (> (string-length data) 0) (null? headers))
         (g_http-async-post url params data '(("Content-Type" . "text/plain")) proxy callback))
        (else (g_http-async-post url params data headers proxy callback))))

(define* (http-async-head url callback (params '()) (headers '()) (proxy '()))
  (when (not (alist? params))
    (type-error params "is not a association list"))
  (when (not (alist? proxy))
    (type-error proxy "is not a association list"))
  (when (not (procedure? callback))
    (type-error callback "is not a procedure"))
  (g_http-async-head url params headers proxy callback))

(define (http-poll)
  (g_http-poll))

(define* (http-wait-all (timeout -1))
  (g_http-wait-all timeout))

) ; end of begin
) ; end of define-library

