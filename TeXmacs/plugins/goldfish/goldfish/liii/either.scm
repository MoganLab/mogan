(define-library (liii either)
  (import (liii base))
  (export from-left
    to-left
    from-right
    to-right
    either?
    either-left?
    either-right?
    either-map
    either-flat-map
    either-for-each
    either-get-or-else
    either-or-else
    either-filter-or-else
    either-contains?
    either-every
    either-any
  ) ;export
  (begin

    ;; ======================
    ;; 构造函数
    ;; ======================

    ;; 创建左值（错误情况）
    (define (from-left value)
      (cons value 'left)
    ) ;define

    ;; 创建右值（成功情况）
    (define (from-right value)
      (cons value 'right)
    ) ;define

    ;; ======================
    ;; 类型判断函数
    ;; ======================

    ;; 检查是否是左值
    (define (either-left? either)
      (and (pair? either)
        (eq? (cdr either) 'left)
      ) ;and
    ) ;define

    ;; 检查是否是右值
    (define (either-right? either)
      (and (pair? either)
        (eq? (cdr either) 'right)
      ) ;and
    ) ;define

    ;; 检查是否是 either 类型 (即左值或右值)
    (define (either? x)
      (or (either-left? x) (either-right? x))
    ) ;define

    ;; 类型安全检查
    (define (check-either x func-name)
      (unless (either? x)
        (type-error (format #f
                      "In function ~a: argument must be *Either* type! **Got ~a**"
                      func-name
                      (object->string x)
                    ) ;format
        ) ;type-error
      ) ;unless
    ) ;define

    ;; ======================
    ;; 提取函数
    ;; ======================

    ;; 从 either 中提取左值
    (define (to-left either)
      (check-either either "to-left")
      (cond ((eq? (cdr either) 'left) (car either))
            (else (value-error "Cannot extract left from Right"
                    either
                  ) ;value-error
            ) ;else
      ) ;cond
    ) ;define

    ;; 从 either 中提取右值
    (define (to-right either)
      (check-either either "to-right")
      (cond ((eq? (cdr either) 'right) (car either))
            (else (value-error "Cannot extract right from Left"
                    either
                  ) ;value-error
            ) ;else
      ) ;cond
    ) ;define

    ;; ======================
    ;; 高阶函数操作
    ;; ======================

    ;; 映射函数：如果 either 是右值，则应用函数 f
    (define (either-map f either)
      (check-either either "either-map")
      (unless (procedure? f)
        (type-error (format #f
                      "In function either-map: argument *f* must be *procedure*! **Got ~a**"
                      f
                    ) ;format
        ) ;type-error
      ) ;unless
      (if (either-right? either)
        (from-right (f (car either)))
        either
      ) ;if
    ) ;define

    ;; 扁平映射函数：如果 either 是右值，则应用返回 Either 的函数 f
    (define (either-flat-map f either)
      (check-either either "either-flat-map")
      (unless (procedure? f)
        (type-error (format #f
                      "In function either-flat-map: argument *f* must be *procedure*! **Got ~a**"
                      f
                    ) ;format
        ) ;type-error
      ) ;unless
      (if (either-right? either)
        (let ((result (f (to-right either))))
          (check-either result
            "either-flat-map: return value of f must be an Either"
          ) ;check-either
          result
        ) ;let
        either
      ) ;if
    ) ;define

    ;; 遍历函数：如果 either 是右值，则应用函数 f (执行副作用)
    (define (either-for-each f either)
      (check-either either "either-for-each")
      (unless (procedure? f)
        (type-error (format #f
                      "In function either-for-each: argument *f* must be *procedure*! **Got ~a**"
                      f
                    ) ;format
        ) ;type-error
      ) ;unless
      (when (either-right? either)
        (f (car either))
      ) ;when
    ) ;define

    ;; ======================
    ;; 逻辑判断与过滤函数
    ;; ======================

    ;; 过滤：如果是右值且不满足 pred，则转换为 (from-left zero)
    (define (either-filter-or-else pred zero either)
      (check-either either
        "either-filter-or-else"
      ) ;check-either
      (unless (procedure? pred)
        (type-error (format #f
                      "In function either-filter-or-else: argument *pred* must be *procedure*! **Got ~a**"
                      (object->string pred)
                    ) ;format
        ) ;type-error
      ) ;unless

      ;; 注意：通常不需要检查 zero，因为它作为 left 值可以是任何类型

      (if (either-right? either)
        (if (pred (car either))
          either
          (from-left zero)
        ) ;if
        either
      ) ;if
    ) ;define

    ;; 包含：如果是右值且内部值等于 x
    (define (either-contains? either x)
      (check-either either "either-contains?")
      (and (either-right? either)
        (equal? x (car either))
      ) ;and
    ) ;define

    ;; 全称量词：如果是右值则判断 pred，如果是左值默认为 #t
    (define (either-every pred either)
      (check-either either "either-every")
      (unless (procedure? pred)
        (type-error (format #f
                      "In function either-every: argument *pred* must be *procedure*! **Got ~a**"
                      (object->string pred)
                    ) ;format
        ) ;type-error
      ) ;unless

      (if (either-right? either)
        (pred (car either))
        #t
      ) ;if
    ) ;define

    ;; 存在量词：如果是右值则判断 pred，如果是左值默认为 #f
    (define (either-any pred either)
      (check-either either "either-any")
      (unless (procedure? pred)
        (type-error (format #f
                      "In function either-any: argument *pred* must be *procedure*! **Got ~a**"
                      (object->string pred)
                    ) ;format
        ) ;type-error
      ) ;unless

      (if (either-right? either)
        (pred (car either))
        #f
      ) ;if
    ) ;define

    ;; ======================
    ;; 附加实用函数
    ;; ======================

    ;; 获取值或默认值
    (define (either-get-or-else either default)
      (check-either either
        "either-get-or-else"
      ) ;check-either
      (if (either-right? either)
        (car either)
        default
      ) ;if
    ) ;define

    ;; 组合器：如果是 Left 则返回 alternative，否则返回自身
    (define (either-or-else either alternative)
      (check-either either "either-or-else")
      (if (either-right? either)
        either
        alternative
      ) ;if
    ) ;define

  ) ;begin
) ;define-library
