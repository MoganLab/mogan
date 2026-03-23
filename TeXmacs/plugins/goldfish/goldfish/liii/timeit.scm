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

(define-library (liii timeit)
  (export timeit)
  (import (liii base)
          (scheme time)
  ) ;import
  (begin

    (define* (timeit stmt (setup '()) (number 1000000))
      (if (not (procedure? stmt))
        (error 'type-error "(timeit stmt setup number): stmt must be a procedure")
      ) ;if
      (if (not (or (procedure? setup) (null? setup)))
        (error 'type-error "(timeit stmt setup number): setup must be a procedure or '()")
      ) ;if
      (if (not (and (integer? number) (positive? number)))
        (error 'type-error "(timeit stmt setup number): number must be a positive integer")
      ) ;if

      ; Execute setup once if it's not '()
      (unless (null? setup)
        (setup)
      ) ;unless

      ; Get start time
      (let ((start-time (current-second)))
        ; Execute stmt number times
        (do ((i 0 (+ i 1)))
            ((= i number))
          (stmt)
        ) ;do

        ; Calculate elapsed time
        (- (current-second) start-time)
      ) ;let
    ) ;define*

  ) ;begin
) ;define-library