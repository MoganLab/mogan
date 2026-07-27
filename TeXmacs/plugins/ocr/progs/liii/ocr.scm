
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ocr.scm
;; DESCRIPTION : ocr
;; COPYRIGHT   : (C) 2025  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (liii ocr))
(import (liii os))
(import (liii base64))
(import (liii time))
(import (only (srfi srfi-19) current-time time-second))

(define temp-dir (os-temp-dir))

(define (get-image t i bool)
  (let* ((cur-t (tree-ref t i)))
    (cond 
      ((not cur-t) #f)
      ((tree-is? cur-t 'image) (get-image-tuple cur-t 0 bool))
      (else (get-image t (+ i 1) bool)))))

(define (get-image-tuple t i bool)
  (if bool
    (let* ((cur-t (tree-ref t i)))
      (cond 
        ((not cur-t) #f)
        ((tree-is? cur-t 'tuple) (get-image-name cur-t 0))
        (else (get-image-tuple t (+ i 1)))))
    (let* ((cur-t (tree-ref t i)))
      (cond 
        ((not cur-t) #f)
        ((tree-is? cur-t 'tuple) (get-image-data cur-t 0))
        (else (get-image-tuple t (+ i 1)))))))

(define (get-image-name t i)
  (let* ((cur-t (tree-ref t i)))
    (cond 
      ((not cur-t) #f)
      ((not (string=? (tree->string cur-t) "")) (tree->string cur-t))
      (else (get-image-name t (+ i 1))))))

(define (get-image-data t i)
  (let* ((cur-t (tree-ref t i)))
    (cond 
      ((not cur-t) #f)
      ((tree-is? cur-t 'raw-data) (cdr (tree->stree cur-t)))
      (else (get-image-name t (+ i 1))))))

(define (get-image-extension name)
  (let* ((parts (string-split name #\.)))
    (if (> (length parts) 1)
        (last parts)
        name)))

(define (insert-tips)
  (go-to (cursor-path))
  (go-to-next-node)
  (kbd-return)
  (let* ((content (string-load (unix->url "$TEXMACS_PATH/plugins/ocr/data/ocr.md"))))
    (insert `(with "par-mode" "center" (document ,(utf8->cork content))))))

(define (insert-latex-by-cursor)
  (let* ((mode (get-env "mode"))
         (latex-code (if (== mode "math")
                         "E=m*c^2"  ;; 数学模式下返回 E=m*c^2 的 LaTeX
                         (string-load (unix->url "$TEXMACS_PATH/plugins/ocr/data/ocr.tex"))))
         (parsed-latex (parse-latex latex-code))
         (texmacs-latex (latex->texmacs parsed-latex)))
    (insert texmacs-latex)))

#|
ocr-to-latex-by-cursor
将图像识别为LaTeX并在当前光标处插入

语法
----
(ocr-to-latex-by-cursor t)

参数
----
t: tree
图像的tree表示，且该图像并不在文档中

返回值
------
无返回值，有副作用。会在文档中插入LaTeX代码片段。

逻辑
----
1. 光标在数学模式中，会直接插入数学公式
2. 光标在文本模式中，插入图片对应的LaTeX代码片段
|#
(tm-define (ocr-to-latex-by-cursor t)
  (let* ((extension (get-image-extension (get-image t 0 #t)))
         (temp-name (string-append temp-dir "/temp-" (number->string (time-second (current-time))) "." extension))
         (data-list 
           (get-image t 0 #f)))
    (when (and (list? data-list) (not (null? data-list)))
          (let* ((base64-str (car data-list))
                (binary-data (decode-base64 base64-str)))
            (string-save binary-data temp-name)
            (display* "Image has saved to " temp-name "\n"))))
  (insert-latex-by-cursor))

; (get-image-extension (get-image t 0 #t)) 获取文件后缀，创建对应临时文件
; (get-image t 0 #f) 获取 raw-data

(define (insert-latex-by-image t)
  (tree-go-to t :end)
  (kbd-return)
  (insert-latex-by-cursor))

#|
ocr-to-latex-by-image
将图像识别为LaTeX并在图像下方插入

语法
----
(ocr-to-latex-by-image t)

参数
----
t: tree
图像的tree表示且该图像已经在文档中

返回值
------
无返回值，有副作用。会在文档中插入LaTeX代码片段。

逻辑
----
1. 光标在数学模式中，会直接插入数学公式
2. 光标在文本模式中，插入图片对应的LaTeX代码片段
|#
(tm-define (ocr-to-latex-by-image t)
  (let* ((extention (get-image-extension (get-image t 0 #t)))
         (temp-name (string-append temp-dir "/temp-" (number->string (time-second (current-time))) "." extention))
         (data-list (get-image t 0 #f)))
    (when (and (list? data-list) (not (null? data-list)))
          (let* ((base64-str (car data-list))
                (binary-data (decode-base64 base64-str)))
            (string-save binary-data temp-name)
            (display* "Image has saved to " temp-name "\n"))))
  (insert-latex-by-image t))
