
(import (scheme base)
  (liii check)
  (liii string)
  (liii list)
)

; Simulate the wrap-tikz-code logic from tm-tikz.scm
(define (wrap-tikz-code code)
  (let ((trimmed (string-trim-left code)))
    (if (string-starts? trimmed "\\documentclass")
      code
      (let ((inner-code
              (if (or (string-starts? trimmed "\\usetikzlibrary")
                      (string-starts? trimmed "\\begin{tikzpicture}"))
                code
                (string-append "\\begin{tikzpicture}\n" code "\n\\end{tikzpicture}")
              ) ;if
            ) ;inner-code
           ) ;
        (string-append
          "\\documentclass[tikz]{standalone}\n\\begin{document}\n"
          inner-code
          "\n\\end{document}"
        ) ;string-append
      ) ;let
    ) ;if
  ) ;let
) ;define

; Simulate the parse-magic-line logic from tm-tikz.scm
(define (parse-magic-line magic-line)
  (let ((tokens (filter (lambda (x) (not (string-null? x))) (string-split magic-line #\space)))
        (width "0px")
        (height "0px")
       ) ;
    (let loop ((args (cdr tokens)))
      (cond ((or (null? args) (null? (cdr args))) (list width height))
            ((string=? (car args) "-width")
             (set! width (cadr args))
             (loop (cddr args))
            ) ;
            ((string=? (car args) "-height")
             (set! height (cadr args))
             (loop (cddr args))
            ) ;
            (else (loop (cddr args)))
      ) ;cond
    ) ;let loop
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for wrap-tikz-code
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check
  (wrap-tikz-code "\\documentclass{article}\n\\begin{document}\n\\end{document}")
  =>
  "\\documentclass{article}\n\\begin{document}\n\\end{document}"
)

(check
  (wrap-tikz-code "  \\documentclass[tikz]{standalone}")
  =>
  "  \\documentclass[tikz]{standalone}"
)

(check
  (wrap-tikz-code "\\usetikzlibrary{calc}\n\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\usetikzlibrary{calc}\n\\draw (0,0) -- (1,1);\n\\end{document}"
)

(check
  (wrap-tikz-code "\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "\\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n\\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

(check
  (wrap-tikz-code "  \\draw (0,0) -- (1,1);")
  =>
  "\\documentclass[tikz]{standalone}\n\\begin{document}\n\\begin{tikzpicture}\n  \\draw (0,0) -- (1,1);\n\\end{tikzpicture}\n\\end{document}"
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for parse-magic-line
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check (parse-magic-line "%") => (list "0px" "0px"))

(check (parse-magic-line "% -width 0.8par") => (list "0.8par" "0px"))

(check (parse-magic-line "% -height 100px") => (list "0px" "100px"))

(check (parse-magic-line "% -width 0.8par -height 100px") => (list "0.8par" "100px"))

(check (parse-magic-line "% -height 100px -width 0.8par") => (list "0.8par" "100px"))

(check (parse-magic-line "% -foo bar -width 0.5par -baz qux -height 50px") => (list "0.5par" "50px"))

(check (parse-magic-line "% -width 0.8par -height") => (list "0.8par" "0px"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; png-size helper
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (png-dimensions-from-bv bv)
  (let ((width (+ (ash (bytevector-u8-ref bv 16) 24)
                  (ash (bytevector-u8-ref bv 17) 16)
                  (ash (bytevector-u8-ref bv 18) 8)
                  (bytevector-u8-ref bv 19)))
        (height (+ (ash (bytevector-u8-ref bv 20) 24)
                   (ash (bytevector-u8-ref bv 21) 16)
                   (ash (bytevector-u8-ref bv 22) 8)
                   (bytevector-u8-ref bv 23))))
    (list width height)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for png-dimensions-from-bv
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define test-png-1x1
  (bytevector #x89 #x50 #x4E #x47 #x0D #x0A #x1A #x0A
              #x00 #x00 #x00 #x0D #x49 #x48 #x44 #x52
              #x00 #x00 #x00 #x01 #x00 #x00 #x00 #x01))

(define test-png-57x57
  (bytevector #x89 #x50 #x4E #x47 #x0D #x0A #x1A #x0A
              #x00 #x00 #x00 #x0D #x49 #x48 #x44 #x52
              #x00 #x00 #x00 #x39 #x00 #x00 #x00 #x39))

(check (png-dimensions-from-bv test-png-1x1) => (list 1 1))
(check (png-dimensions-from-bv test-png-57x57) => (list 57 57))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; eps-bbox-empty? helper
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (eps-bbox-empty? eps-path)
  (let ((p (open-input-file eps-path)))
    (let loop ((i 0))
      (if (>= i 100)
          (begin (close-input-port p) #t)
          (let ((line (read-line p)))
            (if (eof-object? line)
                (begin (close-input-port p) #t)
                (if (string-starts? line "%%BoundingBox: ")
                    (let* ((bbox-str (string-drop line (string-length "%%BoundingBox: ")))
                           (parts (map string->number (string-split bbox-str #\space))))
                      (close-input-port p)
                      (if (= (length parts) 4)
                          (let ((x1 (car parts))
                                (y1 (cadr parts))
                                (x2 (caddr parts))
                                (y2 (cadddr parts)))
                            (or (<= (- x2 x1) 1) (<= (- y2 y1) 1)))
                          #t))
                    (loop (+ i 1)))))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tests for eps-bbox-empty?
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define test-eps-empty-path "/tmp/tikz-test-empty.eps")
(define test-eps-valid-path "/tmp/tikz-test-valid.eps")
(define test-eps-1x1-path "/tmp/tikz-test-1x1.eps")

(with-output-to-file test-eps-empty-path
  (lambda ()
    (display "%!PS-Adobe-3.0 EPSF-3.0\n")
    (display "%%BoundingBox: 0 0 0 0\n")
    (display "%%EndComments\n")))

(with-output-to-file test-eps-valid-path
  (lambda ()
    (display "%!PS-Adobe-3.0 EPSF-3.0\n")
    (display "%%BoundingBox: 0 0 10 10\n")
    (display "%%EndComments\n")))

(with-output-to-file test-eps-1x1-path
  (lambda ()
    (display "%!PS-Adobe-3.0 EPSF-3.0\n")
    (display "%%BoundingBox: 0 0 1 1\n")
    (display "%%EndComments\n")))

(check (eps-bbox-empty? test-eps-empty-path) => #t)
(check (eps-bbox-empty? test-eps-valid-path) => #f)
(check (eps-bbox-empty? test-eps-1x1-path) => #t)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Run all tests
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(check-report "TikZ plugin unit tests")
