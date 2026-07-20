
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-autosave.scm
;; DESCRIPTION : Initialize the 'autosave' plugin
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;                   2026  Darcy Shen
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(use-modules (binary goldfish))
(use-modules (plugin autosave))
(import (liii path))

(define (autosave-serialize lan t)
  (with u
    (pre-serialize lan t)
    (with s (texmacs->utf8raw (stree->tree u)) (string-append s "\n<EOF>\n"))
  ) ;with
) ;define

(define (launcher)
  (let* ((user "$TEXMACS_HOME_PATH/plugins/autosave/goldfish/tm-autosave.scm")
         (sys "$TEXMACS_PATH/plugins/autosave/goldfish/tm-autosave.scm")
         (entry (if (url-exists? user) user sys))
        ) ;
    (string-append (string-quote (url->system (find-binary-goldfish)))
      " "
      (string-quote (url->system entry))
    ) ;string-append
  ) ;let*
) ;define

(plugin-configure autosave
  (:require (has-binary-goldfish?))
  (:launch ,(launcher))
  (:serializer ,autosave-serialize)
) ;plugin-configure

;; 定时自动保存由插件初始化时启动（插件在事件循环启动 ~3s 后懒加载，
;; 因此首次触发约在启动后 123s，与原先 120s 基本一致）
(delayed (:pause 120000) (autosave-delayed))
