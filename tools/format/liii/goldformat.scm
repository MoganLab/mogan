(define (%goldformat-common-dirname path-str)
  (let loop
    ((i (- (string-length path-str) 1)))
    (cond ((< i 0) ".")
          ((or (char=? (string-ref path-str i) #\/) (char=? (string-ref path-str i) #\\))
           (if (= i 0) "." (substring path-str 0 i))
          ) ;
          (else (loop (- i 1)))
    ) ;cond
  ) ;let
) ;define

(set! *load-path*
  (append (list "../common" "tools/common")
    (map (lambda (root) (string-append (%goldformat-common-dirname root) "/common"))
      *load-path*
    ) ;map
    *load-path*
  ) ;append
) ;set!

(define-library (liii goldformat)
  (import (liii base) (liii sys) (liii os) (liii path) (srfi srfi-13))
  (export main)
  (begin

    (define cpp-exts '(".cpp" ".hpp" ".h" ".c"))

    (define cpp-roots '("tests" "src" "moebius" "3rdparty/lolly"))

    (define scm-dirs
      '("TeXmacs/plugins/gnuplot"
        "TeXmacs/progs/generic"
        "TeXmacs/progs/kernel"
        "TeXmacs/progs/source"
        "TeXmacs/progs/utils/plugins"
        "TeXmacs/plugins/llm/progs")
    ) ;define

    (define (cpp-file? name)
      (let loop
        ((exts cpp-exts))
        (if (null? exts) #f (if (string-suffix? (car exts) name) #t (loop (cdr exts))))
      ) ;let
    ) ;define

    (define (collect-cpp-files dir-path)
      (let ((entries (path-list-path (path dir-path))))
        (let loop
          ((i 0) (acc '()))
          (if (>= i (vector-length entries))
            acc
            (let ((entry (vector-ref entries i)))
              (cond ((path-file? entry)
                     (let ((s (path->string entry)))
                       (if (cpp-file? s) (loop (+ i 1) (cons s acc)) (loop (+ i 1) acc))
                     ) ;let
                    ) ;
                    ((path-dir? entry)
                     (loop (+ i 1) (append (collect-cpp-files (path->string entry)) acc))
                    ) ;
                    (else (loop (+ i 1) acc))
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (collect-all-cpp-files)
      (let loop
        ((roots cpp-roots) (acc '()))
        (if (null? roots)
          acc
          (if (path-dir? (path (car roots)))
            (loop (cdr roots) (append acc (collect-cpp-files (car roots))))
            (loop (cdr roots) acc)
          ) ;if
        ) ;if
      ) ;let
    ) ;define

    (define (clang-format-binary)
      (cond ((os-windows?) "clang-format")
            ((os-macos?) "/opt/homebrew/opt/llvm@19/bin/clang-format")
            (else (let loop
                    ((paths '("/usr/local/bin/clang-format-19"
                              "/usr/lib/llvm-19/bin/clang-format"
                              "/usr/bin/clang-format-19"
                              "/usr/bin/clang-format")
                     ) ;paths
                    ) ;
                    (if (null? paths)
                      "clang-format"
                      (if (file-exists? (car paths)) (car paths) (loop (cdr paths)))
                    ) ;if
                  ) ;let
            ) ;else
      ) ;cond
    ) ;define

    (define (write-file-list files)
      (let ((tmp (path->string (path-join (os-temp-dir) "goldformat-cpp-files.txt"))))
        (let ((port (open-output-file tmp)))
          (display (car files) port)
          (let loop
            ((fs (cdr files)))
            (if (null? fs)
              (begin
                (close-output-port port)
                tmp
              ) ;begin
              (begin
                (display (string-append "\n" (car fs)) port)
                (loop (cdr fs))
              ) ;begin
            ) ;if
          ) ;let
        ) ;let
      ) ;let
    ) ;define

    (define (flush-output)
      (flush-output-port (current-output-port))
    ) ;define

    (define (format-cpp)
      (let* ((cf (clang-format-binary)) (files (collect-all-cpp-files)))
        (if (null? files)
          (begin
            (display "No C++ files found.")
            (newline)
          ) ;begin
          (let ((list-file (write-file-list files)))
            (display (string-append "Formatting "
                       (number->string (length files))
                       " C++ files with "
                       cf
                     ) ;string-append
            ) ;display
            (newline)
            (flush-output)
            (os-call (string-append cf " -i --files=" list-file))
            (delete-file list-file)
          ) ;let
        ) ;if
      ) ;let*
    ) ;define

    (define (format-scm)
      (let ((gf (executable)))
        (let loop
          ((dirs scm-dirs))
          (if (null? dirs)
            #t
            (begin
              (flush-output)
              (os-call (string-append gf " fmt " (car dirs)))
              (loop (cdr dirs))
            ) ;begin
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (main)
      (display "=== Formatting C++ files ===")
      (newline)
      (flush-output)
      (format-cpp)
      (newline)
      (display "=== Formatting Scheme files ===")
      (newline)
      (flush-output)
      (format-scm)
      (newline)
      (display "Done.")
      (newline)
    ) ;define

  ) ;begin
) ;define-library
