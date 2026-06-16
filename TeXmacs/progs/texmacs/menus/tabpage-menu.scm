
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

(tm-menu (texmacs-tab-pages)
  (for (view (tabpage-list #t))
    (let* ((buf (view->buffer view))
           (view-win (view->window-of-tabpage view))
           (title (buffer-get-title buf))
           (title* (if (== title "") (url->system (url-tail buf)) title))
           (is-startup? (== (utf8->cork (url->system buf)) "tmfs://startup-tab"))
           ;; 特殊处理启动标签页标题
           (title* (if is-startup? (if (community-stem?) "Mogan STEM" "Liii STEM") title*))
           (mod? (buffer-modified? buf))
           (tab-title (string-append title* (if mod? " *" "")))
           (doc-path (if is-startup? "" (utf8->cork (url->system buf))))
           (active? (== (current-view) view))
          ) ;
      (tab-page (eval view)
       ((balloon (eval `(verbatim ,tab-title)) (eval `(verbatim ,doc-path)))
        (window-set-view view-win view #t)
       ) ;
       ;; #t stansd for focus
       ((balloon "" "Close") (safely-kill-tabpage-by-url view-win view buf))
       (eval active?)
      ) ;tab-page
    ) ;let*
  ) ;for
) ;tm-menu
