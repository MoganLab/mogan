;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish.scm
;; DESCRIPTION : prog format for fish
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
;;;   This module defines the format conversion for fish files. It specifies
;;;   how to convert between TeXmacs document tree format and fish source
;;;   code format, allowing users to import and export fish code.
;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; 定义fish格式模块
(texmacs-module (data fish))

;;------------------------------------------------------------------------------
;; 格式定义
;;

;; 定义fish格式，指定名称和文件后缀
(define-format fish
  (:name "fish source code")
  (:suffix "fish"))

;;------------------------------------------------------------------------------
;; 转换函数定义
;;

;; 定义从TeXmacs格式转换为fish格式的函数
(define (texmacs->fish x . opts)
  (texmacs->verbatim x (acons "texmacs->verbatim:encoding" "SourceCode" '())))

;; 定义从fish格式转换为TeXmacs格式的函数
(define (fish->texmacs x . opts)
  (code->texmacs x))

;; 定义从fish代码片段转换为TeXmacs格式的函数
(define (fish-snippet->texmacs x . opts)
  (code-snippet->texmacs x))

;;------------------------------------------------------------------------------
;; 转换器注册
;;

;; 注册TeXmacs文档树到fish文档的转换器
(converter texmacs-tree fish-document
  (:function texmacs->fish))

;; 注册fish文档到TeXmacs文档树的转换器
(converter fish-document texmacs-tree
  (:function fish->texmacs))

;; 注册TeXmacs文档树到fish代码片段的转换器
(converter texmacs-tree fish-snippet
  (:function texmacs->fish))

;; 注册fish代码片段到TeXmacs文档树的转换器
(converter fish-snippet texmacs-tree
  (:function fish-snippet->texmacs))
