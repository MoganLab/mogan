
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : identify.scm
;; DESCRIPTION : Imagemagick Binary plugin: identify
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary identify) (:use (binary common)))

(define (identify-binary-candidates)
  (cond ((os-macos?) (list "/opt/homebrew/bin/identify" "/usr/local/bin/identify"))
        ((os-windows?)
         (list "C:\\Program Files*\\ImageMagick*\\convert.exe"
           "$USERPROFILE\\scoop\\apps\\imagemagick\\current\\identify.exe"
         ) ;list
        ) ;
        (else (list "/usr/bin/identify"))
  ) ;cond
) ;define

(tm-define (find-binary-identify)
  (:synopsis "Find the url to the identify binary, return (url-none) if not found"
  ) ;:synopsis
  (find-binary (identify-binary-candidates) "identify")
) ;tm-define

(tm-define (has-binary-identify?) (not (url-none? (find-binary-identify))))

(tm-define (version-binary-identify) (version-binary (find-binary-identify)))
