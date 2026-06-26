;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : session-edit-test.scm
;; DESCRIPTION : Regression test for session-edit reasoning support
;; COPYRIGHT   : (C) 2025 Liii Network
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/progs/dynamic/session-edit.scm")

;; test-tree-contains-label
;; 测试 tree-contains-label? 的递归查找能力

(define (test-tree-contains-label)
  (check (tree-contains-label? (stree->tree '(document (reasoning-delta "hello")))
           'reasoning-delta
         ) ;tree-contains-label?
    =>
    #t
  ) ;check
  (check (tree-contains-label? (stree->tree '(document (text "hello"))) 'reasoning-delta)
    =>
    #f
  ) ;check
  (check (tree-contains-label? (stree->tree '(document (concat (reasoning-delta "nested"))))
           'reasoning-delta
         ) ;tree-contains-label?
    =>
    #t
  ) ;check
  (check (tree-contains-label? (stree->tree '(document (fold-explain-reasoning)))
           'fold-explain-reasoning
         ) ;tree-contains-label?
    =>
    #t
  ) ;check
) ;define

;; test-tree-extract-reasoning-delta
;; 测试 tree-extract-reasoning-delta! 提取文本并清除节点

(define (test-tree-extract-reasoning-delta)
  (check (let ((t (stree->tree '(document (reasoning-delta "hello")
                                  (text "world")))))
           (list (tree-extract-reasoning-delta! t) (tree->stree t))
         ) ;let
    =>
    '("hello" (document (text "world")))
  ) ;check
  (check (let ((t (stree->tree '(document (reasoning-delta) (text "x")))))
           (list (tree-extract-reasoning-delta! t) (tree->stree t))
         ) ;let
    =>
    '("" (document (text "x")))
  ) ;check
) ;define

;; test-tree-remove-label
;; 测试 tree-remove-label-from-children! 移除指定 label

(define (test-tree-remove-label)
  (check (let ((t (stree->tree '(document (reasoning-delta "a") (text "b")))))
           (tree-remove-label-from-children! t 'reasoning-delta)
           (tree->stree t)
         ) ;let
    =>
    '(document (text "b"))
  ) ;check
  (check (let ((t (stree->tree '(document (concat (reasoning-delta "a")
                                            (text "b"))))))
           (tree-remove-label-from-children! t 'reasoning-delta)
           (tree->stree t)
         ) ;let
    =>
    '(document (concat (text "b")))
  ) ;check
) ;define

;; test-session-find-last-unfolded-explain
;; 测试 session-find-last-unfolded-explain 向前搜索

(define (test-session-find-last-unfolded-explain)
  (check (tree->stree (session-find-last-unfolded-explain (stree->tree '(document (unfolded-explain (document "a"))))
                        1
                      ) ;session-find-last-unfolded-explain
         ) ;tree->stree
    =>
    '(unfolded-explain (document "a"))
  ) ;check
  (check (tree->stree (session-find-last-unfolded-explain (stree->tree '(document (concat (unfolded-explain (document "b")))))
                        1
                      ) ;session-find-last-unfolded-explain
         ) ;tree->stree
    =>
    '(unfolded-explain (document "b"))
  ) ;check
  (check (session-find-last-unfolded-explain (stree->tree '(document (text "x"))) 1)
    =>
    #f
  ) ;check
) ;define

(tm-define (regtest-session-edit)
  (test-tree-contains-label)
  (test-tree-extract-reasoning-delta)
  (test-tree-remove-label)
  (test-session-find-last-unfolded-explain)
  (check-report)
) ;tm-define
