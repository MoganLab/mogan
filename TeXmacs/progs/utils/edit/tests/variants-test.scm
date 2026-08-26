;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : variants-test.scm
;; DESCRIPTION : Test suite for variant switching
;; COPYRIGHT   : (C) 2026  Mogan STEM authors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils edit variants))

(import (liii check))

(check-set-mode! 'report-failed)

(define (test-variant-menu-categories)
  ;; 变体切换只影响焦点工具栏，不请求其它菜单段重建
  (check (variant-menu-categories) => '(icons-focus))
) ;define

(tm-define (regtest-variants)
  (test-variant-menu-categories)
  (check-report)
) ;tm-define
