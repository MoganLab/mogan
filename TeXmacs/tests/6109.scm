;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 6109.scm
;; DESCRIPTION : Test exporting graphics (draw image) to PNG format
;; COPYRIGHT   : (C) 2026 Mogan STEM
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (graphics graphics-utils))
(use-modules (graphics graphics-main))
(import (liii check))

(define (test_6109)
  (check-set-mode! 'report-failed)

  ;; 1. Check in a newly created buffer with graphics object
  (new-buffer)
  (insert-go-to
    '(with "gr-mode"
       "point"
       "gr-frame"
       (tuple "scale" "1cm" (tuple "0.5gw" "0.5gh"))
       "gr-geometry"
       (tuple "geometry" "1par" "0.6par")
       "gr-grid"
       (tuple "cartesian" (point "0" "0") "1")
       (graphics (point "0" "0") (line (point "-1" "-1") (point "1" "1"))))
    '(8 0)
  ) ;insert-go-to

  ;; Verify inside graphics mode
  (check (in-graphics?) => #t)

  ;; Verify graphics-tree-to-export finds the surrounding with tree
  (let ((gt (graphics-tree-to-export)))
    (check (tree? gt) => #t)
    (check (tree-is? gt 'with) => #t)
    (check
      (tree-is? (tree-ref gt (- (tree-arity gt) 1)) 'graphics)
      =>
      #t
    ) ;check
  ) ;let

  ;; 2. Test exporting graphics to PNG file
  (let* ((u-png (url-glue (url-temp) ".png")))
    (graphics-export-png u-png)
    (check (url-exists? u-png) => #t)
    (check (> (url-size u-png) 0) => #t)

    ;; Verify file starts with PNG magic bytes (\x89 P N G)
    (let ((header (substring (string-load u-png) 0 4)))
      (check (string-starts? header "‰PNG") => #t)
    ) ;let

    (system-remove u-png)
  ) ;let*

  ;; 3. Test exporting graphics with no extension (should auto-append .png)
  (let* ((u-no-ext (url-temp)) (u-expected (url-glue u-no-ext ".png")))
    (graphics-export-png u-no-ext)
    (check (url-exists? u-expected) => #t)
    (check (> (url-size u-expected) 0) => #t)
    (system-remove u-expected)
  ) ;let*

  ;; 4. Test exporting graphics with auto-crop
  (new-buffer)
  (insert-go-to
    '(with "gr-mode"
       "point"
       "gr-frame"
       (tuple "scale" "1cm" (tuple "0.5gw" "0.5gh"))
       "gr-geometry"
       (tuple "geometry" "1par" "0.6par")
       "gr-auto-crop"
       "true"
       "gr-grid"
       (tuple "cartesian" (point "0" "0") "1")
       (graphics (point "0" "0") (line (point "-1" "-1") (point "1" "1"))))
    '(10 0)
  ) ;insert-go-to

  (check (in-graphics?) => #t)
  (let* ((u-crop (url-glue (url-temp) ".png")))
    (graphics-export-png u-crop)
    (check (url-exists? u-crop) => #t)
    (check (> (url-size u-crop) 0) => #t)
    (system-remove u-crop)
  ) ;let*

  (check-report)
) ;define
