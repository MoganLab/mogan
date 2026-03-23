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

(define-library (liii stack)
  (import (liii rich-list) (liii oop))
  (export stack)
  (begin

    (define-case-class stack ((data list?))
      (define (@empty) (stack (list )))

      (define (%length) (length data))

      (define (%size) (length data))

      (define (%top)
        (if (null? data)
            (error 'out-of-range)
            (car data)
        ) ;if
      ) ;define

      (chained-define (%pop)
        (if (null? data)
            (error 'out-of-range "Cannot pop from an empty stack")
            (stack (cdr data))
        ) ;if
      ) ;chained-define

      (chained-define (%pop!)
        (if (null? data)
            (error 'out-of-range)
            (stack (set! data (cdr data)))
        ) ;if
        (%this)
      ) ;chained-define

      (chained-define (%push element)
        (stack (cons element data))
      ) ;chained-define

      (chained-define (%push! element) 
                      (stack (set! data (cons element data))) 
                      (%this)
      ) ;chained-define

      (define (%to-list) data)

      (define (%to-rich-list) (rich-list data))
    ) ;define-case-class
  ) ;begin
) ;define-library
