;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0251.scm
;; DESCRIPTION : Tests for njson error handling robustness
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(import (liii njson))

(check-set-mode! 'report)

;;; ========== 非 ASCII 字符串测试 ==========

;; string->njson with non-ASCII string in JSON value (Erwin Schrödinger)

(define (test-njson-non-ascii-value)
  (let ((j (string->njson "{\"name\":\"Erwin Schr\\u00f6dinger\"}")))
    (check (njson-ref j "name") => "Erwin Schrödinger")
    (njson-free j)
  ) ;let
) ;define

;; string->njson with non-ASCII string in JSON key

(define (test-njson-non-ascii-key)
  (let ((j (string->njson "{\"名前\":\"Tsubasa\"}")))
    (check (njson-ref j "名前") => "Tsubasa")
    (njson-free j)
  ) ;let
) ;define

;; njson->string with non-ASCII content round-trip

(define (test-njson-non-ascii-roundtrip)
  (let* ((original "{\"author\":\"Erwin Schrödinger\",\"city\":\"Zürich\"}")
         (j (string->njson original))
        ) ;
    (check (njson-ref j "author") => "Erwin Schrödinger")
    (check (njson-ref j "city") => "Zürich")
    (njson-free j)
  ) ;let*
) ;define

;;; ========== 畸形 JSON 解析测试 ==========

;; malformed JSON: missing closing brace

(define (test-njson-malformed-missing-brace)
  (check (catch #t (lambda () (string->njson "{\"key\":\"value\"")) (lambda args #t))
    =>
    #t
  ) ;check
) ;define

;; malformed JSON: trailing comma

(define (test-njson-malformed-trailing-comma)
  (check (catch #t (lambda () (string->njson "{\"key\":\"value\",}")) (lambda args #t))
    =>
    #t
  ) ;check
) ;define

;; malformed JSON: single-quoted strings

(define (test-njson-malformed-single-quotes)
  (check (catch #t (lambda () (string->njson "{'key':'value'}")) (lambda args #t))
    =>
    #t
  ) ;check
) ;define

;; malformed JSON: empty string

(define (test-njson-malformed-empty)
  (check (catch #t (lambda () (string->njson "")) (lambda args #t)) => #t)
) ;define

;; malformed JSON: just plain text

(define (test-njson-malformed-plain-text)
  (check (catch #t (lambda () (string->njson "not json at all")) (lambda args #t))
    =>
    #t
  ) ;check
) ;define

;; malformed JSON: truncated array

(define (test-njson-malformed-truncated-array)
  (check (catch #t (lambda () (string->njson "[1, 2, 3")) (lambda args #t)) => #t)
) ;define

;;; ========== 无效 JSON 操作测试 ==========

;; ref on non-existent key

(define (test-njson-ref-missing-key)
  (let ((j (string->njson "{\"a\":1}")))
    (check (catch #t (lambda () (njson-ref j "nonexistent")) (lambda args #t))
      =>
      #t
    ) ;check
    (njson-free j)
  ) ;let
) ;define

;; ref on array with string key

(define (test-njson-ref-array-string-key)
  (let ((j (string->njson "[1,2,3]")))
    (check (catch #t (lambda () (njson-ref j "not-a-number")) (lambda args #t))
      =>
      #t
    ) ;check
    (njson-free j)
  ) ;let
) ;define

;; ref with out-of-bounds index

(define (test-njson-ref-out-of-bounds)
  (let ((j (string->njson "[1,2,3]")))
    (check (catch #t (lambda () (njson-ref j 10)) (lambda args #t)) => #t)
    (njson-free j)
  ) ;let
) ;define

;; set on non-object/array (scalar)

(define (test-njson-set-on-scalar)
  (let ((j (string->njson "42")))
    (check (catch #t (lambda () (njson-set j "key" "value")) (lambda args #t))
      =>
      #t
    ) ;check
    (njson-free j)
  ) ;let
) ;define

;; append on non-array (object)

(define (test-njson-append-on-object)
  (let ((j (string->njson "{\"a\":1}")))
    (check (catch #t (lambda () (njson-append j "value")) (lambda args #t)) => #t)
    (njson-free j)
  ) ;let
) ;define

;; object->alist on array

(define (test-njson-object->alist-on-array)
  (let ((j (string->njson "[1,2,3]")))
    (check (catch #t (lambda () (njson-object->alist j)) (lambda args #t)) => #t)
    (njson-free j)
  ) ;let
) ;define

;; array->list on object

(define (test-njson-array->list-on-object)
  (let ((j (string->njson "{\"a\":1}")))
    (check (catch #t (lambda () (njson-array->list j)) (lambda args #t)) => #t)
    (njson-free j)
  ) ;let
) ;define

;;; ========== 类型错误测试 ==========

;; string->njson with non-string

(define (test-njson-string-to-json-type-error)
  (check (catch #t (lambda () (string->njson 42)) (lambda args #t)) => #t)
) ;define

;; njson-ref with non-njson

(define (test-njson-ref-type-error)
  (check (catch #t (lambda () (njson-ref "not-njson" "key")) (lambda args #t))
    =>
    #t
  ) ;check
) ;define

;;; ========== 非 ASCII 结构转换测试 ==========

;; object->alist with non-ASCII keys and values

(define (test-njson-object->alist-non-ascii)
  (let ((j (string->njson "{\"名前\":\"太郎\",\"住所\":\"東京都\"}")))
    (let ((alist (njson-object->alist j)))
      (check (assoc "名前" alist) => '("名前" . "太郎"))
      (check (assoc "住所" alist) => '("住所" . "東京都"))
    ) ;let
    (njson-free j)
  ) ;let
) ;define

;; array->list with non-ASCII strings

(define (test-njson-array->list-non-ascii)
  (let ((j (string->njson "[\"α\",\"β\",\"γ\"]")))
    (let ((lst (njson-array->list j)))
      (check lst => '("α" "β" "γ"))
    ) ;let
    (njson-free j)
  ) ;let
) ;define

;; njson-keys with non-ASCII keys

(define (test-njson-keys-non-ascii)
  (let ((j (string->njson "{\"颜色\":\"红\",\"大小\":\"大\"}")))
    (let ((ks (njson-keys j)))
      (check (pair? (member "颜色" ks)) => #t)
      (check (pair? (member "大小" ks)) => #t)
      (check (length ks) => 2)
    ) ;let
    (njson-free j)
  ) ;let
) ;define

;;; ========== 边界情况测试 ==========

;; empty object

(define (test-njson-empty-object)
  (let ((j (string->njson "{}")))
    (check (njson-empty? j) => #t)
    (check (njson-size j) => 0)
    (check (njson-keys j) => '())
    (njson-free j)
  ) ;let
) ;define

;; empty array

(define (test-njson-empty-array)
  (let ((j (string->njson "[]")))
    (check (njson-empty? j) => #t)
    (check (njson-size j) => 0)
    (check (njson-array->list j) => '())
    (njson-free j)
  ) ;let
) ;define

;; deep nesting

(define (test-njson-deep-nesting)
  (let ((j (string->njson "{\"a\":{\"b\":{\"c\":{\"d\":\"deep\"}}}}")))
    (check (njson-ref j "a" "b" "c" "d") => "deep")
    (njson-free j)
  ) ;let
) ;define

;;; ========== njson-format-string 测试 ==========

;; njson-format-string with malformed JSON

(define (test-njson-format-string-malformed)
  (check (catch #t (lambda () (njson-format-string "{bad json")) (lambda args #t))
    =>
    #t
  ) ;check
) ;define

;; njson-format-string with non-ASCII

(define (test-njson-format-string-non-ascii)
  (let ((result (njson-format-string "{\"greeting\":\"こんにちは\"}")))
    (check (string? result) => #t)
    (check (string-contains? result "こんにちは") => #t)
  ) ;let
) ;define

;;; ========== 测试入口 ==========

(tm-define (test_0251)
  ;; 非 ASCII 测试
  (test-njson-non-ascii-value)
  (test-njson-non-ascii-key)
  (test-njson-non-ascii-roundtrip)
  ;; 畸形 JSON 测试
  (test-njson-malformed-missing-brace)
  (test-njson-malformed-trailing-comma)
  (test-njson-malformed-single-quotes)
  (test-njson-malformed-empty)
  (test-njson-malformed-plain-text)
  (test-njson-malformed-truncated-array)
  ;; 无效操作测试
  (test-njson-ref-missing-key)
  (test-njson-ref-array-string-key)
  (test-njson-ref-out-of-bounds)
  (test-njson-set-on-scalar)
  (test-njson-append-on-object)
  (test-njson-object->alist-on-array)
  (test-njson-array->list-on-object)
  ;; 类型错误测试
  (test-njson-string-to-json-type-error)
  (test-njson-ref-type-error)
  ;; 非 ASCII 结构转换测试
  (test-njson-object->alist-non-ascii)
  (test-njson-array->list-non-ascii)
  (test-njson-keys-non-ascii)
  ;; 边界情况测试
  (test-njson-empty-object)
  (test-njson-empty-array)
  (test-njson-deep-nesting)
  ;; format-string 测试
  (test-njson-format-string-malformed)
  (test-njson-format-string-non-ascii)
  (check-report)
) ;tm-define
