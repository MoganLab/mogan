
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : pdf.scm
;; DESCRIPTION : PDF Image plugin
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;                   2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (image pdf)
  (:use (binary convert)
        (binary gs)
        (binary pdftocairo)))

(converter pdf-file svg-file
  (:require (url-exists-in-path? "pdf2svg"))
  (:shell "pdf2svg" from to))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Convert PDF to other formats via pdftocairo
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(converter pdf-file svg-file
  (:require (has-binary-pdftocairo?))
  (:shell ,(url->system (find-binary-pdftocairo)) "-origpagesizes -nocrop -nocenter -svg" from to))


(converter pdf-file jpeg-file
  (:require (has-binary-pdftocairo?))
  (:function-with-options pdf-file->pdftocairo-raster)
  ;;(:option "texmacs->image:raster-resolution" "300")
  )

(converter pdf-file tif-file
  (:require (has-binary-pdftocairo?))
  (:function-with-options pdf-file->pdftocairo-raster)
  ;;(:option "texmacs->image:raster-resolution" "300")
  )

;;(converter pdf-file postscript-document
;;  (:require (has-pdftocairo?))
;;  (:shell "pdftocairo" "-eps" from to))
;;
;;(converter pdf-file postscript-file
;;  (:require (has-pdftocairo?))
;;  (:shell "pdftocairo" "-eps" from to))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Convert PDF to other formats via ImageMagick
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(converter pdf-file jpeg-file
  (:require (has-binary-convert?))
  (:function-with-options pdf-file->imagemagick-raster)
  ;;(:option "texmacs->image:raster-resolution" "300")
  )
 
(converter pdf-file tif-file
  (:require (has-binary-convert?))
  (:function-with-options pdf-file->imagemagick-raster)
  ;;(:option "texmacs->image:raster-resolution" "300")
  )

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Convert PDF to other formats via Ghostscript
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; 
; (converter pdf-file jpeg-file
;   (:require (has-binary-gs?))
;   (:function-with-options pdf-file->gs-raster))
; 
; (converter pdf-file tif-file
;   (:require (has-binary-gs?))
;   (:function-with-options pdf-file->gs-raster))
; 
; (converter pdf-file postscript-file
;   (:require (has-binary-gs?))
;   (:function-with-options gs-convert))

; pdf -> png (the latter one works)
(converter pdf-file png-file
  (:require (has-binary-convert?))
  (:function-with-options pdf-file->imagemagick-raster))
  
(converter pdf-file png-file
  (:require (has-binary-pdftocairo?))
  (:function-with-options pdf-file->pdftocairo-raster))

(converter pdf-file png-file
  (:require (has-binary-gs?))
  (:function-with-options gs-pdf-to-png))
