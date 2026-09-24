;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : python-env-test.scm
;; DESCRIPTION : Test Python and Conda environment portability
;; COPYRIGHT   : (C) 2026 Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(import (liii check))

(check-set-mode! 'report-failed)

(load "./TeXmacs/plugins/binary/progs/binary/conda.scm")
(load "./TeXmacs/plugins/python/progs/python/python-binary.scm")
(load "./TeXmacs/plugins/python/progs/init-python.scm")

(define (test-python-connection-variants)
  (let ((vars (connection-variants "python")))
    ;; "default" must always be the first variant
    (check (and (pair? vars) (string=? (car vars) "default")) => #t)
  ) ;let
) ;define

(define (test-python-fallback-for-missing-env)
  (let ((default-info (connection-info "python" "default"))
        (fallback-info (connection-info "python" "conda_nonexistent_env_6102"))
        (plot-fallback (connection-info "python" "conda_other_machine_env"))
       ) ;
    ;; When an unknown environment is requested, it must fallback to default
    (check fallback-info => default-info)
    (check plot-fallback => default-info)
  ) ;let
) ;define

(define (test-conda-env-python-list-order)
  (when (has-binary-conda?)
    (let ((env-list (conda-env-python-list)))
      (when (pair? env-list)
        ;; If there is an env, the launcher should be defined
        (check (string? (conda-env-name (car env-list))) => #t)
        ;; default info must return a valid pipe tuple
        (check (tm-func? (connection-info "python" "default") 'tuple 2) => #t)
      ) ;when
    ) ;let
  ) ;when
) ;define

(tm-define (regtest-python-env)
  (lazy-plugin-force-one "python")
  (test-python-connection-variants)
  (test-python-fallback-for-missing-env)
  (test-conda-env-python-list-order)
  (check-report)
) ;tm-define
