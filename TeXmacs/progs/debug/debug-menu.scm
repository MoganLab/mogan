
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : debug-menu.scm
;; DESCRIPTION : the debug menu
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (debug debug-menu))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Memory
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (show-memory-information t)
  (let* ((s (tree->stree t))
         (a `(concat ,s ," [" ,(number->string (texmacs-memory)) ," bytes]"))
        ) ;
    (stree->tree a)
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Guile
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (debug-backtrace-errors?)
  (in? 'backtrace (debug-options))
) ;define
(tm-define (debug-toggle-backtrace-errors)
  (:synopsis "Toggle scheme backtracing of errors")
  (:check-mark "v" debug-backtrace-errors?)
  (if (debug-backtrace-errors?)
    (debug-disable 'backtrace 'debug)
    (debug-enable 'backtrace 'debug)
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; General debugging options
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (debug-toggle s)
  (:check-mark "v" debug-get)
  (debug-set s (not (debug-get s)))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Memory
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind provoke-error-menu (xxx))

(menu-bind debug-menu
  (-> "Execute"
   ("Execute system command" (interactive system))
   ("Evaluate scheme expression" (interactive footer-eval))
  ) ;->
  (-> "Consoles"
   ("Debugging console" (open-debug-console))
   ("Error messages" (open-error-messages))
   ---
   (group "Automatic")
   ("Open on errors" (toggle-preference "open console on errors"))
   ("Open on warnings" (toggle-preference "open console on warnings"))
  ) ;->
  (-> "Status"
   ("Tree" (show-tree))
   ("Box" (show-box))
   ("Path" (show-path))
   ("Cursors" (show-cursor))
   ("Selection" (show-selection))
   ("Focus" (display* "focus: " (get-focus-path) "\n"))
   ("Environment" (show-env))
   ("History" (show-history))
   ("Memory usage" (show-meminfo))
  ) ;->
  (-> "Timings" ("All" (bench-print-all)))
  (-> "Memory"
   ("Show memory usage in the console" (show-meminfo))
   ("Show memory usage in the footer" (set! footer-hook show-memory-information))
   ("Garbage collection" (delayed (:idle 1000) (gc)))
  ) ;->
  (when (debug-get "correct")
    (-> "Mathematics"
     ("Error status report" (math-status-print))
     ("Reset error counters" (math-status-reset))
    ) ;->
  ) ;when
  (-> "Miscellaneous"
   ("Provoke scheme error" (oops))
   ("Provoke C++ error" (cpp-error))
   (-> "Provoke menu error" (link provoke-error-menu))
  ) ;->
  ---
  ((verbatim "auto") (debug-toggle "auto"))
  ((verbatim "verbose") (debug-toggle "verbose"))
  ((verbatim "events") (debug-toggle "events"))
  ((verbatim "std") (debug-toggle "std"))
  ((verbatim "io") (debug-toggle "io"))
  ((verbatim "bench") (debug-toggle "bench"))
  ((verbatim "history") (debug-toggle "history"))
  ((verbatim "qt") (debug-toggle "qt"))
  ((verbatim "qt-widgets") (debug-toggle "qt-widgets"))
  ((verbatim "keyboard") (debug-toggle "keyboard"))
  ((verbatim "packrat") (debug-toggle "packrat"))
  ((verbatim "flatten") (debug-toggle "flatten"))
  ((verbatim "parser") (debug-toggle "parser"))
  ((verbatim "correct") (debug-toggle "correct"))
  ((verbatim "convert") (debug-toggle "convert"))
  ((verbatim "remote") (debug-toggle "remote"))
) ;menu-bind
