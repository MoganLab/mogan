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
;; distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
;; WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
;; License for the specific language governing permissions and limitations
;; under the License.
;;

(define-library (srfi srfi-166 base)
  (import (liii format base))
  (export show displayed written written-shared written-simply escaped
    maybe-escaped numeric numeric/comma numeric/si numeric/fitted nl fl space-to
    tab-to nothing each each-in-list joined joined/prefix joined/suffix
    joined/last joined/dot joined/range padded padded/right padded/both trimmed
    trimmed/right trimmed/both trimmed/lazy fitted fitted/right fitted/both
    output-default fn with with! forked call-with-output make-state-variable
    port row col width output writer pad-char ellipsis string-width
    substring/width substring/preserve radix precision decimal-sep decimal-align
    sign-rule comma-sep comma-rule word-separator? ambiguous-is-wide?
    pretty-environment
  ) ;export
  (begin
  ) ;begin
) ;define-library
