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

(use-modules (julia julia-binary))

(lazy-format (julia julia-format) julia)

(define (julia-serialize lan t)
  (with u
    (pre-serialize lan t)
    (with s (texmacs->utf8raw (stree->tree u)) (string-append s "\n<EOF>\n"))
  ) ;with
) ;define

(define (julia-entry)
  (url->system (string->url (if (url-exists? "$TEXMACS_HOME_PATH/plugins/julia/bin/julia.jl")
                              "$TEXMACS_HOME_PATH/plugins/julia/bin/julia.jl"
                              "$TEXMACS_PATH/plugins/julia/bin/julia.jl"
                            ) ;if
               ) ;string->url
  ) ;url->system
) ;define

(define (julia-launcher)
  (let* ((boot (string-quote (julia-entry))) (cmd (url->system (find-binary-julia))))
    (if (os-windows?)
      (string-append cmd " " boot)
      (string-append "env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH " cmd " " boot)
    ) ;if
  ) ;let*
) ;define

(plugin-configure julia
  (:require (has-binary-julia?))
  (:serializer ,julia-serialize)
  (:launch ,(julia-launcher))
  (:tab-completion #t)
  (:session "Julia")
) ;plugin-configure

;; 数学输入支持（plugin-input-converters% 组成员）改由
;; utils/plugins/plugin-convert.scm 随 generic 规则一并声明，
;; 避免插件 init 加载时 plugin-convert 尚未预载导致宏未定义
