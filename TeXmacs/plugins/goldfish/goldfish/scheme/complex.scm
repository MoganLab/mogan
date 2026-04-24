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

;;; (scheme complex) library for R7RS
;;; Complex number operations

(define-library (scheme complex)
  (export make-rectangular
          make-polar
          real-part
          imag-part
          magnitude
          angle
  ) ;export
  (import (scheme base))
) ;define-library

;; Note: These functions are implemented in the underlying S7 scheme engine
;; and are made available through the library export mechanism.