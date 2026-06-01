;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : session-edit-test.scm
;; DESCRIPTION : Regression test for session-edit reasoning support
;; COPYRIGHT   : (C) 2025 Liii Network
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(load "./TeXmacs/progs/dynamic/session-edit.scm")

;; regtest-tree-contains-label?
;; 测试 tree-contains-label? 的递归查找能力

(define (regtest-tree-contains-label)
  (regression-test-group "test tree-contains-label?"
    "tree-contains-label"
    :none
    :none
    (test "find label in direct child"
      (tree-contains-label? (stree->tree '(document (reasoning-delta "hello")))
        'reasoning-delta
      ) ;tree-contains-label?
      #t
    ) ;test
    (test "not find label"
      (tree-contains-label? (stree->tree '(document (text "hello"))) 'reasoning-delta)
      #f
    ) ;test
    (test "find label in nested concat"
      (tree-contains-label? (stree->tree '(document (concat (reasoning-delta "nested"))))
        'reasoning-delta
      ) ;tree-contains-label?
      #t
    ) ;test
    (test "find fold-explain-reasoning label"
      (tree-contains-label? (stree->tree '(document (fold-explain-reasoning)))
        'fold-explain-reasoning
      ) ;tree-contains-label?
      #t
    ) ;test
  ) ;regression-test-group
) ;define

;; regtest-tree-extract-reasoning-delta
;; 测试 tree-extract-reasoning-delta! 提取文本并清除节点

(define (regtest-tree-extract-reasoning-delta)
  (regression-test-group "test tree-extract-reasoning-delta!"
    "tree-extract-reasoning-delta"
    :none
    :none
    (test "extract single reasoning-delta"
      (let ((t (stree->tree '(document (reasoning-delta "hello") (text "world")))))
        (list (tree-extract-reasoning-delta! t) (tree->stree t))
      ) ;let
      '("hello" (document (text "world")))
    ) ;test
    (test "extract empty reasoning-delta"
      (let ((t (stree->tree '(document (reasoning-delta) (text "x")))))
        (list (tree-extract-reasoning-delta! t) (tree->stree t))
      ) ;let
      '("" (document (text "x")))
    ) ;test
  ) ;regression-test-group
) ;define

;; regtest-tree-remove-label
;; 测试 tree-remove-label-from-children! 移除指定 label

(define (regtest-tree-remove-label)
  (regression-test-group "test tree-remove-label-from-children!"
    "tree-remove-label"
    :none
    :none
    (test "remove direct child"
      (let ((t (stree->tree '(document (reasoning-delta "a") (text "b")))))
        (tree-remove-label-from-children! t 'reasoning-delta)
        (tree->stree t)
      ) ;let
      '(document (text "b"))
    ) ;test
    (test "remove from concat"
      (let ((t (stree->tree '(document (concat (reasoning-delta "a") (text "b"))))))
        (tree-remove-label-from-children! t 'reasoning-delta)
        (tree->stree t)
      ) ;let
      '(document (concat (text "b")))
    ) ;test
  ) ;regression-test-group
) ;define

;; regtest-session-find-last-unfolded-explain
;; 测试 session-find-last-unfolded-explain 向前搜索

(define (regtest-session-find-last-unfolded-explain)
  (regression-test-group "test session-find-last-unfolded-explain"
    "session-find-last-unfolded-explain"
    :none
    :none
    (test "find direct child"
      (tree->stree (session-find-last-unfolded-explain (stree->tree '(document (unfolded-explain (document "a"))))
                     1
                   ) ;session-find-last-unfolded-explain
      ) ;tree->stree
      '(unfolded-explain (document "a"))
    ) ;test
    (test "find in concat"
      (tree->stree (session-find-last-unfolded-explain (stree->tree '(document (concat (unfolded-explain (document "b")))))
                     1
                   ) ;session-find-last-unfolded-explain
      ) ;tree->stree
      '(unfolded-explain (document "b"))
    ) ;test
    (test "not found"
      (session-find-last-unfolded-explain (stree->tree '(document (text "x"))) 1)
      #f
    ) ;test
  ) ;regression-test-group
) ;define

(tm-define (regtest-session-edit)
  (let ((n (+ (regtest-tree-contains-label)
             (regtest-tree-extract-reasoning-delta)
             (regtest-tree-remove-label)
             (regtest-session-find-last-unfolded-explain)
           ) ;+
        ) ;n
       ) ;
    (display* "Total: " (object->string n) " tests.\n")
    n
  ) ;let
) ;tm-define
