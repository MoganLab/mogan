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

(define-library (liii either)
  (import (liii base))
  (export from-left to-left
          from-right to-right
          either?             ; 导出通用判断函数
          either-left? either-right?
          either-map either-for-each
          either-get-or-else
          either-or-else
          either-filter-or-else
          either-contains
          either-every
          either-any)
  (begin

    ;; ======================
    ;; 构造函数
    ;; ======================
    
    ;; 创建左值（错误情况）
    (define (from-left value)
      (cons value 'left))

    ;; 创建右值（成功情况）
    (define (from-right value)
      (cons value 'right))

    ;; ======================
    ;; 类型判断函数
    ;; ======================

    ;; 检查是否是左值
    (define (either-left? either)
      (and (pair? either) (eq? (cdr either) 'left)))

    ;; 检查是否是右值
    (define (either-right? either)
      (and (pair? either) (eq? (cdr either) 'right)))

    ;; 检查是否是 either 类型 (即左值或右值)
    (define (either? x)
      (or (either-left? x) 
          (either-right? x)))

    ;; 类型安全检查
    (define (check-either x func-name)
      (unless (either? x)
        (type-error 
          (format #f "In function ~a: argument must be *Either* type! **Got ~a**" 
                  func-name (object->string x)))))

    ;; ======================
    ;; 提取函数
    ;; ======================

    ;; 从 either 中提取左值
    (define (to-left either)
      (check-either either "to-left")
      (cond
        ((eq? (cdr either) 'left)
         (car either))
        (else
         (value-error "Cannot extract left from Right" either))))

    ;; 从 either 中提取右值
    (define (to-right either)
      (check-either either "to-right")
      (cond
        ((eq? (cdr either) 'right)
         (car either))
        (else
         (value-error "Cannot extract right from Left" either))))

    ;; ======================
    ;; 高阶函数操作
    ;; ======================

    ;; 映射函数：如果 either 是右值，则应用函数 f
    (define (either-map f either)
      (check-either either "either-map")
      (unless (procedure? f)
        (type-error (format #f "In function either-map: argument *f* must be *procedure*! **Got ~a**" f)))
      (if (either-right? either)
          (from-right (f (car either)))
          either))

    ;; 遍历函数：如果 either 是右值，则应用函数 f (执行副作用)
    (define (either-for-each f either)
      (check-either either "either-for-each")
      (unless (procedure? f)
        (type-error (format #f "In function either-for-each: argument *f* must be *procedure*! **Got ~a**" f)))
      (when (either-right? either)
        (f (car either))))

    ;; ======================
    ;; 逻辑判断与过滤函数 
    ;; ======================

    ;; 过滤：如果是右值且不满足 pred，则转换为 (from-left zero)
    (define (either-filter-or-else pred zero either)
      (check-either either "either-filter-or-else")
      (unless (procedure? pred) 
        (type-error 
          (format #f "In function either-filter-or-else: argument *pred* must be *procedure*! **Got ~a**" 
            (object->string pred))))
      
      ;; 注意：通常不需要检查 zero，因为它作为 left 值可以是任何类型
      
      (if (either-right? either)
          (if (pred (car either))
              either
              (from-left zero))
          either))

    ;; 包含：如果是右值且内部值等于 x
    (define (either-contains either x)
      (check-either either "either-contains")
      (and (either-right? either)
           (equal? x (car either))))

    ;; 全称量词：如果是右值则判断 pred，如果是左值默认为 #t
    (define (either-every pred either)
      (check-either either "either-every")
      (unless (procedure? pred) 
        (type-error 
          (format #f "In function either-every: argument *pred* must be *procedure*! **Got ~a**" 
            (object->string pred))))

      (if (either-right? either)
          (pred (car either))
          #t))

    ;; 存在量词：如果是右值则判断 pred，如果是左值默认为 #f
    (define (either-any pred either)
      (check-either either "either-any")
      (unless (procedure? pred) 
        (type-error 
          (format #f "In function either-any: argument *pred* must be *procedure*! **Got ~a**" 
            (object->string pred))))

      (if (either-right? either)
          (pred (car either))
          #f))

    ;; ======================
    ;; 附加实用函数
    ;; ======================

    ;; 获取值或默认值
    (define (either-get-or-else either default)
      (check-either either "either-get-or-else")
      (if (either-right? either)
          (car either)
          default))

    ;; 组合器：如果是 Left 则返回 alternative，否则返回自身
    (define (either-or-else either alternative)
      (check-either either "either-or-else")
      (if (either-right? either)
          either
          alternative))


  ) ; end of begin
) ; end of define-library