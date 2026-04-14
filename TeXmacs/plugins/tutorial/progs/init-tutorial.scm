;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-tutorial.scm
;; DESCRIPTION : tutorial plugin entrypoint
;; COPYRIGHT   : (C) 2026
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (tutorial-magic-paste-demo-path)
  (unix->url "$TEXMACS_PATH/plugins/tutorial/data/zhihu-magic-paste-demo.html"))

(define tutorial-magic-paste-demo-opened? #f)
(define tutorial-ocr-demo-opened? #f)

(define (tutorial-ocr-demo-document-path)
  (unix->url "$TEXMACS_PATH/plugins/tutorial/data/ocr-demo.tmu"))

(define (tutorial-ocr-demo-image-path)
  (unix->url "$TEXMACS_PATH/misc/images/tutorial/stem-image.png"))

(tm-define (tutorial-notify-action action)
  (cpp-set-preference "tutorial:last-action" action))

(tm-define (tutorial-prepare-magic-paste-demo)
  (let* ((html-path (tutorial-magic-paste-demo-path))
         (html      (utf8->cork (string-load html-path)))
         (old-export (clipboard-get-export)))
    (if (not tutorial-magic-paste-demo-opened?)
        (begin
          (new-document)
          (set! tutorial-magic-paste-demo-opened? #t)))
    (clipboard-set-export "verbatim")
    (clipboard-set "primary" html)
    (clipboard-set-export old-export)))

(tm-define (tutorial-prepare-ocr-demo)
  (if (not tutorial-ocr-demo-opened?)
      (begin
        (load-document (tutorial-ocr-demo-document-path))
        (set! tutorial-ocr-demo-opened? #t)))
  (graphics-file-to-clipboard (tutorial-ocr-demo-image-path)))
