;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; tree-insert-node 测试用例
;; 用于诊断 Shift+Tab 功能中 tree-insert-node 调用失败的问题
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (test-tree-insert)
  (:use (utils library tree)))

;; 测试1: 基本 tree-insert-node 功能
(tm-define (test-tree-insert-node-basic)
  (display "=== Test 1: Basic tree-insert-node ===")
  (newline)
  (with doc (tree-import '(document (item "a") (item "b") (item "c")))
    (display "Original: ") (display (tree->stree doc)) (newline)
    (with new-doc (tree-insert-node doc 1 '(item "inserted"))
      (display "After insert at 1: ") (display (tree->stree new-doc)) (newline)
      (if (== (tree-arity new-doc) 4)
          (display "PASS: arity is 4")
          (display "FAIL: arity should be 4"))
      (newline))))

;; 测试2: 使用 stale tree 引用
(tm-define (test-tree-insert-node-stale)
  (display "=== Test 2: Using stale tree reference ===")
  (newline)
  (with doc (tree-import '(document (item "a") (item "b") (item "c")))
    (display "Original: ") (display (tree->stree doc)) (newline)
    ;; 保存引用
    (with saved-ref doc
      ;; 修改 doc（移除一个元素）
      (tree-remove! doc 0 1)
      (display "After removal: ") (display (tree->stree doc)) (newline)
      (display "Saved ref stree: ") (display (tree->stree saved-ref)) (newline)
      ;; 尝试使用 stale 引用
      (display "Trying to insert using saved ref...")
      (newline)
      (catch #t
        (lambda ()
          (with new-doc (tree-insert-node saved-ref 0 '(item "inserted"))
            (display "After insert: ") (display (tree->stree new-doc)) (newline)))
        (lambda (key . args)
          (display "ERROR: ") (display key) (display " ") (display args) (newline))))))

;; 测试3: 嵌套结构中的 tree-insert-node
(tm-define (test-tree-insert-node-nested)
  (display "=== Test 3: Nested structure ===")
  (newline)
  ;; 模拟嵌套列表结构
  (with doc (tree-import '(document
                            (item "parent1")
                            (enumerate (document (item "child1") (item "child2")))
                            (item "parent2"))))
    (display "Original: ") (display (tree->stree doc)) (newline)
    ;; 获取子列表
    (with sublist (tree-ref doc 1)
      (display "Sublist: ") (display (tree->stree sublist)) (newline)
      ;; 在子列表后插入
      (catch #t
        (lambda ()
          (with new-doc (tree-insert-node doc 2 '(item "inserted"))
            (display "After insert at 2: ") (display (tree->stree new-doc)) (newline)))
        (lambda (key . args)
          (display "ERROR: ") (display key) (display " ") (display args) (newline))))))

;; 测试4: 移除后再插入（模拟 Shift+Tab 场景）
(tm-define (test-tree-insert-node-after-remove)
  (display "=== Test 4: Remove then insert (Shift+Tab scenario) ===")
  (newline)
  ;; 创建嵌套结构：document > enumerate > document > items
  (with inner-doc (tree-import '(document (item "a") (item "b") (item "c")))
    (with enum (tree-import `(enumerate ,(tree->stree inner-doc)))
      (with parent-doc (tree-import `(document (item "before") ,(tree->stree enum) (item "after")))
        (display "Original parent-doc: ") (display (tree->stree parent-doc)) (newline)
        (display "Original enum: ") (display (tree->stree enum)) (newline)
        (display "Original inner-doc: ") (display (tree->stree inner-doc)) (newline)

        ;; 从 inner-doc 移除元素
        (tree-remove! inner-doc 1 1)  ; 移除 "b"
        (display "After removing 'b' from inner-doc: ") (display (tree->stree inner-doc)) (newline)
        (display "enum stree now: ") (display (tree->stree enum)) (newline)
        (display "parent-doc stree now: ") (display (tree->stree parent-doc)) (newline)

        ;; 尝试在 parent-doc 中插入
        (display "Trying to insert into parent-doc...") (newline)
        (catch #t
          (lambda ()
            (with new-parent (tree-insert-node parent-doc 2 '(item "inserted"))
              (display "After insert: ") (display (tree->stree new-parent)) (newline)))
          (lambda (key . args)
            (display "ERROR: ") (display key) (display " ") (display args) (newline)))))))

;; 测试5: 使用 stree 而不是 tree 引用
(tm-define (test-tree-insert-node-with-stree)
  (display "=== Test 5: Using stree instead of tree reference ===")
  (newline)
  (with doc (tree-import '(document (item "a") (item "b") (item "c")))
    (display "Original: ") (display (tree->stree doc)) (newline)
    ;; 保存为 stree
    (with doc-stree (tree->stree doc)
      (display "Saved stree: ") (display doc-stree) (newline)
      ;; 修改 doc
      (tree-remove! doc 0 1)
      (display "After removal: ") (display (tree->stree doc)) (newline)
      ;; 使用 stree 尝试插入（这应该失败，因为 tree-insert-node 需要 tree 引用）
      (display "Note: tree-insert-node requires a tree reference, not stree")
      (newline))))

;; 运行所有测试
(tm-define (run-all-tree-insert-tests)
  (display "\n========================================\n")
  (display "Running tree-insert-node tests\n")
  (display "========================================\n\n")
  (test-tree-insert-node-basic)
  (newline)
  (test-tree-insert-node-stale)
  (newline)
  (test-tree-insert-node-nested)
  (newline)
  (test-tree-insert-node-after-remove)
  (newline)
  (test-tree-insert-node-with-stree)
  (newline)
  (display "\n========================================\n")
  (display "Tests completed\n")
  (display "========================================\n"))
