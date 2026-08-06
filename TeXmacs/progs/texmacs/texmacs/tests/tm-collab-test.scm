;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-collab-test.scm
;; DESCRIPTION : 纯逻辑单元测试：协作文档显示名的校验（collab-valid-doc-name?）、
;;               /docs 交替列表解构（collab-docs-pairs）与菜单标签
;;               （collab-doc-label）。规则与服务端 tools/loro-server/validate.js
;;               保持一致。不弹任何 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026  Jim Zhou
;;
;; USAGE
;;   xmake b stem
;;   xmake r tm-collab-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check) (liii unicode))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/texmacs/texmacs/tm-collab.scm")

;; collab-valid-doc-name?：合法输入——CJK、空格、连字符、下划线、边界长度
;; （长度按字符计：utf8-string-length，与服务端 code point 计数一致）。

(define (test-valid-names)
  (check (collab-valid-doc-name? "文档") => #t)
  (check (collab-valid-doc-name? "会议纪要 2026-08") => #t)
  (check (collab-valid-doc-name? "my doc_v2-final") => #t)
  (check (collab-valid-doc-name? "a") => #t)
  (check (collab-valid-doc-name? (make-string 64 #\a)) => #t)
  (check (collab-valid-doc-name? (utf8-make-string 64 #\中)) => #t)
) ;define

;; collab-valid-doc-name?：非法输入——空、超长、禁字符、控制字符、非字符串。

(define (test-invalid-names)
  (check (collab-valid-doc-name? "") => #f)
  (check (collab-valid-doc-name? (make-string 65 #\a)) => #f)
  (check (collab-valid-doc-name? (utf8-make-string 65 #\中)) => #f)
  (for-each
    (lambda (bad) (check (collab-valid-doc-name? (string-append "a" bad "b")) => #f))
    (list "\\" "/" ":" "*" "?" "\"" "<" ">" "|" "\t" (string (integer->char 127)))
  ) ;for-each
  (check (collab-valid-doc-name? 42) => #f)
) ;define

;; collab-docs-pairs：扁平交替列表 → (uuid . name) 对。

(define (test-docs-pairs)
  (check (collab-docs-pairs '()) => '())
  (check (collab-docs-pairs '("u1" "名字" "u2" ""))
    => '(("u1" . "名字") ("u2" . "")))
  ;; 防御奇数长度：末尾落单的忽略
  (check (collab-docs-pairs '("u1" "n1" "u3")) => '(("u1" . "n1")))
) ;define

;; collab-doc-label：有名 → "name (uuid前8位)"；无名/空名 → uuid 全文。

(define (test-doc-label)
  (check (collab-doc-label "abcd1234-0000-0000-0000-000000000000" "周报")
    => "周报 (abcd1234)")
  (check (collab-doc-label "abcd1234-0000-0000-0000-000000000000" "")
    => "abcd1234-0000-0000-0000-000000000000")
  ;; 防御：uuid 不足 8 位不越界
  (check (collab-doc-label "abc" "n") => "n (abc)")
) ;define

(tm-define (regtest-tm-collab)
  (test-valid-names)
  (test-invalid-names)
  (test-docs-pairs)
  (test-doc-label)
  (check-report)
) ;tm-define
