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

; The (scheme inexact) library exports procedures which are typically only
; useful with inexact values
(define-library (scheme inexact)
  (export acos asin atan cos exp finite? infinite? log nan? sin sqrt s7-sqrt tan)
  (begin

    (define s7-sqrt sqrt)

    (define (sqrt x)
      (if (and (exact? x) (negative? x))
        (s7-sqrt (inexact x))
        (s7-sqrt x)
      ) ;if
    ) ;define

    (define (finite? x)
      (and (number? x)
           (not (infinite? x))
           (not (nan? x))
      ) ;and
    ) ;define

  ) ;begin
) ;define-library

