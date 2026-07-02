
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : db-menu.scm
;; DESCRIPTION : menus for searching and managing databases
;; COPYRIGHT   : (C) 2015  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (database db-menu)
  (:use (database db-widgets) (generic generic-menu))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Getting and modifying the current database query
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (db-toolbar-current-search)
  (db-get-query-preference (current-buffer) "exact-search" "")
) ;define

(define (db-toolbar-search search)
  (let* ((u-search (cork->utf8 search))
         (keys (compute-keys-string u-search "verbatim"))
         (s (string-recompose keys ","))
        ) ;
    (db-set-query-preference (current-buffer) "exact-search" search)
    (db-set-query-preference (current-buffer) "search" s)
    (revert-buffer-revert)
    (keyboard-focus-on "db-search")
  ) ;let*
) ;define

(define (db-toolbar-current-order)
  (db-get-query-preference (current-buffer) "exact-order" "Name")
) ;define

(define (db-toolbar-order order)
  (with order*
    (string-replace (locase-all order) " " "")
    (db-set-query-preference (current-buffer) "exact-order" order)
    (db-set-query-preference (current-buffer) "order" order*)
    (revert-buffer-revert)
  ) ;with
) ;define

(define (db-toolbar-current-direction)
  (db-get-query-preference (current-buffer) "direction" "ascend")
) ;define

(define (db-toolbar-direction dir)
  (db-set-query-preference (current-buffer) "direction" dir)
  (revert-buffer-revert)
) ;define

(define (db-toolbar-current-limit)
  (db-get-query-preference (current-buffer) "limit" "20")
) ;define

(define (db-toolbar-limit limit)
  (db-set-query-preference (current-buffer) "limit" limit)
  (revert-buffer-revert)
) ;define

(define (db-toolbar-current-present)
  (with p
    (db-get-query-preference (current-buffer) "present" "pretty")
    (upcase-first p)
  ) ;with
) ;define

