
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-preferences.scm
;; DESCRIPTION : management of the user preferences
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-preferences)
  (:use (kernel texmacs tm-define))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Defining preference call back routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public preferences-default (make-ahash-table))
(define-public preferences-call-back (make-ahash-table))

(define (define-preference x)
  (with (which value call-back)
    x
    `(if (not (ahash-ref preferences-default ,which))
       (ahash-set! preferences-default ,which ,value))
  ) ;with
) ;define

(define (define-preference-call-back x)
  (with (which value call-back)
    x
    `(begin
       (ahash-set! preferences-call-back ,which ,call-back)
       (notify-preference ,which))
  ) ;with
) ;define

(define-public-macro (define-preferences . l)
  (append '(begin)
    (map-in-order define-preference l)
    (map-in-order define-preference-call-back l)
  ) ;append
) ;define-public-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Setting and getting preferences
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-preference? which what)
  (== (get-preference which)
    (if (!= what "default") what (ahash-ref preferences-default which))
  ) ;==
) ;define

(tm-define (set-preference which what)
  (:synopsis "Set preference @which to @what")
  (:check-mark "*" test-preference?)
  ;; (display* "set-preference " which " := " what "\n")
  (with val
    (if (string? what) what (object->string what))
    (when (!= (get-preference which) val)
      (cpp-set-preference which val)
      (notify-preference which)
      (save-preferences)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (reset-preference which)
  (:synopsis "Revert preference @which to default setting")
  ;; (display* "reset-preference " which "\n")
  (when (cpp-has-preference? which)
    (cpp-reset-preference which)
    (notify-preference which)
    (save-preferences)
  ) ;when
) ;tm-define

(define (get-call-back what)
  (let ((r (ahash-ref preferences-call-back what)))
    (if r r (lambda args (noop)))
  ) ;let
) ;define

(tm-define (notify-preference which)
  (:synopsis "Notify that the preference @which was changed")
  ;; (display* "notify-preference " which ", " (get-preference which) "\n")
  ((get-call-back which) which (get-preference which))
) ;tm-define

(tm-define (get-preference which)
  (:synopsis "Get preference @which")
  (let* ((def (or (ahash-ref preferences-default which) "default"))
         (s? (string? def))
         (r (cpp-get-preference which (if s? def (object->string def))))
        ) ;
    (if s? r (string->object r))
  ) ;let*
) ;tm-define

(tm-define (preference-on? which) (test-preference? which "on"))

(tm-define (toggle-preference which)
  (:synopsis "Toggle the preference @which")
  (:check-mark "v" preference-on?)
  (with what
    (get-preference which)
    (set-preference which
      (cond ((== what "on") "off")
            ((== what "off") "on")
            (else what)
      ) ;cond
    ) ;set-preference
  ) ;with
) ;tm-define

(tm-define (append-preference which val)
  (:synopsis "Appends @val to the list of values of preference @which")
  (with cur
    (get-preference which)
    (if (== cur "default") (set! cur '()))
    (set-preference which (rcons cur val))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Nicer names for preference values
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public preference-decode-table (make-ahash-table))
(define-public preference-encode-table (make-ahash-table))

(tm-define (set-pretty-preference which pretty-val)
  (with val
    (ahash-ref preference-decode-table (cons which pretty-val))
    (set-preference which (or val pretty-val))
  ) ;with
) ;tm-define

(tm-define (get-pretty-preference which)
  (with val
    (get-preference which)
    (with pretty-val
      (ahash-ref preference-encode-table (cons which val))
      ;; (display* "Get: " which ", " val " -> " pretty-val "\n")
      (or pretty-val val "Default")
    ) ;with
  ) ;with
) ;tm-define

(tm-define (set-boolean-preference which val)
  (set-preference which (if val "on" "off"))
) ;tm-define

(tm-define (get-boolean-preference which) (== (get-preference which) "on"))

(define-public (set-preference-name which var val)
  (ahash-set! preference-encode-table (cons which var) val)
  (ahash-set! preference-decode-table (cons which val) var)
) ;define-public

(define-public (set-preference-encode which x)
  `(ahash-set! preference-encode-table (cons ,which ,(car x)) ,(cadr x))
) ;define-public

(define-public (set-preference-decode which x)
  `(ahash-set! preference-decode-table (cons ,which ,(cadr x)) ,(car x))
) ;define-public

(define-public-macro (define-preference-names which . l)
  `(begin
     ,@(map (lambda (x) (set-preference-encode which x)) l)
     ,@(map (lambda (x) (set-preference-decode which x)) l))
) ;define-public-macro

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Look and feel
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (xdg-dekstop-session)
  (locase-all (system-getenv "XDG_SESSION_DESKTOP"))
) ;define

(define (default-look-and-feel)
  (cond ((os-macos?) "macos")
        ((or (os-win32?) (os-mingw?)) "windows")
        ((== (xdg-dekstop-session) "kde") "kde")
        ((== (xdg-dekstop-session) "deepin") "kde")
        ((== (xdg-dekstop-session) "gnome") "gnome")
        ((== (xdg-dekstop-session) "ubuntu") "gnome")
        (else "emacs")
  ) ;cond
) ;define

(define-public (look-and-feel)
  (with s
    (get-preference "look and feel")
    (if (== s "default") (default-look-and-feel) s)
  ) ;with
) ;define-public

(define (test-look-and-feel t)
  ;; (display* "Check look and feel " t "\n")
  (cond ((list? t) (list-or (map test-look-and-feel t)))
        ((symbol? t) (test-look-and-feel (symbol->string t)))
        ((and (string? t) (string-starts? t "no-"))
         (not (test-look-and-feel (substring t 3 (string-length t))))
        ) ;
        (else (with s (look-and-feel) (or (== t s) (and (== t "std") (!= s "emacs")))))
  ) ;cond
) ;define

(define-public (use-popups?) (== (get-preference "complex actions") "popups"))

(define-public (use-menus?) (== (get-preference "complex actions") "menus"))

(define-public (use-print-dialog?)
  (and (qt-gui?) (== (get-preference "gui:print dialogue") "on"))
) ;define-public

(set! has-look-and-feel? test-look-and-feel)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Notify that the Scheme preferences system has been started
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(notify-preferences-booted)
