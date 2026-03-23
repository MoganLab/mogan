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

(define-library (liii range)
  (import (srfi srfi-196))
  (export range numeric-range vector-range string-range range-append
          iota-range range? range=? range-length range-ref range-first
          range-last subrange range-segment range-split-at range-take
          range-take-right range-drop range-drop-right range-count
          range-map->list range-for-each range-fold range-fold-right
          range-any range-every range-filter->list range-remove->list
          range-reverse range-map->vector range-filter->vector
          range-remove->vector vector->range range->list range->vector
          range->string range->generator))
  (begin
    ; (liii range) 重新导出 (srfi srfi-196) 的所有函数
  )
)
