;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tree-test.scm
;; DESCRIPTION : 纯逻辑单元测试：kernel 树工具函数（tree-search 等）。
;;               无 GUI，headless 可跑。
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; USAGE
;;   xmake b stem
;;   xmake r tree-test
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/kernel/library/tree.scm")

;; 复合树的 tree->string 为空串，取第一个子节点的字符串来区分匹配项。

(define (first-arg-string t)
  (tree->string (tree-ref t 0))
) ;define

(define (frac? t)
  (tree-func? t 'frac)
) ;define

(define (make-doc)
  (tm->tree '(document (frac "a" "b") "x" (frac "c" "d")))
) ;define

;; 基本匹配：按 label 搜索，命中所有匹配子树，跳过不匹配的中间节点。

(define (test-tree-search-basic)
  (with r
    (tree-search (make-doc) frac?)
    (check (length r) => 2)
    (check (map tree-label r) => '(frac frac))
    (check (map first-arg-string r) => '("a" "c"))
  ) ;with
) ;define

;; 顺序契约：结果按文档顺序（先序遍历，子节点从左到右），含跨层嵌套。

(define (test-tree-search-document-order)
  (with t
    (tm->tree '(document (section "s" (frac "1" "x")) (frac "2" "y")))
    (check (map first-arg-string (tree-search t frac?)) => '("1" "2"))
  ) ;with
) ;define

;; 嵌套匹配：外层和内层同时命中时两者都返回，外层在前。
;; 外层 frac 的首子是复合树（tree->string 为空串），借此区分内外层。

(define (test-tree-search-nested)
  (with t
    (tm->tree '(document (frac (frac "i" "x") "o")))
    (check (map first-arg-string (tree-search t frac?)) => '("" "i"))
  ) ;with
) ;define

;; 根匹配：根节点自身满足谓词时排在首位，且返回的就是原树对象本身。

(define (test-tree-search-root-match)
  (let* ((t (make-doc)) (r (tree-search t (lambda (x) (tree-func? x 'document)))))
    (check (length r) => 1)
    (check (eq? (car r) t) => #t)
  ) ;let*
  (let* ((t (tm->tree '(frac "a" "b"))) (r (tree-search t frac?)))
    (check (length r) => 1)
    (check (eq? (car r) t) => #t)
  ) ;let*
) ;define

;; 无匹配返回空列表。

(define (test-tree-search-no-match)
  (check (tree-search (make-doc) (lambda (t) #f)) => '())
  (check (tree-search (make-doc) (lambda (t) (tree-func? t 'table))) => '())
) ;define

;; 原子树输入：只检查根自身，匹配返回单元素列表，否则空列表。

(define (test-tree-search-atomic)
  (let* ((t (string->tree "hello")) (r (tree-search t tree-atomic?)))
    (check (length r) => 1)
    (check (eq? (car r) t) => #t)
  ) ;let*
  (check (tree-search (string->tree "hello") frac?) => '())
) ;define

;; 空复合树（无子节点）输入。

(define (test-tree-search-empty-compound)
  (with t
    (tm->tree '(document))
    (check (tree-search t frac?) => '())
    (check (length (tree-search t (lambda (x) (tree-func? x 'document)))) => 1)
  ) ;with
) ;define

;; 纯函数：不修改输入树；返回新列表，调用方 set-car! 不影响再次搜索的结果。

(define (test-tree-search-pure)
  (let* ((t (make-doc)) (r (tree-search t frac?)))
    (set-car! r 'dummy)
    (check (map first-arg-string (tree-search t frac?)) => '("a" "c"))
    (check (first-arg-string (tree-ref t 0)) => "a")
  ) ;let*
) ;define

(tm-define (regtest-tree)
  (test-tree-search-basic)
  (test-tree-search-document-order)
  (test-tree-search-nested)
  (test-tree-search-root-match)
  (test-tree-search-no-match)
  (test-tree-search-atomic)
  (test-tree-search-empty-compound)
  (test-tree-search-pure)
  (check-report)
) ;tm-define
