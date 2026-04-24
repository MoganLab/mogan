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

(define-library (liii bitwise)
  (import (srfi srfi-151) (liii error))
  ; S7 built-in
  (export lognot logand logior logxor ash)
  ; from (srfi srfi-151)
  (export bitwise-not bitwise-and bitwise-ior bitwise-xor bitwise-eqv bitwise-or bitwise-nor
          bitwise-nand bit-count bitwise-orc1 bitwise-orc2 bitwise-andc1 bitwise-andc2
          arithmetic-shift integer-length bitwise-if bit-set? copy-bit bit-swap any-bit-set?
          every-bit-set? first-set-bit bit-field bit-field-any? bit-field-every? bit-field-clear
          bit-field-set
  ) ;export
  (begin
    (define bitwise-or bitwise-ior)
  ) ;begin
) ;define-library

