
(import (texmacs protocol)
  (liii os)
  (liii path)
  (liii uuid)
  (liii sys)
  (liii string)
  (liii list)
) ;import

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
  (flush-verbatim "TeXmacs interface to TikZ"))

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
    (if (string-starts? trimmed "\\documentclass")
        code
        (let ((inner-code
                (if (or (string-starts? trimmed "\\usetikzlibrary")
                        (string-starts? trimmed "\\begin{tikzpicture}"))
                    code
                    (string-append "\\begin{tikzpicture}\n" code "\n\\end{tikzpicture}"))))
          (string-append
            "\\documentclass[tikz]{standalone}\n\\begin{document}\n"
            inner-code
            "\n\\end{document}")))))

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

(define (run-latex tex-path latex-bin)
  (let ((cmd (string-append (goldfish-quote latex-bin)
                            " --interaction=errorstopmode -halt-on-error "
                            (goldfish-quote tex-path)))
        (orig-dir (getcwd)))
    (unsetenv "DYLD_LIBRARY_PATH")
    (unsetenv "DYLD_FRAMEWORK_PATH")
    (unsetenv "DYLD_FALLBACK_LIBRARY_PATH")
    (unsetenv "DYLD_FALLBACK_FRAMEWORK_PATH")
    (chdir (tikz-temp-dir))
    (let ((result (os-call cmd)))
      (chdir orig-dir)
      result)))

(define (run-dvips dvi-path eps-path dvips-bin)
  (let ((cmd (string-append (goldfish-quote dvips-bin)
                            " -q "
                            (goldfish-quote dvi-path)
                            " -o "
                            (goldfish-quote eps-path))))
    (os-call cmd)))

(define (flush-image path width height)
  (if (and (file-exists? path) (> (path-getsize path) 10))
      (flush-file (string-append path "?" "width=" width "&" "height=" height))
      (flush-verbatim "Failed to generate image")))

(define (eval-and-print code width height)
  (let* ((temp-path (gen-temp-path))
         (tex-path (string-append temp-path ".tex"))
         (dvi-path (string-append temp-path ".dvi"))
         (eps-path (string-append temp-path ".eps"))
         (wrapped-code (wrap-tikz-code code))
         (latex-bin (fourth (argv)))
         (dvips-bin (fifth (argv))))
    (dump-tex-code tex-path wrapped-code)
    (if (zero? (run-latex tex-path latex-bin))
        (if (zero? (run-dvips dvi-path eps-path dvips-bin))
            (flush-image eps-path width height)
            (begin
              (flush-verbatim "dvips error")
              (flush-verbatim "")))
        (begin
          (flush-verbatim "latex error")
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
