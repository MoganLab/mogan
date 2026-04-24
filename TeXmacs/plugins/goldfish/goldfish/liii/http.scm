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
  (export http-head
    http-get
    http-post
    http-ok?
    http-async-get
    http-async-post
    http-async-head
    http-poll
    http-wait-all
  ) ;export
  (begin

    (define (http-ok? r)
      (let ((status-code (r 'status-code))
            (reason (r 'reason))
            (url (r 'url))
           ) ;
        (cond ((and (>= status-code 400)
                 (< status-code 500)
               ) ;and
               (error 'http-error
                 (string-append (number->string status-code)
                   " Client Error: "
                   reason
                   " for url: "
                   url
                 ) ;string-append
               ) ;error
              ) ;
              ((and (>= status-code 500)
                 (< status-code 600)
               ) ;and
               (error 'http-error
                 (string-append (number->string status-code)
                   " Server Error: "
                   reason
                   " for url: "
                   url
                 ) ;string-append
               ) ;error
              ) ;
              (else #t)
        ) ;cond
      ) ;let
    ) ;define

    (define (http-require-string who field value)
      (when (not (string? value))
        (type-error (string-append who
                      ": "
                      field
                      " must be string"
                    ) ;string-append
          value
        ) ;type-error
      ) ;when
      value
    ) ;define

    (define (http-require-procedure who field value)
      (when (not (procedure? value))
        (type-error (string-append who
                      ": "
                      field
                      " must be a procedure"
                    ) ;string-append
          value
        ) ;type-error
      ) ;when
      value
    ) ;define

    (define (http-require-boolean who field value)
      (when (not (boolean? value))
        (type-error (string-append who
                      ": "
                      field
                      " must be boolean"
                    ) ;string-append
          value
        ) ;type-error
      ) ;when
      value
    ) ;define

    (define (http-optional-string who field value)
      (if value
        (http-require-string who field value)
        #f
      ) ;if
    ) ;define

    (define (http-optional-procedure who
              field
              value
            ) ;http-optional-procedure
      (if value
        (http-require-procedure who field value)
        #f
      ) ;if
    ) ;define

    (define (http-scalar->string who field value)
      (cond ((string? value) value)
            ((symbol? value) (symbol->string value))
            ((or (integer? value) (real? value))
             (number->string value)
            ) ;
            (else (type-error (string-append who
                                ": "
                                field
                                " must be a string, symbol, or number"
                              ) ;string-append
                    value
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define (http-normalize-string-alist-entry who
              field
              entry
            ) ;http-normalize-string-alist-entry
      (when (not (pair? entry))
        (type-error (string-append who
                      ": "
                      field
                      " entries must be key/value pairs"
                    ) ;string-append
          entry
        ) ;type-error
      ) ;when
      (when (pair? (cdr entry))
        (type-error (string-append who
                      ": "
                      field
                      " entries must be key/value pairs"
                    ) ;string-append
          entry
        ) ;type-error
      ) ;when
      (cons (http-scalar->string who
              (string-append field " key")
              (car entry)
            ) ;http-scalar->string
        (http-scalar->string who
          (string-append field " value")
          (cdr entry)
        ) ;http-scalar->string
      ) ;cons
    ) ;define

    (define (http-normalize-string-alist who
              field
              entries
            ) ;http-normalize-string-alist
      (when (not (alist? entries))
        (type-error (string-append who
                      ": "
                      field
                      " must be an association list"
                    ) ;string-append
          entries
        ) ;type-error
      ) ;when
      (map (lambda (entry)
             (http-normalize-string-alist-entry who
               field
               entry
             ) ;http-normalize-string-alist-entry
           ) ;lambda
        entries
      ) ;map
    ) ;define

    (define (http-normalize-part-key who key)
      (cond ((string? key) key)
            ((symbol? key) (symbol->string key))
            (else (type-error (string-append who
                                ": multipart part key must be string or symbol"
                              ) ;string-append
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

    (define (http-normalize-file-spec-entry who
              entry
            ) ;http-normalize-file-spec-entry
      (when (not (pair? entry))
        (type-error (string-append who
                      ": files entries must be key/value pairs"
                    ) ;string-append
          entry
        ) ;type-error
      ) ;when
      (when (pair? (cdr entry))
        (type-error (string-append who
                      ": files entries must be key/value pairs"
                    ) ;string-append
          entry
        ) ;type-error
      ) ;when
      (let* ((key (http-normalize-part-key who
                    (car entry)
                  ) ;http-normalize-part-key
             ) ;key
             (value (cdr entry))
            ) ;
        (when (not (member key
                     http-file-spec-keys
                     string=?
                   ) ;member
              ) ;not
          (value-error (string-append who
                         ": file spec contains unsupported key"
                       ) ;string-append
            key
          ) ;value-error
        ) ;when
        (when (not (string? value))
          (type-error (string-append who
                        ": file spec "
                        key
                        " must be string"
                      ) ;string-append
            value
          ) ;type-error
        ) ;when
        (cons key value)
      ) ;let*
    ) ;define

    (define (http-normalize-file-entry who entry)
      (when (not (pair? entry))
        (type-error (string-append who
                      ": files must be an association list"
                    ) ;string-append
          entry
        ) ;type-error
      ) ;when
      (let* ((name (http-scalar->string who
                     "files key"
                     (car entry)
                   ) ;http-scalar->string
             ) ;name
             (spec (cdr entry))
            ) ;
        (cond ((string? spec)
               (when (not (file-exists? spec))
                 (value-error (string-append who
                                ": file does not exist"
                              ) ;string-append
                   spec
                 ) ;value-error
               ) ;when
               `((name unquote name) (file unquote spec))
              ) ;
              ((alist? spec)
               (let* ((normalized-spec (map (lambda (item)
                                              (http-normalize-file-spec-entry who
                                                item
                                              ) ;http-normalize-file-spec-entry
                                            ) ;lambda
                                         spec
                                       ) ;map
                      ) ;normalized-spec
                      (file (http-part-ref normalized-spec "file")
                      ) ;file
                      (filename (http-part-ref normalized-spec
                                  "filename"
                                ) ;http-part-ref
                      ) ;filename
                      (content-type (http-part-ref normalized-spec
                                      "content-type"
                                    ) ;http-part-ref
                      ) ;content-type
                     ) ;
                 (when (not file)
                   (value-error (string-append who
                                  ": file spec requires a file path"
                                ) ;string-append
                     spec
                   ) ;value-error
                 ) ;when
                 (when (not (file-exists? file))
                   (value-error (string-append who
                                  ": file does not exist"
                                ) ;string-append
                     file
                   ) ;value-error
                 ) ;when
                 (append `((name unquote name) (file unquote file))
                   (if filename
                     `((filename unquote filename))
                     '()
                   ) ;if
                   (if content-type
                     `((content-type unquote content-type))
                     '()
                   ) ;if
                 ) ;append
               ) ;let*
              ) ;
              (else (type-error (string-append who
                                  ": files value must be a path string or file spec alist"
                                ) ;string-append
                      spec
                    ) ;type-error
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define

    (define (http-normalize-files who files)
      (when (not (alist? files))
        (type-error (string-append who
                      ": files must be an association list"
                    ) ;string-append
          files
        ) ;type-error
      ) ;when
      (map (lambda (entry)
             (http-normalize-file-entry who entry)
           ) ;lambda
        files
      ) ;map
    ) ;define

    (define (http-normalize-post-form-data who data)
      (cond ((null? data) '())
            ((and (string? data)
               (= (string-length data) 0)
             ) ;and
             '()
            ) ;
            ((alist? data)
             (http-normalize-string-alist who
               "data"
               data
             ) ;http-normalize-string-alist
            ) ;
            (else (type-error (string-append who
                                ": data must be an association list when files is provided"
                              ) ;string-append
                    data
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define* (http-head url)
      (let ((r (g_http-head (http-require-string "http-head"
                              "url"
                              url
                            ) ;http-require-string
               ) ;g_http-head
            ) ;r
           ) ;
        r
      ) ;let
    ) ;define*

    (define* (http-get url
               (params '())
               (headers '())
               (proxy '())
               (output-file #f)
               (stream #f)
               (callback #f)
             ) ;http-get
      (let* ((url (http-require-string "http-get"
                    "url"
                    url
                  ) ;http-require-string
             ) ;url
             (params (http-normalize-string-alist "http-get"
                       "params"
                       params
                     ) ;http-normalize-string-alist
             ) ;params
             (headers (http-normalize-string-alist "http-get"
                        "headers"
                        headers
                      ) ;http-normalize-string-alist
             ) ;headers
             (proxy (http-normalize-string-alist "http-get"
                      "proxy"
                      proxy
                    ) ;http-normalize-string-alist
             ) ;proxy
             (output-file (http-optional-string "http-get"
                            "output-file"
                            output-file
                          ) ;http-optional-string
             ) ;output-file
             (stream (http-require-boolean "http-get"
                       "stream"
                       stream
                     ) ;http-require-boolean
             ) ;stream
             (callback (http-optional-procedure "http-get"
                         "callback"
                         callback
                       ) ;http-optional-procedure
             ) ;callback
            ) ;
        (cond ((not stream)
               (g_http-get url params headers proxy #f)
              ) ;
              ((and (not output-file) (not callback))
               (value-error "http-get: stream mode requires output-file or callback"
               ) ;value-error
              ) ;
              (else (let ((stream-callback (lambda (chunk)
                                             (if callback
                                               (let ((ret (callback chunk)))
                                                 (if (boolean? ret) ret #t)
                                               ) ;let
                                               #t
                                             ) ;if
                                           ) ;lambda
                          ) ;stream-callback
                         ) ;
                      (if output-file
                        (let ((port (open-binary-output-file output-file)
                              ) ;port
                             ) ;
                          (dynamic-wind (lambda () #f)
                            (lambda ()
                              (g_http-get url
                                params
                                headers
                                proxy
                                (lambda (chunk)
                                  (write-string chunk port)
                                  (stream-callback chunk)
                                ) ;lambda
                              ) ;g_http-get
                            ) ;lambda
                            (lambda () (close-port port))
                          ) ;dynamic-wind
                        ) ;let
                        (g_http-get url
                          params
                          headers
                          proxy
                          stream-callback
                        ) ;g_http-get
                      ) ;if
                    ) ;let
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define*

    (define* (http-post url
               (params '())
               (data "")
               (headers '())
               (proxy '())
               (files '())
               (output-file #f)
               (stream #f)
               (callback #f)
             ) ;http-post
      (let* ((url (http-require-string "http-post"
                    "url"
                    url
                  ) ;http-require-string
             ) ;url
             (params (http-normalize-string-alist "http-post"
                       "params"
                       params
                     ) ;http-normalize-string-alist
             ) ;params
             (headers (http-normalize-string-alist "http-post"
                        "headers"
                        headers
                      ) ;http-normalize-string-alist
             ) ;headers
             (proxy (http-normalize-string-alist "http-post"
                      "proxy"
                      proxy
                    ) ;http-normalize-string-alist
             ) ;proxy
             (files (http-normalize-files "http-post" files)
             ) ;files
             (output-file (http-optional-string "http-post"
                            "output-file"
                            output-file
                          ) ;http-optional-string
             ) ;output-file
             (stream (http-require-boolean "http-post"
                       "stream"
                       stream
                     ) ;http-require-boolean
             ) ;stream
             (callback (http-optional-procedure "http-post"
                         "callback"
                         callback
                       ) ;http-optional-procedure
             ) ;callback
            ) ;
        (let* ((body-or-data (if (null? files)
                               (http-require-string "http-post"
                                 "data"
                                 data
                               ) ;http-require-string
                               (http-normalize-post-form-data "http-post"
                                 data
                               ) ;http-normalize-post-form-data
                             ) ;if
               ) ;body-or-data
               (headers (if (and (null? files)
                              (> (string-length body-or-data) 0)
                              (null? headers)
                            ) ;and
                          '(("Content-Type" . "text/plain"))
                          headers
                        ) ;if
               ) ;headers
              ) ;
          (cond ((not stream)
                 (g_http-post url
                   params
                   body-or-data
                   headers
                   proxy
                   files
                   #f
                 ) ;g_http-post
                ) ;
                ((and (not output-file) (not callback))
                 (value-error "http-post: stream mode requires output-file or callback"
                 ) ;value-error
                ) ;
                (else (let ((stream-callback (lambda (chunk)
                                               (if callback
                                                 (let ((ret (callback chunk)))
                                                   (if (boolean? ret) ret #t)
                                                 ) ;let
                                                 #t
                                               ) ;if
                                             ) ;lambda
                            ) ;stream-callback
                           ) ;
                        (if output-file
                          (let ((port (open-binary-output-file output-file)
                                ) ;port
                               ) ;
                            (dynamic-wind (lambda () #f)
                              (lambda ()
                                (g_http-post url
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
                              (lambda () (close-port port))
                            ) ;dynamic-wind
                          ) ;let
                          (g_http-post url
                            params
                            body-or-data
                            headers
                            proxy
                            files
                            stream-callback
                          ) ;g_http-post
                        ) ;if
                      ) ;let
                ) ;else
          ) ;cond
        ) ;let*
      ) ;let*
    ) ;define*

    ;; Async HTTP API wrapper functions

    (define* (http-async-get url
               callback
               (params '())
               (headers '())
               (proxy '())
             ) ;http-async-get
      (let ((url (http-require-string "http-async-get"
                   "url"
                   url
                 ) ;http-require-string
            ) ;url
            (callback (http-require-procedure "http-async-get"
                        "callback"
                        callback
                      ) ;http-require-procedure
            ) ;callback
            (params (http-normalize-string-alist "http-async-get"
                      "params"
                      params
                    ) ;http-normalize-string-alist
            ) ;params
            (headers (http-normalize-string-alist "http-async-get"
                       "headers"
                       headers
                     ) ;http-normalize-string-alist
            ) ;headers
            (proxy (http-normalize-string-alist "http-async-get"
                     "proxy"
                     proxy
                   ) ;http-normalize-string-alist
            ) ;proxy
           ) ;
        (g_http-async-get url
          params
          headers
          proxy
          callback
        ) ;g_http-async-get
      ) ;let
    ) ;define*

    (define* (http-async-post url
               callback
               (params '())
               (data "")
               (headers '())
               (proxy '())
             ) ;http-async-post
      (let* ((url (http-require-string "http-async-post"
                    "url"
                    url
                  ) ;http-require-string
             ) ;url
             (callback (http-require-procedure "http-async-post"
                         "callback"
                         callback
                       ) ;http-require-procedure
             ) ;callback
             (params (http-normalize-string-alist "http-async-post"
                       "params"
                       params
                     ) ;http-normalize-string-alist
             ) ;params
             (data (http-require-string "http-async-post"
                     "data"
                     data
                   ) ;http-require-string
             ) ;data
             (headers (http-normalize-string-alist "http-async-post"
                        "headers"
                        headers
                      ) ;http-normalize-string-alist
             ) ;headers
             (proxy (http-normalize-string-alist "http-async-post"
                      "proxy"
                      proxy
                    ) ;http-normalize-string-alist
             ) ;proxy
            ) ;
        (cond ((and (> (string-length data) 0)
                 (null? headers)
               ) ;and
               (g_http-async-post url
                 params
                 data
                 '(("Content-Type" . "text/plain"))
                 proxy
                 callback
               ) ;g_http-async-post
              ) ;
              (else (g_http-async-post url
                      params
                      data
                      headers
                      proxy
                      callback
                    ) ;g_http-async-post
              ) ;else
        ) ;cond
      ) ;let*
    ) ;define*

    (define* (http-async-head url
               callback
               (params '())
               (headers '())
               (proxy '())
             ) ;http-async-head
      (let ((url (http-require-string "http-async-head"
                   "url"
                   url
                 ) ;http-require-string
            ) ;url
            (callback (http-require-procedure "http-async-head"
                        "callback"
                        callback
                      ) ;http-require-procedure
            ) ;callback
            (params (http-normalize-string-alist "http-async-head"
                      "params"
                      params
                    ) ;http-normalize-string-alist
            ) ;params
            (headers (http-normalize-string-alist "http-async-head"
                       "headers"
                       headers
                     ) ;http-normalize-string-alist
            ) ;headers
            (proxy (http-normalize-string-alist "http-async-head"
                     "proxy"
                     proxy
                   ) ;http-normalize-string-alist
            ) ;proxy
           ) ;
        (g_http-async-head url
          params
          headers
          proxy
          callback
        ) ;g_http-async-head
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
