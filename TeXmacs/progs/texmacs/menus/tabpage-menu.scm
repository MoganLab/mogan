
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
  (:use (kernel gui menu-widget)
        (texmacs menus file-menu)))

(import (only (srfi srfi-1) list-index))

(tm-define (move-buffer-to-index buf j)
  (define (transform lst index) (- (length lst) 1 index))
  ;; lst is the reversed buffer list of cpp buffer array
  (let* ((lst  (buffer-menu-unsorted-list 99))
         ;; so we need to transform the index to true index in cpp buffer array
         (from (transform lst (list-index (lambda (x) (== x buf)) lst)))
         (to   (transform lst j)))
      (move-buffer-index from to)))

(tm-menu (texmacs-tab-pages)
  (for (view (tabpage-list #t))
    (let* ((buf (view->buffer view))
           (view-win (view->window-of-tabpage view))
           (title  (buffer-get-title buf))
           (title* (if (== title "") (url->system (url-tail buf)) title))
           ;; 特殊处理启动标签页标题
           (title* (if (== (url->system buf) "tmfs://startup-tab")
                       (if (community-stem?) "Mogan STEM" "Liii STEM")
                       title*))
           (mod?   (buffer-modified? buf))
           (tab-title (string-append title* (if mod? " *" "")))
           (doc-path  (url->system buf))
           (active?   (== (current-view) view)))
      (tab-page
        (eval view)
        ((balloon (eval `(verbatim ,tab-title))
                  (eval `(verbatim ,doc-path)))
         (window-set-view view-win view #t)) ;; #t stansd for focus
        ((balloon "✕" "Close") (safely-kill-tabpage-by-url view-win view buf))
        (eval active?)
      ))))
