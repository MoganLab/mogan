;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6205.scm
;; DESCRIPTION : Test reasoning fold/unfold rendering with arrows in llm package
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/generic/document-edit.scm")
(import (liii check))

(define (test_6205)
  (check-set-mode! 'report-failed)

  (let* ((fixture (url-append (get-texmacs-path) "tests/stem/6205.stem")))
    (load-buffer fixture)
    (let* ((doc (buffer-get-body (current-buffer))) (explain-node (tree-ref doc 0)))
      ;; 初始为折叠态
      (check (tree-label explain-node) => 'folded-explain)

      ;; 点击标题展开为 unfolded-explain
      (let ((title-node (tree-ref explain-node 0)))
        (mouse-unfold title-node)
        (check (tree-label explain-node) => 'unfolded-explain)

        ;; 再次点击折叠为 folded-explain
        (mouse-fold title-node)
        (check (tree-label explain-node) => 'folded-explain)
      ) ;let
    ) ;let*
  ) ;let*

  (check-report)
) ;define
