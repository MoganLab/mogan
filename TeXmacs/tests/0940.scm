;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0940.scm
;; DESCRIPTION : Test clipboard image detection for magic paste（魔法粘贴图像判定）
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;; 验证 clipboard-tree-image? 对四种剪贴板形态的判定：
;; 1. extern 包装的图像 snippet（外部程序复制，经 clipboard-get-data 解包后）
;; 2. 内部复制的 (document (image ...))
;; 3. (with ... (image ...)) 形态
;; 4. extern 文本（非图像）
;;
;; 运行：MOGAN_TEST_GUI=1 xmake r 0940
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. Details see LICENSE.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (generic generic-edit))
(import (liii check))

(tm-define (test_0940)
  ;; extern 包装的图像：经 clipboard-get-data 解包后得到 image 树
  (let ((extern-img (stree->tree '(extern "(image (url \"/tmp/a.png\"))"))))
    (check (clipboard-tree-image? (parse-texmacs-snippet (tree->string extern-img))) => #t)
  ) ;let

  ;; 内部复制：(document (image ...)) 复合树
  (check (clipboard-tree-image? (stree->tree '(document (image (url "/tmp/a.png"))))) => #t)

  ;; (with ... (image ...)) 形态
  (check (clipboard-tree-image?
           (stree->tree '(with "mode" "x" (image (url "/tmp/a.png"))))) => #t)

  ;; extern 文本：非图像
  (let ((extern-text (stree->tree '(extern "hello"))))
    (check (clipboard-tree-image? (parse-texmacs-snippet (tree->string extern-text))) => #f)
  ) ;let

  ;; GUI 模式不自动 quit，延迟一拍退出（让 check 输出 flush 后关窗）
  (exec-delayed-at (lambda () (quit-TeXmacs)) (+ (texmacs-time) 500))
) ;tm-define
