
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tabpage-menu.scm
;; DESCRIPTION : The tab bar below the main icon bar
;; COPYRIGHT   : (C) 2024 Zhenjun Guo
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs menus tabpage-menu)
  (:use (kernel gui menu-widget) (texmacs menus file-menu))
) ;texmacs-module

(import (only (srfi srfi-1) list-index))

(tm-define (move-buffer-to-index buf j)
  (define (transform lst index)
    (- (length lst) 1 index)
  ) ;define
  ;; lst is the reversed buffer list of cpp buffer array
  (let* ((lst (buffer-menu-unsorted-list 99))
         ;; so we need to transform the index to true index in cpp buffer array
         (from (transform lst (list-index (lambda (x) (== x buf)) lst)))
         (to (transform lst j))
        ) ;
    (move-buffer-index from to)
  ) ;let*
) ;tm-define

;; tab 显示标题（含修改标记 *）。菜单与签名共用，避免逻辑重复。

(define (tabpage-display-title buf)
  (let* ((title (buffer-get-title buf))
         (title* (if (== title "") (url->system (url-tail buf)) title))
         (is-startup? (== (utf8->cork (url->system buf)) "tmfs://startup-tab"))
         (title* (if is-startup? (if (community-stem?) "Mogan STEM" "Liii STEM") title*))
         (mod? (buffer-modified? buf))
        ) ;
    (string-append title* (if mod? " *" ""))
  ) ;let*
) ;define

(define (tabpage-doc-path buf)
  (let ((is-startup? (== (utf8->cork (url->system buf)) "tmfs://startup-tab")))
    (if is-startup? "" (utf8->cork (url->system buf)))
  ) ;let
) ;define

;; tab 栏不经 tm-menu：直接用 widget 原语构建整栏。
;; title/close 子 widget 仍复用 kernel 的 make-menu-items（widget 构造器，
;; 非 tm-menu），参数 style=0、bar?=#t 与旧 make-tab-page 一致，
;; 保证 Qt 端拿到的 widget 树形状不变。
;; active 恒为 #f：不进展开树（否则切 tab 让树变化、签名失效、整条重建），
;; active 高亮由 Qt 端 updateActiveTab 维护。

(define (tabpage->widget view)
  (let* ((buf (view->buffer view))
         (view-win (view->window-of-tabpage view))
         (tab-title (tabpage-display-title buf))
         (doc-path (tabpage-doc-path buf))
         (title (list (list 'balloon (list 'verbatim tab-title) (list 'verbatim doc-path))
                  (lambda () (window-set-view view-win view #t))
                ) ;list
         ) ;title
         (close-btn (list (list 'balloon "" "Close")
                      (lambda () (safely-kill-tabpage-by-url view-win view buf))
                    ) ;list
         ) ;close-btn
        ) ;
    (widget-tab-page view
      (car (make-menu-items title 0 #t))
      (car (make-menu-items close-btn 0 #t))
      #f
    ) ;widget-tab-page
  ) ;let*
) ;define

(tm-define (texmacs-tab-pages)
  (widget-hmenu (map tabpage->widget (tabpage-list #t)))
) ;tm-define

;; tab 栏稳定签名：view-url + 显示标题序列（不含 lambda/command）。
;; get_menu_widget 对 which==4 用它判等——menu-expand 后的 xmenu 含每次新建的
;; lambda，equal 无法比较；切 tab 时签名不变 => 跳过重建。

(define (tabpage-entry-signature view)
  (string-append (object->string view)
    "\n"
    (tabpage-display-title (view->buffer view))
  ) ;string-append
) ;define

(tm-define (tabpage-menu-signature)
  (apply string-append (map tabpage-entry-signature (tabpage-list #t)))
) ;tm-define
