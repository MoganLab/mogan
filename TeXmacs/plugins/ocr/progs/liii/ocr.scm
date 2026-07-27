
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
  (if (tree-is? t 'image)
    (get-image-tuple t 0 bool)
    (let* ((cur-t (tree-ref t i)))
      (cond ((not cur-t) #f)
            ((tree-is? cur-t 'image) (get-image-tuple cur-t 0 bool))
            (else (get-image t (+ i 1) bool))
      ) ;cond
    ) ;let*
  ) ;if
) ;define

(define (get-image-tuple t i bool)
  (if bool
    (let* ((cur-t (tree-ref t i)))
      (cond ((not cur-t) #f)
            ((tree-is? cur-t 'tuple) (get-image-name cur-t 0))
            (else (get-image-tuple t (+ i 1)))
      ) ;cond
    ) ;let*
    (let* ((cur-t (tree-ref t i)))
      (cond ((not cur-t) #f)
            ((tree-is? cur-t 'tuple) (get-image-data cur-t 0))
            (else (get-image-tuple t (+ i 1)))
      ) ;cond
    ) ;let*
  ) ;if
) ;define

(define (get-image-name t i)
  (let* ((cur-t (tree-ref t i)))
    (cond ((not cur-t) #f)
          ((not (string=? (tree->string cur-t) "")) (tree->string cur-t))
          (else (get-image-name t (+ i 1)))
    ) ;cond
  ) ;let*
) ;define

(define (get-image-data t i)
  (let* ((cur-t (tree-ref t i)))
    (cond ((not cur-t) #f)
          ((tree-is? cur-t 'raw-data) (cdr (tree->stree cur-t)))
          (else (get-image-name t (+ i 1)))
    ) ;cond
  ) ;let*
) ;define

(define (get-image-extension name)
  (let* ((parts (string-split name #\.)))
    (if (> (length parts) 1) (last parts) name)
  ) ;let*
) ;define

(define (insert-tips)
  (go-to (cursor-path))
  (go-to-next-node)
  (kbd-return)
  (let* ((content (string-load (unix->url "$TEXMACS_PATH/plugins/ocr/data/ocr.md"))))
    (insert `(with ,"par-mode" ,"center" (document ,(utf8->cork content))))
  ) ;let*
) ;define

(define (insert-latex-by-cursor)
  (let* ((mode (get-env "mode"))
         (latex-code (if (== mode "math")
                       "E=m*c^2"
                       ;; 数学模式下返回 E=m*c^2 的 LaTeX
                       (string-load (unix->url "$TEXMACS_PATH/plugins/ocr/data/ocr.tex"))
                     ) ;if
         ) ;latex-code
         (parsed-latex (parse-latex latex-code))
         (texmacs-latex (latex->texmacs parsed-latex))
        ) ;
    (insert texmacs-latex)
  ) ;let*
) ;define


(tm-define (ocr-to-latex-by-cursor t)
  (let* ((extension (get-image-extension (get-image t 0 #t)))
         (temp-name (string-append temp-dir
                      "/temp-"
                      (number->string (time-second (current-time)))
                      "."
                      extension
                    ) ;string-append
         ) ;temp-name
         (data-list (get-image t 0 #f))
        ) ;
    (when (and (list? data-list) (not (null? data-list)))
      (let* ((base64-str (car data-list)) (binary-data (decode-base64 base64-str)))
        (string-save binary-data temp-name)
        (display* "Image has saved to " temp-name "\n")
      ) ;let*
    ) ;when
  ) ;let*
  (insert-latex-by-cursor)
) ;tm-define


(define (insert-latex-by-image t)
  (tree-go-to t :end)
  (kbd-return)
  (insert-latex-by-cursor)
) ;define


(tm-define (ocr-to-latex-by-image t)
  (let* ((extention (get-image-extension (get-image t 0 #t)))
         (temp-name (string-append temp-dir
                      "/temp-"
                      (number->string (time-second (current-time)))
                      "."
                      extention
                    ) ;string-append
         ) ;temp-name
         (data-list (get-image t 0 #f))
        ) ;
    (when (and (list? data-list) (not (null? data-list)))
      (let* ((base64-str (car data-list)) (binary-data (decode-base64 base64-str)))
        (string-save binary-data temp-name)
        (display* "Image has saved to " temp-name "\n")
      ) ;let*
    ) ;when
  ) ;let*
  (insert-latex-by-image t)
) ;tm-define
