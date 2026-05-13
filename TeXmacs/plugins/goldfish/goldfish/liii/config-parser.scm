;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;  http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.
;;
;; Inspired by Python's configparser module

(define-library (liii config-parser)
  (import (scheme base)
    (scheme char)
    (scheme file)
    (liii base)
    (liii hash-table)
    (liii error)
    (srfi srfi-128)
  ) ;import
  (export make-config-parser
    config-parser?
    config-read-string
    config-read-file
    config-sections
    config-has-section?
    config-has-option?
    config-get
    config-get-int
    config-get-boolean
    config-options
    config-items
    config-set!
    config-add-section!
    config-remove-section!
    config-remove-option!
    config-write
  ) ;export
  (begin

    (define-record-type <config-parser>
      (%make-config-parser sections)
      config-parser?
      (sections config-parser-sections)
    ) ;define-record-type

    (define string-comparator
      (make-comparator string? string=? string<? string-hash)
    ) ;define

    (define *boolean-true* '("yes" "true" "on" "1"))
    (define *boolean-false* '("no" "false" "off" "0"))

    (define (string-in-list? str lst)
      (let ((lower (string-downcase str)))
        (let loop
          ((rest lst))
          (cond ((null? rest) #f)
                ((string=? lower (car rest)) #t)
                (else (loop (cdr rest)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (config-error msg)
      (error 'config-error msg)
    ) ;define

    (define (find-first-of str chars)
      (let ((len (string-length str)))
        (let loop
          ((i 0))
          (cond ((= i len) #f)
                ((member (string-ref str i) chars) i)
                (else (loop (+ i 1)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (ini-string-trim str)
      (let ((len (string-length str)))
        (let loop-start
          ((i 0))
          (if (and (< i len) (char-whitespace? (string-ref str i)))
            (loop-start (+ i 1))
            (let loop-end
              ((j (- len 1)))
              (if (and (>= j i) (char-whitespace? (string-ref str j)))
                (loop-end (- j 1))
                (substring str i (+ j 1))
              ) ;if
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (remove-comment line)
      (let ((idx (find-first-of line '(#\; #\#))))
        (if idx (substring line 0 idx) line)
      ) ;let
    ) ;define

    (define (split-at-first-sep str sep-chars)
      (let ((idx (find-first-of str sep-chars)))
        (if idx
          (cons (substring str 0 idx) (substring str (+ idx 1) (string-length str)))
          #f
        ) ;if
      ) ;let
    ) ;define

    (define (parse-section-header line)
      (let* ((trimmed (ini-string-trim line)) (len (string-length trimmed)))
        (if (and (> len 1)
              (char=? (string-ref trimmed 0) #\[)
              (char=? (string-ref trimmed (- len 1)) #\])
            ) ;and
          (ini-string-trim (substring trimmed 1 (- len 1)))
          #f
        ) ;if
      ) ;let*
    ) ;define

    (define (make-config-parser)
      (%make-config-parser (make-hash-table string-comparator))
    ) ;define

    (define (ensure-section config section-name)
      (let ((existing (hash-table-ref/default (config-parser-sections config) section-name #f)
            ) ;existing
           ) ;
        (or existing
          (let ((new-sec (make-hash-table string-comparator)))
            (hash-table-set! (config-parser-sections config) section-name new-sec)
            new-sec
          ) ;let
        ) ;or
      ) ;let
    ) ;define

    (define (config-parse-port config inport)
      (let loop
        ((current-section #f))
        (let ((line (read-line inport)))
          (cond ((eof-object? line) config)
                (else (let* ((no-comment (remove-comment line)) (trimmed (ini-string-trim no-comment)))
                        (if (string=? trimmed "")
                          (loop current-section)
                          (let ((section-name (parse-section-header trimmed)))
                            (cond (section-name (ensure-section config section-name) (loop section-name))
                                  (else (let ((kv (split-at-first-sep trimmed '(#\=
                                                                                #\:))))
                                          (when kv
                                            (let ((key (ini-string-trim (car kv))) (val (ini-string-trim (cdr kv))))
                                              (when current-section
                                                (hash-table-set! (ensure-section config current-section)
                                                  (string-downcase key)
                                                  val
                                                ) ;hash-table-set!
                                              ) ;when
                                            ) ;let
                                          ) ;when
                                          (loop current-section)
                                        ) ;let
                                  ) ;else
                            ) ;cond
                          ) ;let
                        ) ;if
                      ) ;let*
                ) ;else
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (config-read-string config str)
      (config-parse-port config (open-input-string str))
    ) ;define

    (define (config-read-file config path)
      (let ((port (open-input-file path)))
        (config-parse-port config port)
        (close-input-port port)
      ) ;let
    ) ;define

    (define (get-default-section config)
      (hash-table-ref/default (config-parser-sections config) "DEFAULT" #f)
    ) ;define

    (define (config-sections config)
      (let ((all-keys (hash-table-keys (config-parser-sections config))))
        (let loop
          ((rest all-keys) (result '()))
          (cond ((null? rest) (reverse result))
                ((string=? (car rest) "DEFAULT") (loop (cdr rest) result))
                (else (loop (cdr rest) (cons (car rest) result)))
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (config-has-section? config section)
      (hash-table-contains? (config-parser-sections config) section)
    ) ;define

    (define (config-has-option? config section option)
      (let ((opt (string-downcase option)))
        (let ((sec (hash-table-ref/default (config-parser-sections config) section #f)))
          (if (not sec)
            #f
            (or (hash-table-contains? sec opt)
              (let ((defaults (get-default-section config)))
                (and defaults (hash-table-contains? defaults opt))
              ) ;let
            ) ;or
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (config-get config section option)
      (let ((opt (string-downcase option)))
        (let ((sec (hash-table-ref/default (config-parser-sections config) section #f)))
          (cond ((not sec) (config-error (string-append "No section: " section)))
                ((hash-table-contains? sec opt) (hash-table-ref/default sec opt #f))
                (else (let ((defaults (get-default-section config)))
                        (cond ((and defaults (hash-table-contains? defaults opt))
                               (hash-table-ref/default defaults opt #f)
                              ) ;
                              (else (let ((err-msg (string-append "No option '" option "' in section '" section "'")))
                                      (config-error err-msg)
                                    ) ;let
                              ) ;else
                        ) ;cond
                      ) ;let
                ) ;else
          ) ;cond
        ) ;let
      ) ;let
    ) ;define

    (define (config-get-int config section option)
      (string->number (config-get config section option))
    ) ;define

    (define (config-get-boolean config section option)
      (let ((val (config-get config section option)))
        (cond ((string-in-list? val *boolean-true*) #t)
              ((string-in-list? val *boolean-false*) #f)
              (else (config-error (string-append "Not a boolean: " val)))
        ) ;cond
      ) ;let
    ) ;define

    (define (config-options config section)
      (let ((sec (hash-table-ref/default (config-parser-sections config) section #f)))
        (if (not sec)
          (config-error (string-append "No section: " section))
          (let ((defaults (get-default-section config)))
            (if defaults
              (let ((all-keys (hash-table-keys defaults)))
                (hash-table-for-each (lambda (k v)
                                       (when (not (member k all-keys))
                                         (set! all-keys (cons k all-keys))
                                       ) ;when
                                     ) ;lambda
                  sec
                ) ;hash-table-for-each
                all-keys
              ) ;let
              (hash-table-keys sec)
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (config-items config section)
      (let ((sec (hash-table-ref/default (config-parser-sections config) section #f)))
        (if (not sec)
          (config-error (string-append "No section: " section))
          (let ((defaults (get-default-section config)))
            (let ((result '()))
              (when defaults
                (hash-table-for-each (lambda (k v) (set! result (cons (cons k v) result)))
                  defaults
                ) ;hash-table-for-each
              ) ;when
              (hash-table-for-each (lambda (k v)
                                     (let ((existing (assoc k result)))
                                       (if existing (set-cdr! existing v) (set! result (cons (cons k v) result)))
                                     ) ;let
                                   ) ;lambda
                sec
              ) ;hash-table-for-each
              result
            ) ;let
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (config-set! config section option value)
      (let ((sec (hash-table-ref/default (config-parser-sections config) section #f)))
        (if (not sec)
          (config-error (string-append "No section: " section))
          (hash-table-set! sec (string-downcase option) value)
        ) ;if
      ) ;let
    ) ;define

    (define (config-add-section! config section)
      (if (hash-table-contains? (config-parser-sections config) section)
        (config-error (string-append "Section already exists: " section))
        (hash-table-set! (config-parser-sections config)
          section
          (make-hash-table string-comparator)
        ) ;hash-table-set!
      ) ;if
    ) ;define

    (define (config-remove-section! config section)
      (if (string=? section "DEFAULT")
        (config-error "Cannot remove DEFAULT section")
        (if (not (hash-table-contains? (config-parser-sections config) section))
          (config-error (string-append "No section: " section))
          (hash-table-delete! (config-parser-sections config) section)
        ) ;if
      ) ;if
    ) ;define

    (define (config-remove-option! config section option)
      (let ((sec (hash-table-ref/default (config-parser-sections config) section #f)))
        (if (not sec)
          (config-error (string-append "No section: " section))
          (let ((opt (string-downcase option)))
            (if (hash-table-contains? sec opt)
              (hash-table-delete! sec opt)
              (let ((defaults (get-default-section config)))
                (if (and defaults (hash-table-contains? defaults opt))
                  (config-error "Cannot remove DEFAULT option from section")
                  (config-error (string-append "No option '" option "' in section '" section "'"))
                ) ;if
              ) ;let
            ) ;if
          ) ;let
        ) ;if
      ) ;let
    ) ;define

    (define (config-write config port)
      (let ((sections (config-parser-sections config)))
        (let ((defaults (get-default-section config)))
          (when defaults
            (display "[DEFAULT]" port)
            (newline port)
            (hash-table-for-each (lambda (k v)
                                   (display k port)
                                   (display " = " port)
                                   (display v port)
                                   (newline port)
                                 ) ;lambda
              defaults
            ) ;hash-table-for-each
            (newline port)
          ) ;when
        ) ;let
        (hash-table-for-each (lambda (section-name options)
                               (unless (string=? section-name "DEFAULT")
                                 (display "[" port)
                                 (display section-name port)
                                 (display "]" port)
                                 (newline port)
                                 (hash-table-for-each (lambda (k v)
                                                        (display k port)
                                                        (display " = " port)
                                                        (display v port)
                                                        (newline port)
                                                      ) ;lambda
                                   options
                                 ) ;hash-table-for-each
                                 (newline port)
                               ) ;unless
                             ) ;lambda
          sections
        ) ;hash-table-for-each
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
