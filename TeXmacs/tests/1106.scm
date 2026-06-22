;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 1106.scm
;; DESCRIPTION : GUI auto-reproduction for tab-switch dirty-state bug (1106)
;; COPYRIGHT   : (C) 2026
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;; PURPOSE
;;   The bug reproduces in the real GUI just by opening two documents and
;;   switching between their tabs. switch-to-buffer in headless does NOT
;;   trigger it. This file drives the *same* code path that tabpage-menu.scm
;;   uses when the user clicks a tab (window-set-view), in a real GUI process,
;;   with a delay between steps so the Qt event loop has time to process tab
;;   rebuild + paint. The [1106] buffer_modified and [1106-qt] applyDisplayTitle
;;   debug logs can then be watched live in the terminal.
;;
;;   Fixtures live under $TEXMACS_PATH/tests/tmu and are copied to /tmp at the
;;   start of every run so save-buffer / edits never mutate the checked-in
;;   copies.
;;
;;   Because exec-delayed-at runs asynchronously, test_1106 schedules the
;;   whole sequence as a chain of delayed tasks and lets the last task call
;;   (quit-TeXmacs) itself; test_1106 returns immediately so the event loop
;;   keeps running and the chain can fire.
;;
;; USAGE
;;   xmake b stem
;;   MOGAN_TEST_GUI=1 xmake r 1106
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs tests 1106))

(define (fixture-name name)
  (string-append "$TEXMACS_PATH/tests/tmu/" name)
) ;define

(define (tmp-name name)
  (string->url (string-append "/tmp/" name))
) ;define

(define (refresh-fixture name)
  (let ((src (string->url (fixture-name name))) (dst (tmp-name name)))
    (when (url-exists? src)
      (system-copy src dst)
    ) ;when
  ) ;let
) ;define

(define (view-for-buffer buf views)
  (cond ((null? views) #f)
        ((== (view->buffer (car views)) buf) (car views))
        (else (view-for-buffer buf (cdr views)))
  ) ;cond
) ;define

(define (switch-to buf)
  (let* ((views (tabpage-list #t)) (v (view-for-buffer buf views)))
    (when v
      (let ((win (view->window-of-tabpage v)))
        (window-set-view win v #t)
      ) ;let
    ) ;when
  ) ;let*
) ;define

(define step-delay-ms 5000)

(define (run-chain steps)
  (let loop
    ((rest steps) (t (+ (texmacs-time) step-delay-ms)))
    (when (pair? rest)
      (let ((label (caar rest)) (act (cdar rest)))
        (exec-delayed-at (lambda ()
                           (display "[1106-step] ")
                           (display label)
                           (newline)
                           (act)
                           (loop (cdr rest) (+ (texmacs-time) step-delay-ms))
                         ) ;lambda
          t
        ) ;exec-delayed-at
      ) ;let
    ) ;when
  ) ;let
) ;define

(tm-define (test_1106)
  ;; Refresh /tmp copies from $TEXMACS_PATH fixtures so the checked-in files
  ;; never get mutated by save-buffer during the run.
  (refresh-fixture "1106_a.tmu")
  (refresh-fixture "1106_b.tmu")
  (let* ((path-a (tmp-name "1106_a.tmu")) (path-b (tmp-name "1106_b.tmu")))
    (let ((steps (list (cons "load a" (lambda () (load-buffer path-a)))
                   (cons "load b" (lambda () (load-buffer path-b)))
                 ) ;list
          ) ;steps
         ) ;
      (let loop
        ((i 0) (acc steps))
        (if (>= i 5)
          (set! steps
            (append acc (list (cons "all done; quitting" (lambda () (quit-TeXmacs)))))
          ) ;set!
          (loop (+ i 1)
            (append acc
              (list (cons (string-append "round " (number->string i) ": switch a")
                      (lambda () (switch-to path-a))
                    ) ;cons
                (cons (string-append "round " (number->string i) ": switch b")
                  (lambda () (switch-to path-b))
                ) ;cons
              ) ;list
            ) ;append
          ) ;loop
        ) ;if
      ) ;let
      (display "[1106-step] starting delayed chain\n")
      (run-chain steps)
    ) ;let
  ) ;let*
) ;tm-define
