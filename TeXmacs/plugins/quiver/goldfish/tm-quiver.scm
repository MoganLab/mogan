;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-quiver.scm
;; DESCRIPTION : Quiver Binary plugin (pdflatex)
;; COPYRIGHT   : (C) 2026  (Jack) Yansong Li
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (scheme base)
  (texmacs protocol)
  (liii os)
  (liii path)
  (liii uuid)
  (liii sys)
  (liii string)
  (liii list)
) ;import

(define (escape-string str)
  (string-join (map (lambda (char)
                      (if (char=? char #\")
                        (string #\\ #\")
                        (if (char=? char #\\) (string #\\ #\\) (string char))
                      ) ;if
                    ) ;lambda
                 (string->list str)
               ) ;map
  ) ;string-join
) ;define

(define (goldfish-quote s)
  (string-append "\"" (escape-string s) "\"")
) ;define

(define (quiver-welcome)
  (flush-prompt "quiver] ")
  (flush-verbatim "Liii STEM interface to Quiver")
) ;define

(define (quiver-read-code)
  (define (read-code code)
    (let ((line (read-line)))
      (if (or (eof-object? line) (string=? line "<EOF>\n"))
        code
        (read-code (string-append code line))
      ) ;if
    ) ;let
  ) ;define
  (read-code "")
) ;define

(define (gen-temp-path)
  (let ((quiver-tmpdir (string-append (os-temp-dir) "/quiver")))
    (when (not (file-exists? quiver-tmpdir))
      (mkdir quiver-tmpdir)
    ) ;when
    (string-append quiver-tmpdir "/" (uuid4))
  ) ;let
) ;define

(define (string-trim-both s)
  (string-trim-right (string-trim s))
) ;define

(define (strip-math-delimiters str)
  (let* ((s (string-trim-both str)) (len (string-length s)))
    (cond ((and (>= len 4) (string-starts? s "\\[") (string-ends? s "\\]"))
           (strip-math-delimiters (substring s 2 (- len 2)))
          ) ;
          ((and (>= len 4) (string-starts? s "$$") (string-ends? s "$$"))
           (strip-math-delimiters (substring s 2 (- len 2)))
          ) ;
          ((and (>= len 2) (string-starts? s "$") (string-ends? s "$"))
           (strip-math-delimiters (substring s 1 (- len 1)))
          ) ;
          ((and (>= len 32)
             (string-starts? s "\\begin{equation*}")
             (string-ends? s "\\end{equation*}")
           ) ;and
           (strip-math-delimiters (substring s 17 (- len 15)))
          ) ;
          ((and (>= len 30)
             (string-starts? s "\\begin{equation}")
             (string-ends? s "\\end{equation}")
           ) ;and
           (strip-math-delimiters (substring s 16 (- len 14)))
          ) ;
          ((and (>= len 36)
             (string-starts? s "\\begin{displaymath}")
             (string-ends? s "\\end{displaymath}")
           ) ;and
           (strip-math-delimiters (substring s 19 (- len 17)))
          ) ;
          (else s)
    ) ;cond
  ) ;let*
) ;define

(define (wrap-quiver-code raw-code)
  (let* ((code (strip-math-delimiters raw-code)) (trimmed (string-trim-left code)))
    (if (string-starts? trimmed "\\documentclass")
      code
      (let* ((lines (string-split code #\newline))
             (library-lines (filter (lambda (line) (string-starts? (string-trim-left line) "\\usetikzlibrary"))
                              lines
                            ) ;filter
             ) ;library-lines
             (package-lines (filter (lambda (line) (string-starts? (string-trim-left line) "\\usepackage"))
                              lines
                            ) ;filter
             ) ;package-lines
             (other-lines (filter (lambda (line)
                                    (let ((trimmed-line (string-trim-left line)))
                                      (and (not (string-null? trimmed-line))
                                        (not (string-starts? trimmed-line "\\usetikzlibrary"))
                                        (not (string-starts? trimmed-line "\\usepackage"))
                                      ) ;and
                                    ) ;let
                                  ) ;lambda
                            lines
                          ) ;filter
             ) ;other-lines
             (body (string-join other-lines "\n"))
             (body-trimmed (string-trim-left body))
            ) ;
        (let* ((has-tikzcd? (string-starts? body-trimmed "\\begin{tikzcd}"))
               (inner-code (if (or (string-null? body-trimmed) has-tikzcd?)
                             body
                             (string-append "\\begin{tikzcd}[nodes in empty cells]\n" body "\n\\end{tikzcd}")
                           ) ;if
               ) ;inner-code
              ) ;
          (string-append "\\documentclass[tikz]{standalone}\n"
            "\\usepackage{tikz-cd}\n"
            "\\usepackage{amssymb}\n"
            "\\usetikzlibrary{calc}\n"
            "\\usetikzlibrary{decorations.pathmorphing}\n"
            "\\usetikzlibrary{spath3}\n"
            "\n"
            "\\tikzset{curve/.style={settings={#1},to path={(\\tikztostart)\n"
            "    .. controls ($(\\tikztostart)!\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
            "    and ($(\\tikztostart)!1-\\pv{pos}!(\\tikztotarget)!\\pv{height}!270:(\\tikztotarget)$)\n"
            "    .. (\\tikztostart)\\tikztonodes}},\n"
            "    settings/.code={\\tikzset{quiver/.cd,#1}\n"
            "        \\def\\pv##1{\\pgfkeysvalueof{/tikz/quiver/##1}}},\n"
            "    quiver/.cd,pos/.initial=0.35,height/.initial=0}\n"
            "\n"
            "\\tikzset{between/.style n args={2}{/tikz/execute at end to={\n"
            "    \\tikzset{spath/split at keep middle={current}{#1}{#2}}\n"
            "}}}\n"
            "\n"
            "\\tikzset{tail reversed/.code={\\pgfsetarrowsstart{tikzcd to}}}\n"
            "\\tikzset{2tail/.code={\\pgfsetarrowsstart{Implies[reversed]}}}\n"
            "\\tikzset{2tail reversed/.code={\\pgfsetarrowsstart{Implies}}}\n"
            "\\tikzset{no body/.style={/tikz/dash pattern=on 0 off 1mm}}\n"
            "\n"
            (if (null? package-lines)
              ""
              (string-append (string-join package-lines "\n") "\n")
            ) ;if
            "\\begin{document}\n"
            (if (null? library-lines)
              ""
              (string-append (string-join library-lines "\n") "\n")
            ) ;if
            inner-code
            "\n\\end{document}"
          ) ;string-append
        ) ;let*
      ) ;let*
    ) ;if
  ) ;let*
) ;define

(define (parse-magic-line magic-line)
  (let ((tokens (filter (lambda (x) (not (string-null? x))) (string-split magic-line #\space))
        ) ;tokens
        (width "0px")
        (height "0px")
       ) ;
    (let loop
      ((args (cdr tokens)))
      (cond ((or (null? args) (null? (cdr args))) (list width height))
            ((string=? (car args) "-width") (set! width (cadr args)) (loop (cddr args)))
            ((string=? (car args) "-height") (set! height (cadr args)) (loop (cddr args)))
            (else (loop (cddr args)))
      ) ;cond
    ) ;let
  ) ;let
) ;define

(define (dump-tex-code code-path code)
  (with-output-to-file code-path (lambda () (display code)))
) ;define

(define (quiver-temp-dir)
  (string-append (os-temp-dir) "/quiver")
) ;define

(define (run-pdflatex tex-path pdflatex-bin)
  (let* ((inner-cmd (string-append (goldfish-quote pdflatex-bin)
                      " --interaction=errorstopmode -halt-on-error "
                      (goldfish-quote tex-path)
                      " > /dev/null 2>&1 < /dev/null"
                    ) ;string-append
         ) ;inner-cmd
         (cmd (string-append "sh -c " (goldfish-quote inner-cmd)))
         (orig-dir (getcwd))
        ) ;
    (unsetenv "DYLD_LIBRARY_PATH")
    (unsetenv "DYLD_FRAMEWORK_PATH")
    (unsetenv "DYLD_FALLBACK_LIBRARY_PATH")
    (unsetenv "DYLD_FALLBACK_FRAMEWORK_PATH")
    (chdir (quiver-temp-dir))
    (let ((result (os-call cmd)))
      (chdir orig-dir)
      result
    ) ;let
  ) ;let*
) ;define

(define (image-valid? path)
  (and (file-exists? path) (> (path-getsize path) 10))
) ;define

(define (get-pdf-page-size log-path)
  (let ((p (open-input-file log-path)))
    (let loop
      ()
      (let ((line (read-line p)))
        (if (eof-object? line)
          (begin
            (close-input-port p)
            #f
          ) ;begin
          (if (string-contains? line "papersize=")
            (let* ((parts (string-split line #\=))
                   (size-part (cadr parts))
                   (dims (string-split size-part #\,))
                   (w-str (string-remove-suffix (car dims) "pt"))
                   (h-str (string-remove-suffix (cadr dims) "pt"))
                   (w (string->number w-str))
                   (h (string->number h-str))
                  ) ;
              (close-input-port p)
              (if (and w h) (list w h) #f)
            ) ;let*
            (loop)
          ) ;if
        ) ;if
      ) ;let
    ) ;let
  ) ;let
) ;define

(define (pdf-page-empty? log-path)
  (let ((size (get-pdf-page-size log-path)))
    (if (not size)
      #t
      (let ((w (car size)) (h (cadr size)))
        (and (<= w 0.1) (<= h 0.1))
      ) ;let
    ) ;if
  ) ;let
) ;define

(define (flush-image path width height)
  (if (image-valid? path)
    (flush-file (string-append path "?" "width=" width "&" "height=" height))
    (flush-verbatim "Failed to generate image")
  ) ;if
) ;define

(define (eval-and-print code width height)
  (let* ((temp-path (gen-temp-path))
         (tex-path (string-append temp-path ".tex"))
         (pdf-path (string-append temp-path ".pdf"))
         (log-path (string-append temp-path ".log"))
         (wrapped-code (wrap-quiver-code code))
         (pdflatex-bin (fourth (argv)))
        ) ;
    (dump-tex-code tex-path wrapped-code)
    (if (zero? (run-pdflatex tex-path pdflatex-bin))
      (let ((size (get-pdf-page-size log-path)))
        (if (or (not size) (and (<= (car size) 0.1) (<= (cadr size) 0.1)))
          (flush-verbatim "Quiver produced an empty image (0x0 bounding box)")
          (let ((final-width width) (final-height height))
            (when (and (string=? width "0px") (string=? height "0px"))
              (if size
                (begin
                  (set! final-width (string-append (number->string (car size)) "pt"))
                  (set! final-height (string-append (number->string (cadr size)) "pt"))
                ) ;begin
                (begin
                  (set! final-width "0.3par")
                  (set! final-height "0px")
                ) ;begin
              ) ;if
            ) ;when
            (flush-image pdf-path final-width final-height)
          ) ;let
        ) ;if
      ) ;let
      (begin
        (flush-verbatim "pdflatex error")
        (flush-verbatim "")
      ) ;begin
    ) ;if
  ) ;let*
) ;define

(define (split-code-and-magic code)
  (if (not (string-starts? code "%"))
    (list "" code)
    (let ((i/false (string-index code #\newline)))
      (if (not i/false)
        (list code "")
        (list (substring code 0 i/false) (substring code (+ i/false 1)))
      ) ;if
    ) ;let
  ) ;if
) ;define

(define (read-eval-print)
  (let* ((raw-code (quiver-read-code))
         (l (split-code-and-magic raw-code))
         (magic-line (car l))
         (code (cadr l))
         (parsed (if (string-null? magic-line) (list "0px" "0px") (parse-magic-line magic-line))
         ) ;parsed
         (width (car parsed))
         (height (cadr parsed))
        ) ;
    (if (string-null? code)
      (flush-verbatim "No code provided!")
      (eval-and-print code width height)
    ) ;if
  ) ;let*
) ;define

(define (safe-read-eval-print)
  (catch #t
    (lambda () (read-eval-print))
    (lambda args
      (flush-scheme (string-append "(errput (document "
                      (goldfish-quote (symbol->string (car args)))
                      " "
                      (if (and (>= (length args) 2) (not (null? (cadr args))))
                        (goldfish-quote (object->string (cadr args)))
                        ""
                      ) ;if
                      "))"
                    ) ;string-append
      ) ;flush-scheme
    ) ;lambda
  ) ;catch
) ;define

(define (quiver-repl)
  (safe-read-eval-print)
  (quiver-repl)
) ;define

(quiver-welcome)
(quiver-repl)
