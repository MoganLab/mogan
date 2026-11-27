;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6103.scm
;; DESCRIPTION : Test exporting selection as images (png, jpeg, tif)
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(load "./TeXmacs/progs/convert/images/tmimage.scm")
(import (liii check))

(define (test_6103)
  (check-set-mode! 'report-failed)

  ;; 1. Check format registration
  (check (in? "png" (image-formats)) => #t)
  (check (in? "jpeg" (image-formats)) => #t)
  (check (in? "tif" (image-formats)) => #t)
  (check (native-graphics-export-suffix? "png") => #t)
  (check (native-graphics-export-suffix? "jpeg") => #t)
  (check (native-graphics-export-suffix? "jpg") => #t)
  (check (native-graphics-export-suffix? "tif") => #t)
  (check (native-graphics-export-suffix? "tiff") => #t)

  ;; 2. Create document with 3 lines of content and select all
  (new-buffer)
  (insert "Line 1: 1231241523534632465437")
  (insert-return)
  (insert "Line 2: qweuiriuwqeyurqiuywiryu")
  (insert-return)
  (insert "Line 3: askljdfhkla112412352345")
  (select-all)

  ;; 3. Export selection to png, jpeg, and tif
  (let* ((u-png (url-temp-ext "png"))
         (u-jpg (url-temp-ext "jpeg"))
         (u-tif (url-temp-ext "tif"))
        ) ;

    (export-selection-as-graphics u-png)
    (export-selection-as-graphics u-jpg)
    (export-selection-as-graphics u-tif)

    ;; Verify files are created and non-empty
    (check (url-exists? u-png) => #t)
    (check (> (url-size u-png) 0) => #t)

    (check (url-exists? u-jpg) => #t)
    (check (> (url-size u-jpg) 0) => #t)

    (check (url-exists? u-tif) => #t)
    (check (> (url-size u-tif) 0) => #t)

    ;; Cleanup
    (system-remove u-png)
    (system-remove u-jpg)
    (system-remove u-tif)
  ) ;let*

  ;; 4. Test single-line selection export
  (new-buffer)
  (insert "Single line selection")
  (select-all)

  (let* ((u-png (url-temp-ext "png"))
         (u-jpg (url-temp-ext "jpeg"))
         (u-tif (url-temp-ext "tif"))
        ) ;

    (export-selection-as-graphics u-png)
    (export-selection-as-graphics u-jpg)
    (export-selection-as-graphics u-tif)

    (check (url-exists? u-png) => #t)
    (check (> (url-size u-png) 0) => #t)

    (check (url-exists? u-jpg) => #t)
    (check (> (url-size u-jpg) 0) => #t)

    (check (url-exists? u-tif) => #t)
    (check (> (url-size u-tif) 0) => #t)

    ;; Cleanup
    (system-remove u-png)
    (system-remove u-jpg)
    (system-remove u-tif)
  ) ;let*

  (check-report)
) ;define