(define (db-toolbar-present present)
  (with p
    (locase-all present)
    (db-set-query-preference (current-buffer) "present" p)
    (revert-buffer-revert)
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Visibility of the database toolbar
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (db-toolbar-on?)
  (with r
    (db-url? (current-buffer))
    (when (not r)
      (set! toolbar-db-active? #f)
      (update-bottom-tools)
      r
    ) ;when
  ) ;with
) ;define

(tm-define (db-show-toolbar)
  (delayed (:idle 100)
    (set! toolbar-db-active? #t)
    (update-bottom-tools)
    (delayed (:idle 250) (keyboard-focus-on "db-search"))
  ) ;delayed
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The database toolbar
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-widget (db-toolbar)
  (if (db-toolbar-on?)
    (hlist (text "Search: ")
      (input (when answer
               (db-toolbar-search answer)
             ) ;when
        "db-search"
        (list (db-toolbar-current-search))
        "25em"
      ) ;input
      //
      //
      //
      (text "Order: ")
      (with order
        (db-toolbar-current-order)
        (enum (when answer
                (db-toolbar-order answer)
              ) ;when
          (list order "Name" "Author" "Title" "Year" "Year, Name" "")
          order
          "10em"
        ) ;enum
      ) ;with
      (if (== (db-toolbar-current-direction) "ascend")
       ((balloon (icon "tm_similar_previous.xpm") "Reverse ordering")
        (db-toolbar-direction "descend")
       ) ;
      ) ;if
      (if (== (db-toolbar-current-direction) "descend")
       ((balloon (icon "tm_similar_next.xpm") "Reverse ordering")
        (db-toolbar-direction "ascend")
       ) ;
      ) ;if
      //
      //
      //
      (text "Limit: ")
      (enum (when answer
              (db-toolbar-limit answer)
            ) ;when
        (list "1" "2" "5" "10" "20" "50" "100" "1000" "10000" "")
        (db-toolbar-current-limit)
        "4em"
      ) ;enum
      >>
      >>
      >>
      >>
      >>
      >>
      >>
      >>
      (glue #t #f 0 24)
      (enum (when answer
              (db-toolbar-present answer)
            ) ;when
        (list "Detailed" "Folded" "Pretty")
        (db-toolbar-current-present)
        "6em"
      ) ;enum
    ) ;hlist
  ) ;if
) ;tm-widget

(tm-define (load-db-buffer path)
  (load-document (string->url path))
  (db-show-toolbar)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Automatically generated menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (db-get-types) (smart-ref db-kind-table (db-get-kind)))

(tm-menu (insert-entry-menu)
  (with types
    (sort (db-get-types) string<=?)
    (for (type types) ((eval (upcase-first type)) (make-db-entry type)))
    ---
    ("Other" (interactive make-db-entry))
  ) ;with
) ;tm-menu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Customizing the Insert menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind db-extra-menu)

(menu-bind insert-menu
  (:mode in-database?)
  (if (db-get-types) (=> "Database entry" (link insert-entry-menu)))
  (if (not (db-get-types)) ("Database entry" (interactive make-db-entry)))
  (when (and (with-database-tool?) (not (db-url? (current-buffer))))
    (if (not (selection-active-any?))
      (when (tree-innermost db-entry-any?)
        ("Import entry" (db-import-this-entry))
      ) ;when
    ) ;if
    (if (selection-active-any?) ("Import selected entries" (db-import-selection)))
  ) ;when
  (link db-extra-menu)
  (if (or (in-text?) (in-math?)) ---)
  (if (in-text?) (link text-inline-menu))
  (if (in-math?) (link math-insert-menu))
  ---
  (link texmacs-insert-menu)
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Customizing the icons
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind db-extra-icons)

(menu-bind db-insert-icons
  (if (db-get-types)
    (=> (balloon (icon "tm_entry_add.xpm") "Insert a new entry")
      (link insert-entry-menu)
    ) ;=>
  ) ;if
  (if (not (db-get-types))
   ((balloon (icon "tm_entry_add.xpm") "Insert a new entry")
    (interactive make-db-entry)
   ) ;
  ) ;if
  (if (and (with-database-tool?) (not (db-url? (current-buffer))))
    (if (not (selection-active-any?))
      (when (tree-innermost db-entry-any?)
        ((balloon (icon "tm_entry_confirm.xpm") "Import database entry")
         (db-import-this-entry)
        ) ;
      ) ;when
    ) ;if
    (if (selection-active-any?)
     ((balloon (icon "tm_entry_confirm.xpm") "Import selected database entries")
      (db-import-selection)
     ) ;
    ) ;if
  ) ;if
  (if (and (with-database-tool?) (db-url? (current-buffer)))
    (if (not (selection-active-any?))
      (when (tree-innermost db-entry-any?)
        ((balloon (icon "tm_entry_confirm.xpm") "Confirm database entry")
         (kbd-alternate-return)
        ) ;
        ((balloon (icon "tm_entry_remove.xpm") "Remove database entry")
         (structured-remove-left)
        ) ;
      ) ;when
    ) ;if
    (if (selection-active-any?)
     ((balloon (icon "tm_entry_confirm.xpm") "Confirm selected database entries")
      (kbd-alternate-return)
     ) ;
     ((balloon (icon "tm_entry_remove.xpm") "Remove selected database entries")
      (structured-remove-left)
     ) ;
    ) ;if
  ) ;if
  (link db-extra-icons)
) ;menu-bind

(menu-bind texmacs-mode-icons
  (:mode in-database?)
  (link db-insert-icons)
  (if (or (in-text?) (in-math?) (in-prog?)) /)
  (if (in-text?) (link text-inline-icons))
  (if (in-math?) (link math-insert-icons))
  (if (in-prog?) (link prog-format-icons))
  (link texmacs-insert-icons)
) ;menu-bind

(tm-define (alternate-second-icon t)
  (:require (tree-is? t 'db-entry))
  "tm_alternate_both.xpm"
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main database menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind db-entry-menu
  (if (db-get-types) (=> "New entry" (link insert-entry-menu)))
  (if (not (db-get-types)) ("New entry" (interactive make-db-entry)))
  (when (with-database-tool?)
    (if (not (db-url? (current-buffer)))
      (if (not (selection-active-any?))
        (when (tree-innermost db-entry-any?)
          ("Import entry" (db-import-this-entry))
        ) ;when
      ) ;if
      (if (selection-active-any?) ("Import selected entries" (db-import-selection)))
    ) ;if
    (if (db-url? (current-buffer))
      (if (not (selection-active-any?))
        (when (tree-innermost db-entry-any?)
          ("Confirm entry" (kbd-alternate-return))
          ("Remove entry" (structured-remove-left))
        ) ;when
      ) ;if
      (if (selection-active-any?)
       ("Confirm selected entries" (kbd-alternate-return))
       ("Remove selected entries" (structured-remove-left))
      ) ;if
    ) ;if
  ) ;when
) ;menu-bind

(menu-bind db-menu
 ("Open identities" (open-identities))
 ("Open bibliography" (load-db-buffer "tmfs://db/bib/global"))
 (if (supports-gpg?) ("Open key manager" (open-gpg-key-manager)))
 ---
 (when (in-database?)
   (link db-entry-menu)
 ) ;when
 ---
 (when (in-database?)
   (=> "Storage"
     (if (nnull? (recent-databases))
       (with cur
         (user-database)
         (for (db (recent-databases))
          ((check (eval (url->system (url-tail db))) "v" (== db cur)) (use-database db))
         ) ;for
       ) ;with
       ---
     ) ;if
     ("Other" (choose-file use-database "Select database" "generic"))
   ) ;=>
 ) ;when
 (when (db-importable?)
   (if (selection-active-any?) ("Import selected entries" (db-import-selection)))
   (if (and (not (selection-active-any?)) (not (db-url? (current-buffer))))
    ("Import entries in buffer" (db-import-current-buffer))
   ) ;if
   (if (and (not (selection-active-any?))
         (db-url? (current-buffer))
         (null? (db-recent-imports))
       ) ;and
    ("Import" (db-import-select))
   ) ;if
   (if (and (not (selection-active-any?))
         (db-url? (current-buffer))
         (nnull? (db-recent-imports))
       ) ;and
     (=> "Import"
       (for (name (db-recent-imports))
         (let* ((short-name `(verbatim ,(url->system (url-tail name))))
                (long-name `(verbatim ,(url->system name)))
               ) ;
           ((balloon (eval short-name) (eval long-name)) (db-import-file name))
         ) ;let*
       ) ;for
       ---
       ("Other" (db-import-select))
     ) ;=>
   ) ;if
 ) ;when
 (when (db-exportable?)
   (if (selection-active-any?) ("Export selected entries" (db-export-select)))
   (if (and (not (selection-active-any?)) (not (db-url? (current-buffer))))
    ("Export entries in buffer" (db-export-select))
   ) ;if
   (if (and (not (selection-active-any?))
         (db-url? (current-buffer))
         (null? (db-recent-exports))
       ) ;and
    ("Export" (db-export-select))
   ) ;if
   (if (and (not (selection-active-any?))
         (db-url? (current-buffer))
         (nnull? (db-recent-exports))
       ) ;and
     (=> "Export"
       (for (name (db-recent-exports))
         (let* ((short-name `(verbatim ,(url->system (url-tail name))))
                (long-name `(verbatim ,(url->system name)))
               ) ;
           ((balloon (eval short-name) (eval long-name)) (db-export-file name))
         ) ;let*
       ) ;for
       ---
       ("Other" (db-export-select))
     ) ;=>
   ) ;if
 ) ;when
 ---
 ("Preferences" (open-db-preferences))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Focus menus
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (focus-insert-field-menu t make-field)
  (with l
    (db-field-possible-attributes t)
    (for (attr l) ((eval (upcase-first attr)) (make-field attr)))
    ---
    ("Other" (interactive make-field))
  ) ;with
) ;tm-menu

(tm-define (focus-can-insert-remove? t)
  (:require (or (db-entry? t) (db-field? t)))
  #t
) ;tm-define

(tm-menu (focus-insert-menu t)
  (:require (or (db-entry? t) (db-field? t)))
  (when (db-field? t)
    (-> "Insert above" (dynamic (focus-insert-field-menu t make-db-field-before)))
  ) ;when
  (-> "Insert below" (dynamic (focus-insert-field-menu t make-db-field-after)))
  (when (db-field? t)
    ("Remove upwards" (remove-db-field #f))
    ("Remove downwards" (remove-db-field #t))
  ) ;when
) ;tm-menu

(tm-menu (focus-insert-icons t)
  (:require (or (db-entry? t) (db-field? t)))
  (when (db-field? t)
    (=> (balloon (icon "tm_insert_up.xpm") "Structured insert above")
      (dynamic (focus-insert-field-menu t make-db-field-before))
    ) ;=>
  ) ;when
  (=> (balloon (icon "tm_insert_down.xpm") "Structured insert below")
    (dynamic (focus-insert-field-menu t make-db-field-after))
  ) ;=>
  (when (db-field? t)
    ((balloon (icon "tm_delete_up.xpm") "Structured remove upwards")
     (remove-db-field #f)
    ) ;
    ((balloon (icon "tm_delete_down.xpm") "Structured remove downwards")
     (remove-db-field #t)
    ) ;
  ) ;when
) ;tm-menu
