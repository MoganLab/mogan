;
; Copyright (C) 2025 The Goldfish Scheme Authors
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

(define-library (liii option)
  (import (liii base))
  (export none option option?
          option-map option-filter option-flat-map option-for-each
          option-get option-get-or-else option=? option-or-else
          option-defined? option-empty?
          option-every option-any
  ) ;export
  (begin

    ; 内部表示：
    ; (cons #f 'N) 表示 none
    ; (cons value 'S) 表示 (option value)

    (define (none)
      (cons #f 'N)
    ) ;define

    (define (option value)
      (cons value 'S)
    ) ;define

    (define (option? x)
      (and (pair? x)
           (or (eq? (cdr x) 'N)
               (eq? (cdr x) 'S)
           ) ;or
      ) ;and
    ) ;define

    (define (option-empty? opt)
      (eq? (cdr opt) 'N)
    ) ;define

    (define (option-defined? opt)
      (eq? (cdr opt) 'S)
    ) ;define

    (define (option-map f opt)
      (if (option-empty? opt)
          (none)
          (option (f (car opt)))
      ) ;if
    ) ;define

    (define (option-filter pred opt)
      (if (or (option-empty? opt) (not (pred (car opt))))
          (none)
          opt
      ) ;if
    ) ;define

    (define (option-flat-map f opt)
      (if (option-empty? opt)
          (none)
          (f (car opt))
      ) ;if
    ) ;define

    (define (option-for-each f opt)
      (when (option-defined? opt)
        (f (car opt))
      ) ;when
    ) ;define

    (define (option-get opt)
      (if (option-empty? opt)
          (error "option is empty, cannot get value")
          (car opt)
      ) ;if
    ) ;define

    (define (option-get-or-else default opt)
      (if (option-empty? opt)
          (if (procedure? default)
              (default)
              default
          ) ;if
          (car opt)
      ) ;if
    ) ;define

    (define (option-or-else alt opt)
      (if (option-empty? opt)
          alt
          opt
      ) ;if
    ) ;define

    (define (option=? opt1 opt2)
      (cond ((and (option-empty? opt1) (option-empty? opt2)) #t)
            ((or (option-empty? opt1) (option-empty? opt2)) #f)
            (else (equal? (car opt1) (car opt2)))
      ) ;cond
    ) ;define

    (define (option-every pred opt)
      (if (option-empty? opt)
          #f
          (pred (car opt))
      ) ;if
    ) ;define

    (define (option-any pred opt)
      (if (option-empty? opt)
          #f
          (pred (car opt))
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
