;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ocr-clipboard.scm
;; DESCRIPTION : OCR 图片数据提取（剪贴板 / image tree）
;; COPYRIGHT   : (C) 2026  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (ocr ocr-clipboard))

;; clipboard/tree 系列符号属 GPL 层，goldfish 库不可直接调用（协议隔离），
;; 图片数据在本模块提取后以纯字符串（raw bytes）传入 goldfish。

;; 从剪贴板取出 image tree；非图片返回 #f
(tm-define (ocr-clipboard->image-tree)
  (let* ((clip (clipboard-get "primary"))
         (snippet (tree->string (tree-ref clip 1)))
         (data (parse-texmacs-snippet snippet))
        ) ;
    (and data (tree-is? (tree-ref data 0) 'image) (tree-ref data 0))
  ) ;let*
) ;tm-define

;; 从 image tree 提取 raw-data bytes（用于 MD5 与上传）
;; raw-data stree 形如 (raw-data <bytes-string> "png")，取 cadr 得到 bytes 字符串
(tm-define (ocr-image-tree->raw-bytes t)
  (let loop
    ((node t))
    (cond ((not node) #f)
          ((tree-is? node 'raw-data) (cadr (tree->stree node)))
          ((tree? node)
           (let inner
             ((i 0) (n (tree-arity node)))
             (if (>= i n) #f (or (loop (tree-ref node i)) (inner (+ i 1) n)))
           ) ;let
          ) ;
          (else #f)
    ) ;cond
  ) ;let
) ;tm-define

;; 以下自 (liii ocr-image-util) 上移（tree 行走属 GPL 层）

;; 在 image tree 的子节点中定位第一个 'image 节点

(define (locate-image t i)
  (let ((cur-t (tree-ref t i)))
    (cond ((not cur-t) #f)
          ((tree-is? cur-t 'image) cur-t)
          (else (locate-image t (+ i 1)))
    ) ;cond
  ) ;let
) ;define

;; 在某个节点下定位第一个 'tuple 节点

(define (locate-tuple t i)
  (let ((cur-t (tree-ref t i)))
    (cond ((not cur-t) #f)
          ((tree-is? cur-t 'tuple) cur-t)
          (else (locate-tuple t (+ i 1)))
    ) ;cond
  ) ;let
) ;define

;; 在 tuple 子节点中找第一个非空字符串（URL/路径名）

(define (first-non-empty-string t i)
  (let ((cur-t (tree-ref t i)))
    (cond ((not cur-t) #f)
          (else
            (let ((s (tree->string cur-t)))
              (if (string=? s "") (first-non-empty-string t (+ i 1)) s)
            ) ;let
          ) ;else
    ) ;cond
  ) ;let
) ;define

;; 在 tuple 子节点中查找 'raw-data 节点

(define (find-raw-data t i)
  (let ((cur-t (tree-ref t i)))
    (cond ((not cur-t) #f)
          ((tree-is? cur-t 'raw-data) (cdr (tree->stree cur-t)))
          (else (find-raw-data t (+ i 1)))
    ) ;cond
  ) ;let
) ;define

;; 从 image tree 提取 raw-data bytes（找不到则回退到 URL/路径名）
(tm-define (ocr-get-image t i)
  (let ((img (locate-image t i)))
    (and img
      (let ((tup (locate-tuple img 0)))
        (and tup (or (find-raw-data tup 0) (first-non-empty-string tup 0)))
      ) ;let
    ) ;and
  ) ;let
) ;tm-define

;; 从 image tree 提取 URL/路径名
(tm-define (ocr-get-image-by-name t i)
  (let ((img (locate-image t i)))
    (and img
      (let ((tup (locate-tuple img 0)))
        (and tup (first-non-empty-string tup 0))
      ) ;let
    ) ;and
  ) ;let
) ;tm-define

;; 提取可上传的 base64 图片数据；仅 raw-data 形态可用（URL/路径名返回 #f）
(tm-define (ocr-get-image-base64 t)
  (let ((data (ocr-get-image t 0)))
    (if (pair? data) (car data) #f)
  ) ;let
) ;tm-define
