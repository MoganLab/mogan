;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish-mode.scm
;; DESCRIPTION : fish language mode
;; COPYRIGHT   : (C) 2025  vesita
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; DESCRIPTION:
;;;   This module defines the fish language mode within TeXmacs. It sets up
;;;   the environment to detect when the user is working with fish code and
;;;   provides appropriate mode detection predicates.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义fish语言模式模块，使用基础的texmacs模式功能
(texmacs-module (code fish-mode)
  (:use (kernel texmacs tm-modes)))

;;------------------------------------------------------------------------------
;; 模式定义
;;

;; 定义fish相关的模式谓词
;; in-fish% 检测当前环境是否为fish编程语言环境
;; in-prog-fish% 检测是否在程序模式下的fish代码环境中
(texmacs-modes
  (in-fish% (== (get-env "prog-language") "fish"))
  ;; 判断是否处于fish代码模式
  (in-prog-fish% #t in-prog% in-fish%))
