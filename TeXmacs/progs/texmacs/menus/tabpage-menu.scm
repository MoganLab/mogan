
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

(tm-menu (texmacs-tab-pages)
  (for (view (tabpage-list #t))
    (let* ((buf (view->buffer view))
           (view-win (view->window-of-tabpage view))
           (tab-title (tabpage-display-title buf))
           (doc-path (tabpage-doc-path buf))
          ) ;
      (tab-page (eval view)
       ((balloon (eval `(verbatim ,tab-title)) (eval `(verbatim ,doc-path)))
        (window-set-view view-win view #t)
       ) ;
       ;; #t stansd for focus
       ((balloon "" "Close") (safely-kill-tabpage-by-url view-win view buf))
       ;; active 不进展开树（否则切 tab 让展开树变化、缓存失效、整条重建），
       ;; 改由 Qt 端 updateActiveTab 维护。这里恒为 #f。
       (eval #f)
      ) ;tab-page
    ) ;let*
  ) ;for
) ;tm-menu

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
