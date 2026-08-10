;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : matlab-mode.scm
;; DESCRIPTION : Matlab language mode
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
;;;   This module defines the MATLAB language mode within TeXmacs. It sets up
;;;   the environment to detect when the user is working with MATLAB code and
;;;   provides appropriate mode detection predicates.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义matlab语言模式模块，使用基础的texmacs模式功能
(texmacs-module (code matlab-mode)
  (:use (kernel texmacs tm-modes)))

;;------------------------------------------------------------------------------
;; 模式定义
;;

;; 定义MATLAB相关的模式谓词
;; in-matlab% 检测当前环境是否为MATLAB编程语言环境
;; in-prog-matlab% 检测是否在程序模式下的MATLAB代码环境中
(texmacs-modes
  (in-matlab% (== (get-env "prog-language") "matlab"))
  ;; 判断是否处于matlab代码模式
  (in-prog-matlab% #t in-prog% in-matlab%))