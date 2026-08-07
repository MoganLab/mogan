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

(define-library (scheme let)
  (export
    ;; 谓词
    let?
    openlet?
    funclet?
    ;; 环境获取
    curlet
    outlet
    rootlet
    owlet
    funclet
    ;; 环境构造与操作
    inlet
    sublet
    varlet
    cutlet
    openlet
    coverlet
    unlet
    ;; 绑定访问
    let-ref
    let-set!
    let->list
    ;; 符号查找
    symbol->value
    symbol->dynamic-value
  ) ;export
  (begin
  ) ;begin
) ;define-library
