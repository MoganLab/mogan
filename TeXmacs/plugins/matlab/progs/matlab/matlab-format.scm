;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : matlab.scm
;; DESCRIPTION : prog format for Matlab
;; COPYRIGHT   : (C) 2022-2025  Darcy Shen, Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;
;;; DESCRIPTION:
;;;   This module defines the format conversion for MATLAB files. It specifies
;;;   how to convert between TeXmacs document tree format and MATLAB source
;;;   code format, allowing users to import and export MATLAB code.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义matlab格式模块
(texmacs-module (matlab matlab-format))

;; ------------------------------------------------------------------------------
;; 格式定义
;;

;; 定义MATLAB格式，指定名称和文件后缀
(define-format matlab (:name "Matlab source code") (:suffix "m"))

;; ------------------------------------------------------------------------------
;; 转换函数定义
;;

;; 定义从TeXmacs格式转换为MATLAB格式的函数

(define (texmacs->matlab x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '()))
) ;define

;; 定义从MATLAB格式转换为TeXmacs格式的函数

(define (matlab->texmacs x . opts)
  (code->texmacs x)
) ;define

;; 定义从MATLAB代码片段转换为TeXmacs格式的函数

(define (matlab-snippet->texmacs x . opts)
  (code-snippet->texmacs x)
) ;define

;; ------------------------------------------------------------------------------
;; 转换器注册
;;

;; 注册TeXmacs文档树到MATLAB文档的转换器
(converter texmacs-tree matlab-document (:function texmacs->matlab))

;; 注册MATLAB文档到TeXmacs文档树的转换器
(converter matlab-document texmacs-tree (:function matlab->texmacs))

;; 注册TeXmacs文档树到MATLAB代码片段的转换器
(converter texmacs-tree matlab-snippet (:function texmacs->matlab))

;; 注册MATLAB代码片段到TeXmacs文档树的转换器
(converter matlab-snippet texmacs-tree (:function matlab-snippet->texmacs))
