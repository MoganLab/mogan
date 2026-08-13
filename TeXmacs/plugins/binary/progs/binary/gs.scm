
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : gs.scm
;; DESCRIPTION : Ghostscript Binary plugin
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary gs) (:use (binary common)))

(define (gs-binary-candidates)
  (cond ((os-macos?)
         (list "/opt/homebrew/Cellar/ghostscript/1*/bin/gs"
           "/usr/local/Cellar/ghostscript/1*/bin/gs"
         ) ;list
        ) ;
        ((os-windows?)
         (list "$TEXMACS_HOME_PATH\\bin\\gs.exe"
           "C:\\Program Files*\\gs\\gs*\\bin\\gswin64c.exe"
           "C:\\Program Files*\\gs\\gs*\\bin\\gswin32c.exe"
         ) ;list
        ) ;
        (else (list "/usr/bin/gs"))
  ) ;cond
) ;define

(tm-define (find-binary-gs)
  (:synopsis "Find the url to the gs binary, return (url-none) if not found")
  (find-binary (gs-binary-candidates) "gs")
) ;tm-define

(tm-define (has-binary-gs?) (not (url-none? (find-binary-gs))))

(tm-define (version-binary-gs) (version-binary (find-binary-gs)))

(define (get-image-size-from-bbox line)
  (let* ((box (string-drop line (length "%%BoundingBox: ")))
         (num-box (string-replace box "\r" ""))
         (fbox (and (> (length box) 0) (map string->float (string-split num-box #\space)))
         ) ;fbox
        ) ;
    (and (== (length fbox) 4)
      (list (floor (first fbox))
        ;; x1
        (floor (second fbox))
        ;; y1
        (ceiling (third fbox))
        ;; x2
        (ceiling (fourth fbox))
      ) ;list
    ) ;and
  ) ;let*
) ;define
;; y2

(define (gs-image-size u)
  (let* ((out (check-stderr (string-append (url->system (find-binary-gs))
                              " -dQUIET "
                              " -dNOPAUSE "
                              " -dBATCH "
                              " -dSAFER "
                              " -sDEVICE=bbox "
                              (url-sys-concretize u)
                            ) ;string-append
              ) ;check-stderr
         ) ;out
         (l (filter (lambda (x) (string-starts? x "%%BoundingBox: "))
              (string-split out #\newline)
            ) ;filter
         ) ;l
        ) ;
    (if (> (length l) 0) (get-image-size-from-bbox (car l)))
  ) ;let*
) ;define

(tm-define (eps-image-size u)
  (let* ((out (string-load u))
         (l (filter (lambda (x) (string-starts? x "%%BoundingBox: "))
              (list-take (string-split out #\newline) 100)
            ) ;filter
         ) ;l
        ) ;
    (when (> (length l) 0)
      (let ((res1 (get-image-size-from-bbox (car l))))
        (if res1 res1 (gs-image-size u))
      ) ;let
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (gs-eps-to-pdf from opts)
  (let* ((to (assoc-ref opts 'dest))
         (box (eps-image-size from))
         (w (number->string (- (third box) (first box))))
         (h (number->string (- (fourth box) (second box))))
         (offset-x (number->string (- (first box))))
         (offset-y (number->string (- (second box))))
         (gs-inline (string-append "<< /PageSize [ " w " " h
                      " ] >> setpagedevice gsave " offset-x " " offset-y
                      " translate " "1 1 scale"
                    ) ;string-append
         ) ;gs-inline
         (cmd (string-append (url->system (find-binary-gs))
                " -dQUIET "
                " -dNOPAUSE "
                " -dBATCH "
                " -dSAFER "
                " -sDEVICE=pdfwrite "
                " -dAutoRotatePages=/None "
                " -dCompatibilityLevel=1.4 "
                (string-append " -sOutputFile=" (url->system to) " ")
                (string-append " -c " (string-quote gs-inline))
                (string-append " -f " (url-sys-concretize from) " ")
                (string-append " -c " (string-quote " grestore "))
              ) ;string-append
         ) ;cmd
        ) ;
    (debug-message "io" (string-append "call: " cmd "\n"))
    (system cmd)
  ) ;let*
) ;tm-define

(tm-define (gs-eps-to-png from opts)
  (let* ((to (assoc-ref opts 'dest))
         (opt_w (assoc-ref opts 'width))
         (opt_h (assoc-ref opts 'height))
         (box (eps-image-size from))
         (box_w (- (third box) (first box)))
         (box_h (- (fourth box) (second box)))
         (width (if (and opt_w (!= opt_w 0)) opt_w box_w))
         (height (if (and opt_h (!= opt_h 0)) opt_h box_w))
         (page_size_in_px (string-append " -g"
                            (number->string (exact (floor width)))
                            "x"
                            (number->string (exact (floor height)))
                          ) ;string-append
         ) ;page_size_in_px
         (resolution_in_px (string-append " -r"
                             (number->string (round (/ (* width 72.0) box_w)))
                             "x"
                             (number->string (round (/ (* height 72.0) box_h)))
                             " "
                           ) ;string-append
         ) ;resolution_in_px
         (offset-x (number->string (- (first box))))
         (offset-y (number->string (- (second box))))
         (gs-inline (string-append " " offset-x " " offset-y " translate gsave "))
         (cmd (string-append (string-append (url->system (find-binary-gs))
                               " -dQUIET "
                               " -dNOPAUSE "
                               " -dBATCH "
                               " -dSAFER "
                               " -sDEVICE=pngalpha "
                               " -dGraphicsAlphaBits=4 "
                               " -dTextAlphaBits=4 "
                               page_size_in_px
                               (string-append " -sOutputFile=" (url->system to) " ")
                               resolution_in_px
                               (string-append " -c " (string-quote gs-inline))
                               (string-append " -f " (url-sys-concretize from) " ")
                               (string-append " -c " (string-quote " grestore "))
                             ) ;string-append
              ) ;string-append
         ) ;cmd
        ) ;
    (debug-message "io" (string-append cmd "\n"))
    (system cmd)
  ) ;let*
) ;tm-define

(tm-define (gs-pdf-to-png from opts)
  (let* ((to (assoc-ref opts 'dest))
         (opt_w (assoc-ref opts 'width))
         (opt_h (assoc-ref opts 'height))
         (image_size (pdf-image-size from))
         (box_w (first image_size))
         (box_h (second image_size))
         (width (if (and opt_w (!= opt_w 0)) opt_w box_w))
         (height (if (and opt_h (!= opt_h 0)) opt_h box_w))
         (page_size_in_px (string-append " -g" (number->string width) "x" (number->string height))
         ) ;page_size_in_px
         (resolution_in_px (string-append " -r"
                             (number->string (/ (* width 72.0) box_w))
                             "x"
                             (number->string (/ (* height 72.0) box_h))
                             " "
                           ) ;string-append
         ) ;resolution_in_px
         (cmd (string-append (string-append (url->system (find-binary-gs))
                               " -dQUIET "
                               " -dNOPAUSE "
                               " -dBATCH "
                               " -dSAFER "
                               " -sDEVICE=pngalpha "
                               " -dGraphicsAlphaBits=4 "
                               " -dTextAlphaBits=4 "
                               " -dUseCropBox "
                               (string-append " -sOutputFile=" (url->system to) " ")
                               page_size_in_px
                               resolution_in_px
                               (url-sys-concretize from)
                             ) ;string-append
              ) ;string-append
         ) ;cmd
        ) ;
    (debug-message "io" (string-append cmd "\n"))
    (system cmd)
  ) ;let*
) ;tm-define
