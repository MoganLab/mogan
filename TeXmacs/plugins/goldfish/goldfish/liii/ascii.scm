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

(define-library (liii ascii)
  (export ascii-codepoint?
          ascii-bytevector?

          ascii-char?
          ascii-string?

          ascii-control?
          ascii-non-control?
          ascii-whitespace?
          ascii-space-or-tab?
          ascii-other-graphic?
          ascii-upper-case?
          ascii-lower-case?
          ascii-alphabetic?
          ascii-alphanumeric?
          ascii-numeric?

          ascii-digit-value
          ascii-upper-case-value
          ascii-lower-case-value
          ascii-nth-digit
          ascii-nth-upper-case
          ascii-nth-lower-case
          ascii-upcase
          ascii-downcase
          ascii-control->graphic
          ascii-graphic->control
          ascii-mirror-bracket

          ascii-ci=?
          ascii-ci<?
          ascii-ci>?
          ascii-ci<=?
          ascii-ci>=?

          ascii-string-ci=?
          ascii-string-ci<?
          ascii-string-ci>?
          ascii-string-ci<=?
          ascii-string-ci>=?)
  (import (srfi srfi-175)))
