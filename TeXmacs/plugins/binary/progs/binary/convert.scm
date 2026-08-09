
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : convert.scm
;; DESCRIPTION : Imagemagick Binary plugin: convert
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary convert) (:use (binary common)))

(define (convert-binary-candidates)
  (cond ((os-macos?) (list "/opt/homebrew/bin/convert" "/usr/local/bin/convert"))
        ((os-windows?)
         (list "C:\\Program Files*\\ImageMagick*\\convert.exe"
           "$USERPROFILE\\scoop\\apps\\imagemagick\\current\\convert.exe"
         ) ;list
        ) ;
        (else (list "/usr/bin/convert"))
  ) ;cond
) ;define

(tm-define (find-binary-convert)
  (:synopsis "Find the url to the convert binary, return (url-none) if not found")
  (find-binary (convert-binary-candidates) "convert")
) ;tm-define

(tm-define (has-binary-convert?) (not (url-none? (find-binary-convert))))

(tm-define (version-binary-convert) (version-binary (find-binary-convert)))

(tm-define (pdf-file->imagemagick-raster x opts)
  (let* ((dest (assoc-ref opts 'dest)) (res (get-raster-resolution opts)))
    ;; (display (string-append "convert -density " res " " x " "  dest))
    (system-2 (string-append (url->system (find-binary-convert)) " -density " res)
      x
      dest
    ) ;system-2
    ;; NOTE: changing the resolution to 300 (the default) causes a problem
    ;; when converting TeXmacs documents to Html with formulas as images:
    ;; the formulas appear way too large...
    ;; (system-2 (string-append "convert ") x dest)
    (if (url-exists? dest) dest #f)
  ) ;let*
) ;tm-define
