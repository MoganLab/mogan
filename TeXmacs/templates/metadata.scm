
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : metadata.scm
;; DESCRIPTION : Local template metadata for bundled templates
;; COPYRIGHT   : (C) 2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (templates metadata))

(tm-define bundled-templates
  ;; Local templates bundled with Mogan
  ;; Remote templates will be loaded from Gitee Releases
  '())

(tm-define (template-get-bundled-templates)
  (:synopsis "Get list of bundled template metadata")
  bundled-templates)

(tm-define (template-exists? template-id)
  (:synopsis "Check if a template exists in bundled set")
  (assoc template-id bundled-templates))

(tm-define (template-get-metadata template-id)
  (:synopsis "Get metadata for a specific template")
  (let ((tmpl (assoc template-id bundled-templates)))
    (if tmpl
        (cdr tmpl)
        #f)))
