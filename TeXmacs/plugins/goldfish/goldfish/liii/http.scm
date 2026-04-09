;;
;; COPYRIGHT: (C) 2025  Liii Network Inc
;; All rights reverved.
;;

(define-library (liii http)
(import (liii hash-table)
        (liii alist)
        (liii error)
        (scheme file)
) ;import
(export http-head http-get http-post http-ok?
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

(define (http-require-string who field value)
  (when (not (string? value))
    (type-error (string-append who ": " field " must be string") value)
  ) ;when
  value
) ;define

(define (http-require-procedure who field value)
  (when (not (procedure? value))
    (type-error (string-append who ": " field " must be a procedure") value)
  ) ;when
  value
) ;define

(define (http-require-boolean who field value)
  (when (not (boolean? value))
    (type-error (string-append who ": " field " must be boolean") value)
  ) ;when
  value
) ;define

(define (http-optional-string who field value)
  (if value
    (http-require-string who field value)
    #f
  ) ;if
) ;define

(define (http-optional-procedure who field value)
  (if value
    (http-require-procedure who field value)
    #f
  ) ;if
) ;define

(define (http-scalar->string who field value)
  (cond ((string? value) value)
        ((symbol? value) (symbol->string value))
        ((or (integer? value) (real? value)) (number->string value))
        (else
          (type-error
            (string-append who ": " field " must be a string, symbol, or number")
            value
          ) ;type-error
        ) ;else
  ) ;cond
) ;define

(define (http-normalize-string-alist-entry who field entry)
  (when (not (pair? entry))
    (type-error (string-append who ": " field " entries must be key/value pairs") entry)
  ) ;when
  (when (pair? (cdr entry))
    (type-error (string-append who ": " field " entries must be key/value pairs") entry)
  ) ;when
  (cons (http-scalar->string who (string-append field " key") (car entry))
        (http-scalar->string who (string-append field " value") (cdr entry))
  ) ;cons
) ;define

(define (http-normalize-string-alist who field entries)
  (when (not (alist? entries))
    (type-error (string-append who ": " field " must be an association list") entries)
  ) ;when
  (map (lambda (entry)
         (http-normalize-string-alist-entry who field entry)
       ) ;lambda
       entries
  ) ;map
) ;define

(define (http-normalize-part-key who key)
  (cond ((string? key) key)
        ((symbol? key) (symbol->string key))
        (else
          (type-error
            (string-append who ": multipart part key must be string or symbol")
            key
          ) ;type-error
        ) ;else
  ) ;cond
) ;define

(define http-file-spec-keys
  '("file" "filename" "content-type")
) ;define

(define (http-part-ref part key)
  (let ((entry (assoc key part string=?)))
    (and entry (cdr entry))
  ) ;let
) ;define

(define (http-normalize-file-spec-entry who entry)
  (when (not (pair? entry))
    (type-error (string-append who ": files entries must be key/value pairs") entry)
  ) ;when
  (when (pair? (cdr entry))
    (type-error (string-append who ": files entries must be key/value pairs") entry)
  ) ;when
  (let* ((key (http-normalize-part-key who (car entry)))
         (value (cdr entry)))
    (when (not (member key http-file-spec-keys string=?))
      (value-error (string-append who ": file spec contains unsupported key") key)
    ) ;when
    (when (not (string? value))
      (type-error (string-append who ": file spec " key " must be string") value)
    ) ;when
    (cons key value)
  ) ;let*
) ;define

(define (http-normalize-file-entry who entry)
  (when (not (pair? entry))
    (type-error (string-append who ": files must be an association list") entry)
  ) ;when
  (let* ((name (http-scalar->string who "files key" (car entry)))
         (spec (cdr entry)))
    (cond ((string? spec)
           (when (not (file-exists? spec))
             (value-error (string-append who ": file does not exist") spec)
           ) ;when
           `((name . ,name)
             (file . ,spec))
          ) ;cond
          ((alist? spec)
           (let* ((normalized-spec (map (lambda (item)
                                          (http-normalize-file-spec-entry who item)
                                        ) ;lambda
                                        spec
                                   ) ;map
                  )
                  (file (http-part-ref normalized-spec "file"))
                  (filename (http-part-ref normalized-spec "filename"))
                  (content-type (http-part-ref normalized-spec "content-type")))
             (when (not file)
               (value-error (string-append who ": file spec requires a file path") spec)
             ) ;when
             (when (not (file-exists? file))
               (value-error (string-append who ": file does not exist") file)
             ) ;when
             (append `((name . ,name)
                       (file . ,file))
                     (if filename `((filename . ,filename)) '())
                     (if content-type `((content-type . ,content-type)) '())
             ) ;append
           ) ;let*
          ) ;
          (else
            (type-error
              (string-append who ": files value must be a path string or file spec alist")
              spec
            ) ;type-error
          ) ;else
    ) ;cond
  ) ;let*
) ;define

(define (http-normalize-files who files)
  (when (not (alist? files))
    (type-error (string-append who ": files must be an association list") files)
  ) ;when
  (map (lambda (entry)
         (http-normalize-file-entry who entry)
       ) ;lambda
       files
  ) ;map
) ;define

(define (http-normalize-post-form-data who data)
  (cond ((null? data) '())
        ((and (string? data) (= (string-length data) 0)) '())
        ((alist? data)
         (http-normalize-string-alist who "data" data)
        ) ;
        (else
          (type-error
            (string-append who ": data must be an association list when files is provided")
            data
          ) ;type-error
        ) ;else
  ) ;cond
) ;define

(define* (http-head url)
  (let ((r (g_http-head (http-require-string "http-head" "url" url))))
        r
  ) ;let
) ;define*

(define* (http-get url (params '()) (headers '()) (proxy '())
                   (output-file #f) (stream #f) (callback #f))
  (let* ((url (http-require-string "http-get" "url" url))
         (params (http-normalize-string-alist "http-get" "params" params))
         (headers (http-normalize-string-alist "http-get" "headers" headers))
         (proxy (http-normalize-string-alist "http-get" "proxy" proxy))
         (output-file (http-optional-string "http-get" "output-file" output-file))
         (stream (http-require-boolean "http-get" "stream" stream))
         (callback (http-optional-procedure "http-get" "callback" callback)))
    (cond ((not stream)
           (g_http-get url params headers proxy #f)
          )
          ((and (not output-file) (not callback))
           (value-error "http-get: stream mode requires output-file or callback")
          ) ;
          (else
            (let ((stream-callback
                    (lambda (chunk)
                      (if callback
                        (let ((ret (callback chunk)))
                          (if (boolean? ret) ret #t)
                        ) ;let
                        #t
                      ) ;if
                    ) ;lambda
                  ))
              (if output-file
                (let ((port (open-binary-output-file output-file)))
                  (dynamic-wind
                    (lambda () #f)
                    (lambda ()
                      (g_http-get
                        url
                        params
                        headers
                        proxy
                        (lambda (chunk)
                          (write-string chunk port)
                          (stream-callback chunk)
                        ) ;lambda
                      ) ;g_http-get
                    ) ;lambda
                    (lambda ()
                      (close-port port)
                    ) ;lambda
                  ) ;dynamic-wind
                ) ;let
                (g_http-get url params headers proxy stream-callback)
              ) ;if
            ) ;let
          ) ;else
    ) ;cond
  ) ;let*
) ;define*

(define* (http-post url (params '()) (data "") (headers '()) (proxy '()) (files '())
                    (output-file #f) (stream #f) (callback #f))
  (let* ((url (http-require-string "http-post" "url" url))
         (params (http-normalize-string-alist "http-post" "params" params))
         (headers (http-normalize-string-alist "http-post" "headers" headers))
         (proxy (http-normalize-string-alist "http-post" "proxy" proxy))
         (files (http-normalize-files "http-post" files))
         (output-file (http-optional-string "http-post" "output-file" output-file))
         (stream (http-require-boolean "http-post" "stream" stream))
         (callback (http-optional-procedure "http-post" "callback" callback)))
    (let* ((body-or-data
             (if (null? files)
               (http-require-string "http-post" "data" data)
               (http-normalize-post-form-data "http-post" data)
             ) ;if
           )
           (headers
             (if (and (null? files)
                      (> (string-length body-or-data) 0)
                      (null? headers))
               '(("Content-Type" . "text/plain"))
               headers
             ) ;if
           )
           ) ;headers
      (cond ((not stream)
             (g_http-post url params body-or-data headers proxy files #f)
            )
            ((and (not output-file) (not callback))
             (value-error "http-post: stream mode requires output-file or callback")
            ) ;
            (else
              (let ((stream-callback
                      (lambda (chunk)
                        (if callback
                          (let ((ret (callback chunk)))
                            (if (boolean? ret) ret #t)
                          ) ;let
                          #t
                        ) ;if
                      ) ;lambda
                    ))
                (if output-file
                  (let ((port (open-binary-output-file output-file)))
                    (dynamic-wind
                      (lambda () #f)
                      (lambda ()
                        (g_http-post
                          url
                          params
                          body-or-data
                          headers
                          proxy
                          files
                          (lambda (chunk)
                            (write-string chunk port)
                            (stream-callback chunk)
                          ) ;lambda
                        ) ;g_http-post
                      ) ;lambda
                      (lambda ()
                        (close-port port)
                      ) ;lambda
                    ) ;dynamic-wind
                  ) ;let
                  (g_http-post url params body-or-data headers proxy files stream-callback)
                ) ;if
              ) ;let
            ) ;else
      ) ;cond
    ) ;let*
  ) ;let*
) ;define*

;; Async HTTP API wrapper functions

(define* (http-async-get url callback (params '()) (headers '()) (proxy '()))
  (let ((url (http-require-string "http-async-get" "url" url))
        (callback (http-require-procedure "http-async-get" "callback" callback))
        (params (http-normalize-string-alist "http-async-get" "params" params))
        (headers (http-normalize-string-alist "http-async-get" "headers" headers))
        (proxy (http-normalize-string-alist "http-async-get" "proxy" proxy)))
    (g_http-async-get url params headers proxy callback)
  ) ;let
) ;define*

(define* (http-async-post url callback (params '()) (data "") (headers '()) (proxy '()))
  (let* ((url (http-require-string "http-async-post" "url" url))
         (callback (http-require-procedure "http-async-post" "callback" callback))
         (params (http-normalize-string-alist "http-async-post" "params" params))
         (data (http-require-string "http-async-post" "data" data))
         (headers (http-normalize-string-alist "http-async-post" "headers" headers))
         (proxy (http-normalize-string-alist "http-async-post" "proxy" proxy)))
    (cond ((and (> (string-length data) 0) (null? headers))
           (g_http-async-post url params data '(("Content-Type" . "text/plain")) proxy callback))
          (else (g_http-async-post url params data headers proxy callback))
    ) ;cond
  ) ;let*
) ;define*

(define* (http-async-head url callback (params '()) (headers '()) (proxy '()))
  (let ((url (http-require-string "http-async-head" "url" url))
        (callback (http-require-procedure "http-async-head" "callback" callback))
        (params (http-normalize-string-alist "http-async-head" "params" params))
        (headers (http-normalize-string-alist "http-async-head" "headers" headers))
        (proxy (http-normalize-string-alist "http-async-head" "proxy" proxy)))
    (g_http-async-head url params headers proxy callback)
  ) ;let
) ;define*

(define (http-poll)
  (g_http-poll)
) ;define

(define* (http-wait-all (timeout -1))
  (g_http-wait-all timeout)
) ;define*

) ;begin
) ;define-library
