;
; Copyright (C) 2026 The Goldfish Scheme Authors
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
; http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
; License for the specific language governing permissions and limitations
; under the License.
;

(define-library (liii path)
  (export
    path path-from-string
    path-dir? path-file? path-exists?
    path-getsize path-read-text path-read-bytes
    path-write-text path-append-text path-touch
    path-root path-of-drive path-from-parts path-from-env
    path-cwd path-home path-temp-dir
    path-parts path-type path-drive path-copy
    path->string path-to-string
    path-name path-stem path-suffix
    path-equals? path=?
    path-absolute? path-relative?
    path-join path-parent
    path-list path-list-path
    path-rmdir path-unlink
  ) ;export
  (import (liii base)
          (liii error)
          (liii os)
          (prefix (liii rich-path) rich-)
  ) ;import
  (begin

    (define (normalize-string-path value)
      (if (os-windows?)
          (string-map (lambda (ch)
                        (if (char=? ch #\/)
                            #\\
                            ch
                        ) ;if
                      ) ;lambda
                      value
          ) ;string-map
          value
      ) ;if
    ) ;define

    (define (path-object? value)
      (rich-path :is-type-of value)
    ) ;define

    (define (path->object value func-name)
      (cond ((path-object? value) value)
            ((string? value) (rich-path (normalize-string-path value)))
            (else
              (type-error (string-append func-name ": path must be string or path"))
            ) ;else
      ) ;cond
    ) ;define

    (define (path->input-string value func-name)
      (cond ((path-object? value) (value :to-string))
            ((string? value) (normalize-string-path value))
            (else
              (type-error (string-append func-name ": path must be string or path"))
            ) ;else
      ) ;cond
    ) ;define

    (define* (path (value "."))
      (if (path-object? value)
          (value :copy)
          (path->object value "path")
      ) ;if
    ) ;define*

    (define path-from-string path)

    (define (path-dir? value)
      (rich-path-dir? (path->input-string value "path-dir?"))
    ) ;define

    (define (path-file? value)
      (rich-path-file? (path->input-string value "path-file?"))
    ) ;define

    (define (path-exists? value)
      (rich-path-exists? (path->input-string value "path-exists?"))
    ) ;define

    (define (path-getsize value)
      (rich-path-getsize (path->input-string value "path-getsize"))
    ) ;define

    (define (path-read-text value)
      (rich-path-read-text (path->input-string value "path-read-text"))
    ) ;define

    (define (path-read-bytes value)
      (rich-path-read-bytes (path->input-string value "path-read-bytes"))
    ) ;define

    (define (path-write-text value content)
      (if (not (string? content))
          (type-error "path-write-text: content must be string")
          (rich-path-write-text (path->input-string value "path-write-text") content)
      ) ;if
    ) ;define

    (define (path-append-text value content)
      (if (not (string? content))
          (type-error "path-append-text: content must be string")
          (rich-path-append-text (path->input-string value "path-append-text") content)
      ) ;if
    ) ;define

    (define (path-touch value)
      (rich-path-touch (path->input-string value "path-touch"))
    ) ;define

    (define (path-root)
      (rich-path :root)
    ) ;define

    (define (path-of-drive ch)
      (rich-path :of-drive ch)
    ) ;define

    (define (path-from-parts x)
      (rich-path :from-parts x)
    ) ;define

    (define (path-from-env name)
      (rich-path :from-env name)
    ) ;define

    (define (path-cwd)
      (rich-path :cwd)
    ) ;define

    (define (path-home)
      (rich-path :home)
    ) ;define

    (define (path-temp-dir)
      (rich-path :temp-dir)
    ) ;define

    (define (path-parts value)
      ((path->object value "path-parts") :get-parts)
    ) ;define

    (define (path-type value)
      ((path->object value "path-type") :get-type)
    ) ;define

    (define (path-drive value)
      ((path->object value "path-drive") :get-drive)
    ) ;define

    (define (path-copy value)
      ((path->object value "path-copy") :copy)
    ) ;define

    (define (path->string value)
      (path->input-string value "path->string")
    ) ;define

    (define path-to-string path->string)

    (define (path-name value)
      ((path->object value "path-name") :name)
    ) ;define

    (define (path-stem value)
      ((path->object value "path-stem") :stem)
    ) ;define

    (define (path-suffix value)
      ((path->object value "path-suffix") :suffix)
    ) ;define

    (define (path-equals? left right)
      ((path->object left "path-equals?") :equals (path->object right "path-equals?"))
    ) ;define

    (define path=? path-equals?)

    (define (path-absolute? value)
      ((path->object value "path-absolute?") :absolute?)
    ) ;define

    (define (path-relative? value)
      ((path->object value "path-relative?") :relative)
    ) ;define

    (define (path-join base . segments)
      (let loop ((acc (path->object base "path-join"))
                 (rest segments))
        (if (null? rest)
            acc
            (loop (acc :/ (car rest)) (cdr rest))
        ) ;if
      ) ;let
    ) ;define

    (define (path-parent value)
      (let* ((path-value (path->object value "path-parent"))
             (parent (path-value :parent)))
        (if (and (os-windows?)
                 (string=? (parent :to-string) "")
                 (path-relative? path-value)
            ) ;and
            (path ".")
            parent
        ) ;if
      ) ;let*
    ) ;define

    (define (path-list value)
      (listdir (path->string value))
    ) ;define

    (define (path-list-path value)
      (vector-map
        (lambda (entry) (path-join (path->object value "path-list-path") entry))
        (path-list value)
      ) ;vector-map
    ) ;define

    (define (path-rmdir value)
      ((path->object value "path-rmdir") :rmdir)
    ) ;define

    (define* (path-unlink value (missing-ok #f))
      ((path->object value "path-unlink") :unlink missing-ok)
    ) ;define*

  ) ;begin
) ;define-library
