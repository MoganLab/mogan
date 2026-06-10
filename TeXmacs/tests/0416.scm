;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : 0416.scm
;; DESCRIPTION : Unit tests for embedded image Base64 inline in HTML export
;; COPYRIGHT   : (C) 2026 Sisyphus
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(use-modules (convert html tmhtml))

(check-set-mode! 'report-failed)

(define (test-embedded-image-base64-inline)
  (display "Verifying embedded image Base64 inline generation...\n")
  ;; Save original state
  (let ((orig-base64 tmhtml-base64?))
    ;; Test 1: Base64 mode with embedded PNG data
    (set! tmhtml-base64? #t)
    (let* ((png-header #u8(137 80 78 71 13 10 26 10 0 0 0 13 73 72 68 82))
           (embedded-name `(tuple ,(bytevector->base64 png-header) "png"))
           (result (tmhtml-image (list embedded-name)))
           (img-attrs (cdadr (car result)))
           (src (cadr (assoc 'src img-attrs)))
          ) ;let*
      (check (string-starts? src "data:image/png;base64,") => #t)
      (check (string-contains? src "iVBORw0KGgo") => #t) ; Base64 of PNG header
    ) ;let*

    ;; Test 2: Base64 mode with width and height
    ;; tmlength->htmllength converts pt to pixels
    (let* ((png-header #u8(137 80 78 71 13 10 26 10))
           (embedded-name `(tuple ,(bytevector->base64 png-header) "png"))
           (result (tmhtml-image (list embedded-name "100pt" "200pt")))
           (img-attrs (cdadr (car result)))
           (src (cadr (assoc 'src img-attrs)))
           (width (cadr (assoc 'width img-attrs)))
           (height (cadr (assoc 'height img-attrs)))
          ) ;let*
      (check (string-starts? src "data:image/png;base64,") => #t)
      (check (string? width) => #t)
      (check (string? height) => #t)
      (check (> (string->number width) 0) => #t)
      (check (> (string->number height) 0) => #t)
    ) ;let*

    ;; Test 3: Base64 mode without width/height (only image data)
    (let* ((png-header #u8(137 80 78 71 13 10 26 10))
           (embedded-name `(tuple ,(bytevector->base64 png-header) "png"))
           (result (tmhtml-image (list embedded-name)))
           (img-attrs (cdadr (car result)))
          ) ;let*
      (check (assoc 'width img-attrs) => #f)
      (check (assoc 'height img-attrs) => #f)
    ) ;let*

    ;; Restore original state
    (set! tmhtml-base64? orig-base64)
    (display "Embedded image Base64 inline tests passed.\n")
  ) ;let
) ;define

(define (bytevector->base64 bv)
  (import (liii base64))
  (utf8->string (bytevector-base64-encode bv))
) ;define

(tm-define (test_0416)
  (test-embedded-image-base64-inline)
  (check-report)
) ;tm-define
