;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6107.scm
;; DESCRIPTION : Test embedded image context and pasted image structures
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))
(load "./TeXmacs/progs/generic/embedded-edit.scm")

(define (test_6107)
  (check-set-mode! 'report-failed)

  ;; 1. 验证嵌入式图片上下文判定
  (let* ((embedded-stree '(image (tuple (raw-data "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJ")
                                   "png")
                            "0.8par"
                            "0.119533par"
                            ""
                            "")
         ) ;embedded-stree
         (embedded-t (stree->tree embedded-stree))
        ) ;
    (check (embedded-image-context? embedded-t) => #t)
    (check (linked-image-context? embedded-t) => #f)
    (check (embedded-suffix embedded-t) => "png")
  ) ;let*

  ;; 2. 验证网络链接式图片上下文判定（粘贴前的形态）
  (let* ((feishu-url "https://h02wf0jq8yp.feishu.cn/space/api/box/stream/download/asynccode/?code=test&scene_type=CCM"
         ) ;feishu-url
         (linked-stree `(image ,feishu-url ,"0.6383w" ,"" ,"" ,""))
         (linked-t (stree->tree linked-stree))
        ) ;
    (check (linked-image-context? linked-t) => #t)
    (check (embedded-image-context? linked-t) => #f)
  ) ;let*

  ;; 3. 验证 HTML snippet 中 <img> 转换出来的结构
  (let* ((html-img "<img src=\"https://example.com/photo.png\">")
         (converted (convert html-img "html-snippet" "texmacs-stree"))
        ) ;
    (check (and (list? converted) (eq? (car converted) 'image)) => #t)
    (check (cadr converted) => "https://example.com/photo.png")
  ) ;let*

  ;; 4. 验证居中排版包裹结构（空行上粘贴图片时包 with par-mode center）
  (let* ((img '(image (tuple (raw-data "dummy") "png") "0.8par" "0.12par" "" ""))
         (wrapped `(with ,"par-mode" ,"center" ,img))
         (wrapped-t (stree->tree wrapped))
        ) ;
    (check (tree-is? wrapped-t 'with) => #t)
    (check (tree->stree (tree-ref wrapped-t 0)) => "par-mode")
    (check (tree->stree (tree-ref wrapped-t 1)) => "center")
    (check (embedded-image-context? (tree-ref wrapped-t 2)) => #t)
  ) ;let*

  (check-report)
) ;define
