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
  (unix->url "$TEXMACS_PATH/plugins/tutorial/data/zhihu-magic-paste-demo.html")
) ;define

(define tutorial-magic-paste-demo-opened? #f)

(define tutorial-ocr-demo-opened? #f)

(define (tutorial-ocr-demo-document-path)
  (unix->url "$TEXMACS_PATH/plugins/tutorial/data/ocr-demo.tmu")
) ;define

(define (tutorial-ocr-demo-image-path)
  (unix->url "$TEXMACS_PATH/misc/images/tutorial/stem-image.png")
) ;define

(tm-define (tutorial-notify-action action)
  (cpp-set-preference "tutorial:last-action" action)
) ;tm-define

(tm-define (tutorial-prepare-magic-paste-demo)
  (let* ((html-path (tutorial-magic-paste-demo-path))
         (html (string-load html-path))
         (old-export (clipboard-get-export))
        ) ;
    (if (not tutorial-magic-paste-demo-opened?)
      (begin
        (new-document)
        (set! tutorial-magic-paste-demo-opened? #t)
      ) ;begin
    ) ;if
    (if (defined? 'qt-clipboard-set-html)
      (qt-clipboard-set-html html)
      (begin
        (clipboard-set-export "verbatim")
        (clipboard-set "primary" html)
        (clipboard-set-export old-export)
      ) ;begin
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (tutorial-prepare-ocr-demo)
  (if (not tutorial-ocr-demo-opened?)
    (begin
      (load-document (tutorial-ocr-demo-document-path))
      (set! tutorial-ocr-demo-opened? #t)
    ) ;begin
  ) ;if
  (graphics-file-to-clipboard (tutorial-ocr-demo-image-path))
) ;tm-define

;; Why: 直接调 kbd-magic-paste 会触发登录/额度校验弹窗，不适合教程演示；
;;      且其 notify-action 写死为 "ocr-paste"，无法匹配魔法粘贴步骤的 "paste"。
;;      这里直接走核心解析逻辑 smart-format-paste，再显式发 "paste" 完成信号。
(tm-define (tutorial-trigger-magic-paste)
  (smart-format-paste)
  (when (defined? 'tutorial-notify-action)
    (tutorial-notify-action "paste")
  ) ;when
) ;tm-define

(tm-define (tutorial-trigger-ocr)
  (ocr-paste)
  (when (defined? 'tutorial-notify-action)
    (tutorial-notify-action "ocr-paste")
  ) ;when
) ;tm-define
