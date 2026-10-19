;;
;; Copyright (C) 2026 The Goldfish Scheme Authors
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

(define-library (liii signal)
  (export signal-trap signal-get signal-raise)
  (import (scheme base) (liii base) (liii error) (liii hash-table))
  (begin
    (define *signal-table* (g_signal-supported-signals))

    (define *handlers* (make-hash-table))

    (for-each (lambda (entry) (hash-table-set! *handlers* (car entry) 'default))
      *signal-table*
    ) ;for-each

    (define (check-signal sig who)
      (if (not (symbol? sig))
        (type-error (string-append who ": sig must be a symbol") sig)
      ) ;if
      (let ((entry (assq sig *signal-table*)))
        (if (not entry) (value-error (string-append who ": unsupported signal") sig))
        entry
      ) ;let
    ) ;define

    ;; 返回值协议：#f 表示正常结束；(type . info) 表示 handler 抛出了错误，
    ;; 由 C++ 层在复位派发状态后重抛，避免错误跳过复位逻辑
    (define (dispatch-signal sig)
      (let ((h (hash-table-ref/default *handlers* sig 'default)))
        (if (procedure? h)
          (catch #t (lambda () (h sig) #f) (lambda (type info) (cons type info)))
          #f
        ) ;if
      ) ;let
    ) ;define

    (g_signal-set-dispatcher dispatch-signal)

    (define (signal-trap sig handler)
      (let ((entry (check-signal sig "(signal-trap sig handler)")))
        (if (not (or (procedure? handler) (eq? handler 'default) (eq? handler 'ignore)))
          (type-error "(signal-trap sig handler): handler must be a procedure, 'default, or 'ignore"
            handler
          ) ;type-error
        ) ;if
        (let ((old-handler (hash-table-ref/default *handlers* sig 'default))
              (mode (cond ((eq? handler 'default) 'default)
                          ((eq? handler 'ignore) 'ignore)
                          (else 'catch)
                    ) ;cond
              ) ;mode
             ) ;
          (if (not (g_signal-os-trap (cdr entry) mode))
            (os-error "(signal-trap sig handler): failed to set OS signal action" sig)
          ) ;if
          (hash-table-set! *handlers* sig handler)
          old-handler
        ) ;let
      ) ;let
    ) ;define

    (define (signal-get sig)
      (let ((entry (check-signal sig "(signal-get sig)")))
        (hash-table-ref/default *handlers* sig 'default)
      ) ;let
    ) ;define

    (define (signal-raise sig)
      (let ((entry (check-signal sig "(signal-raise sig)")))
        (g_signal-raise (cdr entry))
        (values)
      ) ;let
    ) ;define
  ) ;begin
) ;define-library
