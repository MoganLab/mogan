;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-julia.scm
;; DESCRIPTION : Initialize the julia plugin
;; COPYRIGHT   : (C) 2021 Massimiliano Gubinelli
;;                   2026 Tianyou Liu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (dynamic session-edit) (binary julia))

(define (julia-serialize lan t)
  (let* ((u (pre-serialize lan t))
         (s (texmacs->utf8raw (stree->tree u))))
    (string-append s "\n<EOF>\n")))

(define (julia-entry)
  (url->system (string->url
    (if (url-exists? "$TEXMACS_HOME_PATH/plugins/julia/bin/julia.jl")
        "$TEXMACS_HOME_PATH/plugins/julia/bin/julia.jl"
        "$TEXMACS_PATH/plugins/julia/bin/julia.jl"))))

(define (julia-launcher)
  (let* ((boot (string-quote (julia-entry)))
         (cmd  (url->system (find-binary-julia))))
    (if (or (os-win32?) (os-mingw?))
        (string-append cmd " " boot)
        (string-append "env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH " cmd " " boot))))

(plugin-configure julia
  (:require (has-binary-julia?))
  (:serializer ,julia-serialize)
  (:launch ,(julia-launcher))
  (:tab-completion #t)
  (:session "Julia"))

(lazy-format (data julia) julia)

(when (supports-julia?)
  (plugin-input-converters julia))
