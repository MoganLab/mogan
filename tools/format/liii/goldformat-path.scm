;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldformat-path.scm
;; DESCRIPTION : Project-specific paths and file collection for formatting
;; COPYRIGHT   : (C) 2026 Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (liii goldformat-path)
  (import (liii base) (liii path) (liii string))
  (export cpp-roots
    scm-dirs
    collect-cpp-files
    collect-all-cpp-files
    collect-scm-files
    collect-all-scm-files
  ) ;export
  (begin

    (define cpp-roots '("tests" "src" "moebius" "3rdparty/lolly"))

    (define scm-dirs
      '("TeXmacs/plugins/gnuplot"
        "TeXmacs/progs/client"
        "TeXmacs/progs/convert"
        "TeXmacs/progs/database"
        "TeXmacs/progs/generic"
        "TeXmacs/progs/kernel"
        "TeXmacs/progs/source"
        "TeXmacs/progs/texmacs/texmacs"
        "TeXmacs/progs/texmacs/menus"
        "TeXmacs/progs/texmacs/keyboard"
        "TeXmacs/progs/utils/plugins"
        "TeXmacs/plugins/llm/progs")
    ) ;define

    (define cpp-exts '(".cpp" ".hpp" ".h" ".c"))

    (define (cpp-file? name)
      (let loop
        ((exts cpp-exts))
        (if (null? exts) #f (if (string-ends? name (car exts)) #t (loop (cdr exts))))
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

    (define scm-exts '(".scm"))

    (define (scm-file? name)
      (let loop
        ((exts scm-exts))
        (if (null? exts) #f (if (string-ends? name (car exts)) #t (loop (cdr exts))))
      ) ;let
    ) ;define

    (define (collect-scm-files dir-path)
      (let ((entries (path-list-path (path dir-path))))
        (let loop
          ((i 0) (acc '()))
          (if (>= i (vector-length entries))
            acc
            (let ((entry (vector-ref entries i)))
              (cond ((path-file? entry)
                     (let ((s (path->string entry)))
                       (if (scm-file? s) (loop (+ i 1) (cons s acc)) (loop (+ i 1) acc))
                     ) ;let
                    ) ;
                    ((path-dir? entry)
                     (loop (+ i 1) (append (collect-scm-files (path->string entry)) acc))
                    ) ;
                    (else (loop (+ i 1) acc))
              ) ;cond
            ) ;let
          ) ;if
        ) ;let
      ) ;let
    ) ;define

    (define (collect-all-scm-files)
      (let loop
        ((dirs scm-dirs) (acc '()))
        (if (null? dirs)
          acc
          (if (path-dir? (path (car dirs)))
            (loop (cdr dirs) (append acc (collect-scm-files (car dirs))))
            (loop (cdr dirs) acc)
          ) ;if
        ) ;if
      ) ;let
    ) ;define

  ) ;begin
) ;define-library
