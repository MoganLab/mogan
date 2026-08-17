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

(define-library (liii http-common)
  (import (liii alist) (liii error) (scheme file))
  (export http-ok? http-require-string http-require-procedure
    http-require-boolean http-optional-string http-optional-procedure
    http-scalar->string http-normalize-string-alist http-normalize-files
    http-normalize-post-form-data
  ) ;export
  (begin

    (define (http-ok? r)
      (let ((status-code (r 'status-code)) (reason (r 'reason)) (url (r 'url)))
        (cond ((and (>= status-code 400) (< status-code 500))
               (error 'http-error
                 (string-append (number->string status-code)
                   " Client Error: "
                   reason
                   " for url: "
                   url
                 ) ;string-append
               ) ;error
              ) ;
              ((and (>= status-code 500) (< status-code 600))
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
      (if value (http-require-string who field value) #f)
    ) ;define

    (define (http-optional-procedure who field value)
      (if value (http-require-procedure who field value) #f)
    ) ;define

    (define (http-scalar->string who field value)
      (cond ((string? value) value)
            ((symbol? value) (symbol->string value))
            ((or (integer? value) (real? value)) (number->string value))
            (else (type-error (string-append who ": " field " must be a string, symbol, or number")
                    value
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define (http-normalize-string-alist-entry who field entry)
      (when (not (pair? entry))
        (type-error (string-append who ": " field " entries must be key/value pairs")
          entry
        ) ;type-error
      ) ;when
      (when (pair? (cdr entry))
        (type-error (string-append who ": " field " entries must be key/value pairs")
          entry
        ) ;type-error
      ) ;when
      (cons (http-scalar->string who (string-append field " key") (car entry))
        (http-scalar->string who (string-append field " value") (cdr entry))
      ) ;cons
    ) ;define

    (define (http-normalize-string-alist who field entries)
      (when (not (alist? entries))
        (type-error (string-append who ": " field " must be an association list")
          entries
        ) ;type-error
      ) ;when
      (map (lambda (entry) (http-normalize-string-alist-entry who field entry))
        entries
      ) ;map
    ) ;define

    (define (http-normalize-part-key who key)
      (cond ((string? key) key)
            ((symbol? key) (symbol->string key))
            (else (type-error (string-append who ": multipart part key must be string or symbol")
                    key
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

    (define http-file-spec-keys '("file" "filename" "content-type"))

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
      (let* ((key (http-normalize-part-key who (car entry))) (value (cdr entry)))
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
      (let* ((name (http-scalar->string who "files key" (car entry))) (spec (cdr entry)))
        (cond ((string? spec)
               (when (not (file-exists? spec))
                 (value-error (string-append who ": file does not exist") spec)
               ) ;when
               `((name . ,name) (file . ,spec))
              ) ;
              ((alist? spec)
               (let* ((normalized-spec (map (lambda (item) (http-normalize-file-spec-entry who item)) spec)
                      ) ;normalized-spec
                      (file (http-part-ref normalized-spec "file"))
                      (filename (http-part-ref normalized-spec "filename"))
                      (content-type (http-part-ref normalized-spec "content-type"))
                     ) ;
                 (when (not file)
                   (value-error (string-append who ": file spec requires a file path") spec)
                 ) ;when
                 (when (not (file-exists? file))
                   (value-error (string-append who ": file does not exist") file)
                 ) ;when
                 (append `((name . ,name) (file . ,file))
                   (if filename `((filename . ,filename)) '())
                   (if content-type `((content-type . ,content-type)) '())
                 ) ;append
               ) ;let*
              ) ;
              (else (type-error (string-append who ": files value must be a path string or file spec alist")
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
      (map (lambda (entry) (http-normalize-file-entry who entry)) files)
    ) ;define

    (define (http-normalize-post-form-data who data)
      (cond ((null? data) '())
            ((and (string? data) (= (string-length data) 0)) '())
            ((alist? data) (http-normalize-string-alist who "data" data))
            (else (type-error (string-append who ": data must be an association list when files is provided")
                    data
                  ) ;type-error
            ) ;else
      ) ;cond
    ) ;define

  ) ;begin
) ;define-library
