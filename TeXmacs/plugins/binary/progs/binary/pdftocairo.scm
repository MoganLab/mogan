
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : pdftocairo.scm
;; DESCRIPTION : pdftocairo Binary plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary pdftocairo)
  (:use (binary common)
        (convert images image-format)))

(define (pdftocairo-binary-candidates)
  (cond ((os-macos?)
         (list "/opt/homebrew/bin/pdftocairo"
               "/usr/local/bin/pdftocairo"))
        ((os-win32?)
         (list ))
        (else
         (list "/usr/bin/pdftocairo"))))

(tm-define (find-binary-pdftocairo)
  (:synopsis "Find the url to the pdftocairo binary, return (url-none) if not found")
  (find-binary (pdftocairo-binary-candidates) "pdftocairo"))

(tm-define (has-binary-pdftocairo?)
  (not (url-none? (find-binary-pdftocairo))))

(tm-define (version-binary-pdftocairo)
  (version-binary (find-binary-pdftocairo)))

(tm-define (pdf-file->pdftocairo-raster x opts)
  (let* ((dest (assoc-ref opts 'dest))
         (fullname (url-concretize dest))
         (suffix (url-suffix fullname))
         (fm* (format-from-suffix suffix))
         (fm (if (== fm* "tif") "tiff" fm*))
         (transp (if (== fm "png") "-transp " ""))
         (name (string-drop-right fullname (+ 1 (string-length suffix))))
         (res (get-raster-resolution opts))
         (cmd (url->system (find-binary-pdftocairo))))
    ;;(display (string-append cmd " -singlefile " transp "-" fm " -r " res " " x " "  name))
    (system-2 (string-append cmd " -singlefile " transp "-" fm " -r " res)
              x name)
    (if (url-exists? dest) dest #f)))
