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

(use-modules (dynamic session-edit))

(define (julia-serialize lan t)
  (let* ((u (pre-serialize lan t))
         (s (texmacs->code (stree->tree u) "SourceCode")))
    (string-append s "\n<EOF>\n")))

(define (julia-entry)
  (url->system (string->url
    (if (url-exists? "$TEXMACS_HOME_PATH/plugins/julia/julia/MoganJulia.jl")
        "$TEXMACS_HOME_PATH/plugins/julia/julia/MoganJulia.jl"
        "$TEXMACS_PATH/plugins/julia/julia/MoganJulia.jl"))))

(define (julia-launcher)
  (let* ((boot (string-quote (julia-entry))))
    (string-append "julia " boot)))

(plugin-configure julia
  (:winpath "Julia*" "bin")
  (:macpath "Julia*" "Contents/Resources/julia/bin")
  (:require (url-exists-in-path? "julia"))
  (:serializer ,julia-serialize)
  (:launch ,(julia-launcher))
  (:tab-completion #t)
  (:session "Julia"))

(lazy-format (data julia) julia)

(when (supports-julia?)
  (plugin-input-converters julia))
