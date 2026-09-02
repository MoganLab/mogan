(define-library (liii json)
  (import (liii base) (liii list))
  (export json-string-escape
    string->json
    json->string

    json-ref
    json-set
    json-push
    json-drop
    json-reduce

    json-null?
    json-object?
    json-array?
    json-string?
    json-float?
    json-number?
    json-integer?
    json-boolean?

    json-contains-key?

    json-ref-string
    json-ref-number
    json-ref-integer
    json-ref-boolean
    json-get-or-else

    json-keys
  ) ;export

  (begin

    ;; ; ---------------------------------------------------------
    ;; ; 0. 统一接口
    ;; ; ---------------------------------------------------------

    ;; json-string-escape 由 C++ 实现（src/liii_json.cpp 中的 g_json_string_escape）
    (define json-string-escape g_json_string_escape)

    ;; json->string 由 C++ 实现（src/liii_json.cpp 中的 g_json->string）
    (define json->string g_json->string)

    ;; string->json 由 C++ 实现（src/liii_json.cpp 中的 g_string->json）
    (define string->json g_string->json)

    ;; json-ref 由 C++ 实现（src/liii_json.cpp 中的 g_json_ref）
    (define json-ref g_json_ref)

    ;; json-set 由 C++ 实现（src/liii_json.cpp 中的 g_json_set）
    (define json-set g_json_set)

    ;; json-push 由 C++ 实现（src/liii_json.cpp 中的 g_json_push，含变参多键路径，
    ;; 语义覆盖历史上的 json-push 与 json-push*）
    (define json-push g_json_push)

    ;; json-drop 由 C++ 实现（src/liii_json.cpp 中的 g_json_drop，含变参多键路径，
    ;; 语义覆盖历史上的 json-drop 与 json-drop*）
    (define json-drop g_json_drop)

    ;; json-reduce 由 C++ 实现（src/liii_json.cpp 中的 g_json_reduce，含变参多键路径，
    ;; 语义覆盖历史上的 json-reduce 多层路径模式）
    (define json-reduce g_json_reduce)

    (define (ensure-json-structure x)
      (unless (or (json-object? x) (json-array? x))
        (type-error "Value is not a JSON object or array" x)
      ) ;unless
    ) ;define

    ;; ; ---------------------------------------------------------
    ;; ; 1. 类型谓词
    ;; ; ---------------------------------------------------------

    (define (json-null? x)
      (eq? x 'null)
    ) ;define

    (define (json-object? x)
      (and (list? x) (not (null? x)) (or (equal? x '(())) (every pair? x)))
    ) ;define

    (define (json-array? x)
      (vector? x)
    ) ;define

    (define (json-string? x)
      (string? x)
    ) ;define

    (define (json-number? x)
      (number? x)
    ) ;define

    (define (json-integer? x)
      (integer? x)
    ) ;define

    (define (json-float? x)
      (float? x)
    ) ;define

    (define (json-boolean? x)
      (boolean? x)
    ) ;define

    ;; ; ---------------------------------------------------------
    ;; ; 2. 状态检查
    ;; ; ---------------------------------------------------------

    (define (json-contains-key? json key)
      (if (not (json-object? json))
        #f
        (if (equal? json '(())) #f (if (assoc key json) #t #f))
      ) ;if
    ) ;define

    ;; ; ---------------------------------------------------------
    ;; ; 3. 安全获取器
    ;; ; ---------------------------------------------------------

    (define (json-get-or-else json default)
      (if (json-null? json) default json)
    ) ;define

    (define (json-ref-string json key default)
      (let ((val (json-ref json key)))
        (if (string? val) val default)
      ) ;let
    ) ;define

    (define (json-ref-number json key default)
      (let ((val (json-ref json key)))
        (if (number? val) val default)
      ) ;let
    ) ;define

    (define (json-ref-integer json key default)
      (let ((val (json-ref json key)))
        (if (integer? val) val default)
      ) ;let
    ) ;define

    (define (json-ref-boolean json key default)
      (let ((val (json-ref json key)))
        (if (boolean? val) val default)
      ) ;let
    ) ;define

    ;; ; ---------------------------------------------------------
    ;; ; 4. 辅助工具
    ;; ; ---------------------------------------------------------

    (define (json-keys json)
      (if (json-object? json) (if (equal? json '(())) '() (map car json)) '())
    ) ;define

  ) ;begin
) ;define-library
