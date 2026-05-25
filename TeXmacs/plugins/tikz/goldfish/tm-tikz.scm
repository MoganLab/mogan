
(import (scheme base)
  (texmacs protocol)
  (liii os)
  (liii path)
  (liii uuid)
  (liii sys)
  (liii string)
  (liii list)
)

(define (escape-string str)
  (string-join
    (map (lambda (char)
           (if (char=? char #\")
               (string #\\ #\")
               (if (char=? char #\\)
                   (string #\\ #\\)
                   (string char))))
         (string->list str))))

(define (goldfish-quote s)
  (string-append "\"" (escape-string s) "\""))

(define (tikz-welcome)
  (flush-prompt "tikz] ")
  (flush-verbatim "Liii STEM interface to TikZ"))

(define (tikz-read-code)
  (define (read-code code)
    (let ((line (read-line)))
      (if (or (eof-object? line) (string=? line "<EOF>\n"))
          code
          (read-code (string-append code line)))))
  (read-code ""))

(define (gen-temp-path)
  (let ((tikz-tmpdir (string-append (os-temp-dir) "/tikz")))
    (when (not (file-exists? tikz-tmpdir))
      (mkdir tikz-tmpdir))
    (string-append tikz-tmpdir "/" (uuid4))))

(define (wrap-tikz-code code)
  (let ((trimmed (string-trim-left code)))
    (if (or (string-starts? trimmed "\\documentclass")
            (string-contains? code "\\begin{document}"))
        code
        (let* ((lines (string-split code #\newline))
               (library-lines
                 (filter (lambda (line)
                           (string-starts? (string-trim-left line) "\\usetikzlibrary"))
                         lines))
               (package-lines
                 (filter (lambda (line)
                           (string-starts? (string-trim-left line) "\\usepackage"))
                         lines))
             (other-lines
               (filter (lambda (line)
                         (let ((trimmed-line (string-trim-left line)))
                           (and (not (string-starts? trimmed-line "\\usetikzlibrary"))
                                (not (string-starts? trimmed-line "\\usepackage"))
                                (not (string-null? trimmed-line)))))
                       lines))
               (body (string-join other-lines "\n"))
               (body-trimmed (string-trim-left body)))
          (let* ((has-chemfig? (or (string-starts? body-trimmed "\\chem")
                                  (string-starts? body-trimmed "\\scheme")))
                 (need-chemfig-package?
                   (or has-chemfig?
                       (string-contains? body "\\chem")
                       (string-contains? body "\\scheme")))
                 (need-optikz-package?
                   (or (string-contains? body "\\spectrometer")
                       (string-contains? body "\\camera")
                       (string-contains? body "\\diode")
                       (string-contains? body "\\splitter")
                       (string-contains? body "\\convexlens")
                       (string-contains? body "\\concavelens")
                       (string-contains? body "\\planconvexlens")
                       (string-contains? body "\\mirror")
                       (string-contains? body "\\curvedmirror")
                       (string-contains? body "\\pockelscell")
                       (string-contains? body "\\laser")
                       (string-contains? body "\\grating")
                       (string-contains? body "\\drawrainbow")
                       (string-contains? body "\\TFP")))
                 (extra-packages
                   (let ((pkgs '()))
                     (when (and need-chemfig-package?
                                (null? (filter (lambda (line) (string-contains? line "chemfig"))
                                               package-lines)))
                       (set! pkgs (cons "\\usepackage{chemfig}" pkgs)))
                     (when (and need-optikz-package?
                                (null? (filter (lambda (line) (string-contains? line "optikz"))
                                               package-lines)))
                       (set! pkgs (cons "\\usepackage{optikz}" pkgs)))
                     (when (and need-optikz-package?
                                (null? (filter (lambda (line) (string-contains? line "calc"))
                                               package-lines)))
                       (set! pkgs (cons "\\usepackage{calc}" pkgs)))
                     (reverse pkgs)))
                  (inner-code
                    (if (or (string-null? body-trimmed)
                            (string-contains? body "\\begin{tikzpicture}")
                            has-chemfig?)
                        body
                        (string-append "\\begin{tikzpicture}\n" body "\n\\end{tikzpicture}"))))
            (string-append
              "\\documentclass[tikz,rgb]{standalone}\n"
              (if (null? package-lines) "" (string-append (string-join package-lines "\n") "\n"))
              (if (null? extra-packages) "" (string-append (string-join extra-packages "\n") "\n"))
              "\\begin{document}\n"
              (if (null? library-lines) "" (string-append (string-join library-lines "\n") "\n"))
              inner-code
              "\n\\end{document}"))))))

(define (parse-magic-line magic-line)
  (let ((tokens (filter (lambda (x) (not (string-null? x)))
                        (string-split magic-line #\space)))
        (width "0px")
        (height "0px"))
    (let loop ((args (cdr tokens)))
      (cond ((or (null? args) (null? (cdr args)))
             (list width height))
            ((string=? (car args) "-width")
             (set! width (cadr args))
             (loop (cddr args)))
            ((string=? (car args) "-height")
             (set! height (cadr args))
             (loop (cddr args)))
            (else (loop (cddr args)))))))

(define (dump-tex-code code-path code)
  (with-output-to-file code-path
    (lambda () (display code))))

(define (tikz-temp-dir)
  (string-append (os-temp-dir) "/tikz"))

(define (run-pdflatex tex-path pdflatex-bin)
  (let* ((inner-cmd (string-append (goldfish-quote pdflatex-bin)
                                   " --interaction=errorstopmode -halt-on-error "
                                   (goldfish-quote tex-path)
                                   " > /dev/null 2>&1"))
         (cmd (string-append "sh -c " (goldfish-quote inner-cmd)))
         (orig-dir (getcwd)))
    (unsetenv "DYLD_LIBRARY_PATH")
    (unsetenv "DYLD_FRAMEWORK_PATH")
    (unsetenv "DYLD_FALLBACK_LIBRARY_PATH")
    (unsetenv "DYLD_FALLBACK_FRAMEWORK_PATH")
    (chdir (tikz-temp-dir))
    (let ((result (os-call cmd)))
      (chdir orig-dir)
      result)))

(define (image-valid? path)
  (and (file-exists? path) (> (path-getsize path) 10)))

(define (get-pdf-page-size log-path)
  (let ((p (open-input-file log-path)))
    (let loop ()
      (let ((line (read-line p)))
        (if (eof-object? line)
            (begin (close-input-port p) #f)
            (if (string-contains? line "papersize=")
                (let* ((parts (string-split line #\=))
                       (size-part (cadr parts))
                       (dims (string-split size-part #\,))
                       (w-str (string-remove-suffix (car dims) "pt"))
                       (h-str (string-remove-suffix (cadr dims) "pt"))
                       (w (string->number w-str))
                       (h (string->number h-str)))
                  (close-input-port p)
                  (if (and w h)
                      (list w h)
                      #f))
                (loop)))))))

(define (pdf-page-empty? log-path)
  (let ((size (get-pdf-page-size log-path)))
    (if (not size)
        #t
        (let ((w (car size))
              (h (cadr size)))
          (and (<= w 0.1) (<= h 0.1))))))

(define (flush-image path width height)
  (if (image-valid? path)
      (flush-file (string-append path "?" "width=" width "&" "height=" height))
      (flush-verbatim "Failed to generate image")))

(define (eval-and-print code width height)
  (let* ((temp-path (gen-temp-path))
         (tex-path (string-append temp-path ".tex"))
         (pdf-path (string-append temp-path ".pdf"))
         (log-path (string-append temp-path ".log"))
         (wrapped-code (wrap-tikz-code code))
         (pdflatex-bin (fourth (argv))))
    (dump-tex-code tex-path wrapped-code)
    (if (zero? (run-pdflatex tex-path pdflatex-bin))
        (let ((size (get-pdf-page-size log-path)))
          (if (or (not size) (and (<= (car size) 0.1) (<= (cadr size) 0.1)))
              (flush-verbatim "TikZ produced an empty image (0x0 bounding box)")
              (let ((final-width width)
                    (final-height height))
                (when (and (string=? width "0px") (string=? height "0px"))
                  (if size
                      (begin
                        (set! final-width (string-append (number->string (car size)) "pt"))
                        (set! final-height (string-append (number->string (cadr size)) "pt")))
                      (begin
                        (set! final-width "0.3par")
                        (set! final-height "0px"))))
                (flush-image pdf-path final-width final-height))))
        (begin
          (flush-verbatim "pdflatex error")
          (flush-verbatim "")))))

(define (split-code-and-magic code)
  (if (not (string-starts? code "%"))
      (list "" code)
      (let ((i/false (string-index code #\newline)))
        (if (not i/false)
            (list code "")
            (list (substring code 0 i/false)
                  (substring code (+ i/false 1)))))))

(define (read-eval-print)
  (let* ((raw-code (tikz-read-code))
         (l (split-code-and-magic raw-code))
         (magic-line (car l))
         (code (cadr l))
         (parsed (if (string-null? magic-line)
                     (list "0px" "0px")
                     (parse-magic-line magic-line)))
         (width (car parsed))
         (height (cadr parsed)))
    (if (string-null? code)
        (flush-verbatim "No code provided!")
        (eval-and-print code width height))))

(define (safe-read-eval-print)
  (catch #t
    (lambda () (read-eval-print))
    (lambda args
      (flush-scheme
        (string-append "(errput (document "
                       (goldfish-quote (symbol->string (car args)))
                       " "
                       (if (and (>= (length args) 2) (not (null? (cadr args))))
                           (goldfish-quote (object->string (cadr args)))
                           "")
                       "))")))))

(define (tikz-repl)
  (safe-read-eval-print)
  (tikz-repl))

(tikz-welcome)
(tikz-repl)
