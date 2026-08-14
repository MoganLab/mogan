;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1199.scm
;; DESCRIPTION : 回归测试：UTF-8 键名通过 utf8-kbd-map 注册，不改变旧 kbd-map。
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1199))

(import (liii check))
(check-set-mode! 'report-failed)

(define (test-1199-encoding)
  (check (string-convert "￥" "UTF-8" "Cork") => "<#FFE5>")
  (check (string-convert "、" "UTF-8" "Cork") => "<#3001>")
) ;define

(define (test-1199-bindings)
  (kbd-flush-pending)
  (check-true (pair? (kbd-find-key-binding "<#FFE5>")))
  (check-true (pair? (kbd-find-key-binding "<#3001>")))
  (check-true (pair? (kbd-find-key-binding "<#FFE5> tab")))
  (check-true (pair? (kbd-find-key-binding "<#3001> tab")))
) ;define

(define step-delay-ms 300)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((act (cdar rest)))
        (exec-delayed-at (lambda () (act) (loop (cdr rest) (+ (texmacs-time) step-delay-ms)))
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_1199)
  (test-1199-encoding)
  (test-1199-bindings)
  (run-chain (list (cons "new document"
                     (lambda () (new-document) (check (get-env "mode") => "text"))
                   ) ;cons
               (cons "￥ enters math"
                 (lambda () (keyboard-press "<#FFE5>" 0) (check (get-env "mode") => "math"))
               ) ;cons
               (cons "￥ leaves math"
                 (lambda () (keyboard-press "<#FFE5>" 0) (check (get-env "mode") => "text"))
               ) ;cons
               (cons "、 inserts dunhao"
                 (lambda () (keyboard-press "<#3001>" 0) (check (before-cursor) => "<#3001>"))
               ) ;cons
               (cons "、 tab enters hybrid"
                 (lambda ()
                   (keyboard-press "tab" 0)
                   (check-true (inside? 'hybrid))
                   ;; 顿号已被消费，不残留在 hybrid 前
                   (check (before-cursor) => #f)
                 ) ;lambda
               ) ;cons
               (cons "report + quit" (lambda () (check-report) (quit-TeXmacs)))
             ) ;list
  ) ;run-chain
) ;tm-define
