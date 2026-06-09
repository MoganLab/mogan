;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : goldformat-binary.scm
;; DESCRIPTION : Find clang-format binary on different platforms
;; COPYRIGHT   : (C) 2026 Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-library (liii goldformat-binary)
  (import (liii base) (liii os))
  (export clang-format-binary)
  (begin

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

  ) ;begin
) ;define-library
