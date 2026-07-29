;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-ghost.scm
;; DESCRIPTION : Ghost 自动补全后台常驻进程入口
;; COPYRIGHT   : (C) 2026 AcceleratorX
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (texmacs protocol))
(import (liii path))
(import (liii os))
(import (liii string))

(define (ghost-env-path name suffix)
  (let ((value (getenv name)))
    (if (and (string? value) (not (string-null? value)))
      (path-join value suffix)
      #f
    ) ;if
  ) ;let
) ;define

(define (ghost-cons-existing-path candidate paths)
  (if (and candidate (path-exists? candidate))
      (cons (path->string candidate) paths)
      paths)
) ;define

(define (ghost-setup-load-path)
  (let* ((suffix "plugins/ghost/goldfish")
         (repo-path (path-join (getcwd) "ghost" "goldfish"))
         (user-path (ghost-env-path "TEXMACS_HOME_PATH" suffix))
         (sys-path (ghost-env-path "TEXMACS_PATH" suffix))
        ) ;
    (set! *load-path*
      (ghost-cons-existing-path user-path
        (ghost-cons-existing-path sys-path
          (ghost-cons-existing-path repo-path *load-path*)
        ) ;ghost-cons-existing-path
      ) ;ghost-cons-existing-path
    ) ;set!
  ) ;let*
) ;define

(ghost-setup-load-path)

(import (liii ghost-worker))

;; 与主进程握手
(define (welcome)
  (flush-verbatim "ghost")
) ;define

(define (document->string doc)
  (cond ((and (pair? doc) (eq? (car doc) 'document) (= (length doc) 2)) (cadr doc))
        ((string? doc) doc)
        (else "")
      ) ;cond
) ;define

;; 读取主进程输入并调用 worker 异步处理
(define (loop)
  (let* ((code (read-paragraph-by-visible-eof))
         (payload (document->string code))
        ) ;let*
    (unless (string-null? (string-trim-both payload))
      (ghost-worker-handle payload)
    ) ;unless
    (loop)
  ) ;let*
) ;define

(welcome)
(loop)
