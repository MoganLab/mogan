;
; Copyright (C) 2024 The Goldfish Scheme Authors
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

(define-library (liii logging)
  (import (liii lang)
          (liii rich-path)
          (liii datetime)
          (liii error)
  ) ;import
  (export logging)
  (begin

    (define-constant NOTSET 0)
    (define-constant DEBUG 10)
    (define-constant INFO 20)
    (define-constant WARNING 30)
    (define-constant ERROR 40)
    (define-constant CRITICAL 50)

    (define loggers-registry (make-hash-table))
    (define-class logging
      ((name string? "default")
       (log-path string? "")
       (level integer? WARNING)
      ) ;
  

      (define (%set-path! p)
        (cond ((string? p)
               (set! log-path p))
        
              ((path :is-type-of p)
               (set! log-path (p :to-string))
              ) ;
        
              (else
               (type-error "path should be a string or path object")
              ) ;else
        ) ;cond
      ) ;define

      (define (%set-level! l)
        (define (check-valid-level val)
          (member val '(0 10 20 30 40 50))
        ) ;define
  
        (cond ((integer? l) 
               (if (check-valid-level l)
                   (set! level l)
                   (value-error "invalid level number" l))
               ) ;if

              ((rich-integer :is-type-of l) 
               (if (check-valid-level (l :get))
                   (set! level (l :get))
                   (value-error "invalid level number" (l :get))
               ) ;if
              ) ;

              (else 
               (type-error "level should be an integer")
              ) ;else
        ) ;cond
      ) ;define

      (define (@apply p-name)
        ;; Check if logger with this name already exists in registry
        (let ((existing-logger (hash-table-ref loggers-registry p-name)))
          (if (eq? existing-logger #f)
              ;; If not, create a new logger and store in registry
              (let ((new-logger (logging)))
                (new-logger :set-name! p-name)
                (hash-table-set! loggers-registry p-name new-logger)
                new-logger
              ) ;let
              ;; If exists, return existing logger
              existing-logger
          ) ;if
        ) ;let
      ) ;define

      (define (format-timestamp)
        (let ((now (datetime :now)))
          (now :to-string)
        ) ;let
      ) ;define

      (define (print-log level-name . args)
        (let* ((timestamp (format-timestamp))
               (prefix (string-append timestamp " [" level-name "] " name ": "))
               (message (apply string-append 
                               (map (lambda (arg) 
                                      (if (string? arg) 
                                          arg 
                                          (arg :get))
                                      ) ;if
                                    args))
                               ) ;map
               ) ;message
          (let ((line (string-append prefix message "\n")))
            (if (string=? log-path "")
                (display line)
                (path-append-text log-path line)
            ) ;if
          ) ;let
        ) ;let*
      ) ;define

      (define (%get-level)
        (cond
          ((= level 0) "NOTSET")
          ((= level 10) "DEBUG")
          ((= level 20) "INFO")
          ((= level 30) "WARNING")
          ((= level 40) "ERROR")
          ((= level 50) "CRITICAL")
        ) ;cond
      ) ;define

      (define (%debug?)
        (<= level DEBUG)
      ) ;define

      (define (%info?)
        (<= level INFO)
      ) ;define

      (define (%warning?)
        (<= level WARNING)
      ) ;define

      (define (%error?)
        (<= level ERROR)
      ) ;define

      (define (%critical?)
        (<= level CRITICAL)
      ) ;define

      (define (%debug . args)
        (when (%debug?)
          (apply print-log "DEBUG" args)
        ) ;when
      ) ;define

      (define (%info . args)
        (when (%info?)
          (apply print-log "INFO" args)
        ) ;when
      ) ;define

      (define (%warning . args)
        (when (%warning?)
          (apply print-log "WARNING" args)
        ) ;when
      ) ;define

      (define (%error . args)
        (when (%error?)
          (apply print-log "ERROR" args)
        ) ;when
      ) ;define

      (define (%critical . args)
        (when (%critical?)
          (apply print-log "CRITICAL" args))
        ) ;when

      ) ;define

    ) ;define-class
  ) ;begin
