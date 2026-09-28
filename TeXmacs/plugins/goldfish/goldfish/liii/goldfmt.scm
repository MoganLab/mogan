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

(define-library (liii goldfmt)
  (export format-string format-scheme-string format-stem-string format-datum
    format-node format-nodes can-inline? scan-string call-with-stem-mode
    string-contains
  ) ;export
  (import (liii base)
    (liii goldfmt scan)
    (liii goldfmt format)
    (liii goldfmt stem)
    (srfi srfi-13)
  ) ;import
  (begin
    (define format-scheme-string format-string)
  ) ;begin
) ;define-library
