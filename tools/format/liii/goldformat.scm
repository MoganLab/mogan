;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldformat.scm
;; DESCRIPTION : Format C++ and Scheme files (replaces bin/format)
;; COPYRIGHT   : (C) 2026 Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (liii goldformat)
  (import (liii base)
    (liii sys)
    (liii os)
    (liii path)
    (liii goldformat-binary)
    (liii goldformat-path)
  ) ;import
  (export main)
  (begin

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
