
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-maxima.scm
;; DESCRIPTION : Initialize maxima plugin
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (binary maxima))

(define (maxima-serialize lan t)
  (with s (string-drop-right (utf8raw-serialize lan t) 1)
    (cond ((== s "") "0;\n")
          ((in? (string-ref s (- (string-length s) 1)) '(#\; #\$))
           (string-append s "\n"))
          (else (string-append s ";\n")))))

(define (maxima-entry)
  (string-quote
    (if (url-exists? "$TEXMACS_HOME_PATH/plugins/maxima")
      (url->system "$TEXMACS_HOME_PATH/plugins/maxima/lisp/texmacs-maxima.lisp")
      (url->system "$TEXMACS_PATH/plugins/maxima/lisp/texmacs-maxima.lisp"))))

(define (maxima-launchers)
  (if (os-win32?)
      `((:launch ,(string-append "cmd.exe /c " (url->system (find-binary-maxima)) " -p " (maxima-entry))))
      `((:launch ,(string-append (url->system (find-binary-maxima)) " -p " (maxima-entry))))))

(when (and (has-binary-maxima?) (string-starts? (url->system (find-binary-maxima)) "/opt/homebrew/bin"))
  (plugin-add-macos-path "gnuplot" "/opt/homebrew/bin" #t))

(when (and (has-binary-maxima?) (string-starts? (url->system (find-binary-maxima)) "/usr/local/bin"))
  (plugin-add-macos-path "gnuplot" "/usr/local/bin" #t))

(plugin-configure maxima
  (:require (has-binary-maxima?))
  ,@(maxima-launchers)
  (:serializer ,maxima-serialize)
  (:session "Maxima")
  (:scripts "Maxima"))

(when (supports-maxima?)
  (import-from (maxima-kbd))
  (import-from (maxima-menus))
  (lazy-input-converter (maxima-input) maxima)
  (plugin-approx-command-set! "maxima" "float")

  (kbd-map
    (:mode in-maxima?)
    (:mode in-math?)
    ("$" "$")))
