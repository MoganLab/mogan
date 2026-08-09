
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : common.scm
;; DESCRIPTION : routines for Binary plugins
;; COPYRIGHT   : (C) 2024  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (binary common))

(define (find-binary-in-path name)
  (let* ((u (url-resolve-in-path name))
         (excluded? (and (os-windows?) (url-descends? u (system->url "C:\\Windows\\System32")))
         ) ;excluded?
        ) ;
    (if excluded? (url-none) u)
  ) ;let*
) ;define

(define (find-binary-in-candidates-name candidates)
  (with names
    (list-remove-duplicates (map (lambda (x) (url->string (url-tail x))) candidates)
    ) ;list-remove-duplicates
    (with u
      (list-find (map (lambda (x) (find-binary-in-path x)) names) url-exists?)
      (or u (url-none))
    ) ;with
  ) ;with
) ;define

(define (find-binary-in-candidates candidates)
  (with u
    (list-find candidates (lambda (x) (url-exists? (url-resolve x "r"))))
    (and u (url-resolve u "r"))
  ) ;with
) ;define

(define (find-binary-in-specified path)
  (with u (url-resolve path "r") (if (and (url-exists? u) (url-regular? u)) u #f))
) ;define

(tm-define (find-binary candidates binary-id)
  (let* ((global-binary-opt (get-preference "plugin:binary"))
         (this-binary-opt (get-preference (string-append "plugin:binary:" binary-id)))
        ) ;
    (cond ((== global-binary-opt "off") (url-none))
          ((== this-binary-opt "off") (url-none))
          ((== this-binary-opt "candidates-only") (find-binary-in-candidates candidates))
          (else (or (and (!= this-binary-opt "default") (find-binary-in-specified this-binary-opt))
                  (find-binary-in-candidates candidates)
                  (find-binary-in-candidates-name candidates)
                ) ;or
          ) ;else
    ) ;cond
  ) ;let*
) ;tm-define

(tm-define (version-binary u)
  (if (url-none? u)
    ""
    (let* ((msg (check-stdout (string-append (url->system u) " --version")))
           (msg-l (filter (lambda (x) (not (string-null? x))) (string-split msg #\newline))
           ) ;msg-l
          ) ;
      (if (== (length msg-l) 0) "" (car msg-l))
    ) ;let*
  ) ;if
) ;tm-define
