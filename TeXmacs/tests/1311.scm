;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1311.scm
;; DESCRIPTION : 测试键盘输入文本与符号实体正常插入，不被错误包裹为 <...>
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1311))

(import (liii check))
(check-set-mode! 'report-failed)

(define (test-input-cases)
  (new-document)
  ;; 1. 前缀字符 + 中文省略号：A…… (A<ldots><ldots>) 不应被错误包裹为 <A<ldots><ldots>>
  (keyboard-press "A<ldots><ldots>" 0)
  (check (tree->stree (buffer-tree)) => '(document "A<ldots><ldots>"))

  ;; 2. 纯中文省略号：…… (<ldots><ldots>) 不应被错误包裹为 <<ldots><ldots>>
  (keyboard-press "<ldots><ldots>" 0)
  (check (tree->stree (buffer-tree))
    =>
    '(document "A<ldots><ldots><ldots><ldots>")
  ) ;check

  ;; 3. Emoji 复合符号：😊 (<smiley><#FE0F>) 正常追加插入
  (keyboard-press "<smiley><#FE0F>" 0)
  (check (tree->stree (buffer-tree))
    =>
    '(document "A<ldots><ldots><ldots><ldots><smiley><#FE0F>")
  ) ;check

  ;; 4. 未加尖括号的裸符号实体名：alpha 应被正确包裹为 <alpha>
  (keyboard-press "alpha" 0)
  (check (tree->stree (buffer-tree))
    =>
    '(document "A<ldots><ldots><ldots><ldots><smiley><#FE0F><alpha>")
  ) ;check
) ;define

(tm-define (test_1311) (test-input-cases) (check-report))
