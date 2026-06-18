
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : bib-manage.scm
;; DESCRIPTION : global bibliography management
;; COPYRIGHT   : (C) 2015  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (database bib-manage) (:use (database bib-db)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Caching existing BibTeX files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define bib-dir "$TEXMACS_HOME_PATH/system/database")

(define bib-cache-dir (string-append bib-dir "/bib"))

(define bib-master (url->url (string-append bib-dir "/bib-master.tmdb")))

(define (bib-cache-id f)
  (with-database bib-master
    (let* ((s (url->system f)) (l (db-search (list (list "source" s)))))
      (and (== (length l) 1) (car l))
    ) ;let*
  ) ;with-database
) ;define

(define (bib-cache-stamp f)
  (and-with id
    (bib-cache-id f)
    (with-database bib-master (db-get-field-first id "stamp" #f))
  ) ;and-with
) ;define

(define (bib-cache-imported f)
  (and-with id
    (bib-cache-id f)
    (with-database bib-master (system->url (db-get-field-first id "target" #f)))
  ) ;and-with
) ;define

(define (bib-cache-up-to-date? f)
  (and-with stamp
    (bib-cache-stamp f)
    (and (url-exists? f) (== (number->string (url-last-modified f)) stamp))
  ) ;and-with
) ;define

(define (bib-cache-acknowledge id f imported)
  (when (url-exists? imported)
    (with-database bib-master
      (with stamp
        (number->string (url-last-modified f))
        (db-set-field id "source" (list (url->system f)))
        (db-set-field id "target" (list (url->system imported)))
        (db-set-field id "stamp" (list stamp))
      ) ;with
    ) ;with-database
  ) ;when
) ;define

(define (convert-tmbib s)
  (system-wait "Converting BibTeX file" "please wait")
  (tmbib-document->texmacs* s)
) ;define

(define (bib-cache-create f)
  (with-global db-bib-origin
    (url->string (url-tail f))
    (let* ((bib-doc (string-load f))
           (t (convert-tmbib bib-doc))
           (tm-doc (convert t "texmacs-stree" "texmacs-document"))
           (id (with-database bib-master (db-create-id)))
           (dupl (url->url (string-append bib-cache-dir "/" id ".bib")))
           (imported (url->url (string-append bib-cache-dir "/" id ".tm")))
          ) ;
      (string-save bib-doc dupl)
      (string-save tm-doc imported)
      (bib-cache-acknowledge id f imported)
    ) ;let*
  ) ;with-global
) ;define

(define (bib-cache-update f)
  (with-global db-bib-origin
    (url->string (url-tail f))
    (let* ((id (bib-cache-id f))
           (dupl (url->url (string-append bib-cache-dir "/" id ".bib")))
           (imported (url->url (string-append bib-cache-dir "/" id ".tm")))
           (old-s (string-load dupl))
           (old-doc (string-load imported))
           (old-t (convert old-doc "texmacs-document" "texmacs-stree"))
           (old-body (tmfile-extract old-t 'body))
           (new-s (string-load f))
           ;; (dummy
           ;; (begin
           ;;   (display* "---------------------------\n")
           ;;   (display* "old-s= " old-s "\n")
           ;;   (display* "---------------------------\n")
           ;;   (display* "old-body= " old-body "\n")
           ;;   (display* "---------------------------\n")
           ;;   (display* "new-s= " new-s "\n")
           ;;   (display* "---------------------------\n")))
           (new-body (conservative-bib-import old-s old-body new-s))
           ;; (d2 (display* "new-body= " (tm->stree new-body) "\n"))
           (new-t `(document (TeXmacs ,(texmacs-version))
                     (style "database-bib")
                     (body ,new-body))
           ) ;new-t
           (new-doc (convert new-t "texmacs-stree" "texmacs-document"))
          ) ;
      (string-save new-s dupl)
      (string-save new-doc imported)
      (bib-cache-acknowledge id f imported)
    ) ;let*
  ) ;with-global
) ;define

(tm-define (bib-cache-bibtex f)
  (cond ((not (bib-cache-id f)) (bib-cache-create f))
        ((not (bib-cache-up-to-date? f)) (bib-cache-update f))
  ) ;cond
  (when (not (bib-cache-id f))
    (texmacs-error "failed to create bibliographic database" "bib-cache-bibtex")
  ) ;when
  (and-with id
    (bib-cache-id f)
    (url->url (string-append bib-cache-dir "/" id ".tm"))
  ) ;and-with
) ;tm-define

(tm-define (bib-cache-database f names)
  (and-with imported
    (bib-cache-bibtex f)
    (and-with id
      (bib-cache-id f)
      (let* ((doc (string-load imported))
             (t (convert doc "texmacs-document" "texmacs-stree"))
             (body* (tmfile-extract t 'body))
             (body (if (tm-compound? body*) body* '(document)))
             (db (url->url (string-append bib-cache-dir "/" id ".tmdb")))
             (h (list->ahash-set names))
             (ok? (lambda (e) (and (ahash-ref h (tm-ref e 2)) (db-entry? e))))
             (l (list-filter (tm-children body) ok?))
            ) ;
        (with-database db (bib-save `(document ,@l)))
        db
      ) ;let*
    ) ;and-with
  ) ;and-with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Importing BibTeX files or entries
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bib-entry? t)
  (or (tm-func? t 'bib-entry 3)
    (and (db-entry-any? t)
      (tm-atomic? (tm-ref t 1))
      (in? (tm->string (tm-ref t 1)) bib-types-list)
    ) ;and
  ) ;or
) ;define

(tm-define (contains-bib-entry? t)
  (or (bib-entry? t)
    (and (tm-func? t 'document) (list-or (map contains-bib-entry? (tm-children t))))
  ) ;or
) ;tm-define

(define (bib-import-tree doc)
  (with-database (bib-database) (bib-save doc))
  (when (db-url? (current-buffer))
    (revert-buffer-revert)
  ) ;when
) ;define

(define (bib-confirm-tree doc)
  (with-database (bib-database) (db-confirm-entries-in doc))
) ;define

(tm-define (bib-import-bibtex f)
  (with imported
    (bib-cache-bibtex f)
    (when (url-exists? imported)
      (let* ((tm-doc (string-load imported))
             (t (convert tm-doc "texmacs-document" "texmacs-stree"))
             (body (tmfile-extract t 'body))
            ) ;
        (bib-import-tree body)
        (set-message "Imported bibliographic entries" "import bibliography")
        (db-confirm-imported f)
      ) ;let*
    ) ;when
  ) ;with
) ;tm-define

(tm-define (bib-import-selection)
  (when (selection-active-any?)
    (bib-confirm-tree (selection-tree))
    (selection-cancel)
  ) ;when
) ;tm-define

(tm-define (bib-import-current-buffer)
  (when (and (in-bib?) (not (db-url? (current-buffer))))
    (bib-import-tree (buffer-tree))
  ) ;when
) ;tm-define

(tm-define (bib-importable?)
  (cond ((selection-active-any?) (contains-bib-entry? (selection-tree)))
        ((and (in-bib?) (db-url? (current-buffer))) #t)
        ((and (in-bib?) (not (db-url? (current-buffer))))
         (contains-bib-entry? (buffer-tree))
        ) ;
        (else #f)
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Exporting BibTeX files or entries
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (bib-export-global f)
  (with-database (bib-database)
    (with all
      (bib-load)
      (when (and all (tm-func? all 'document))
        (let* ((doc `(document ,@(map db->bib (cdr all))))
               (bibtex-doc (convert doc "texmacs-stree" "bibtex-document"))
              ) ;
          (string-save bibtex-doc f)
        ) ;let*
      ) ;when
    ) ;with
  ) ;with-database
) ;tm-define

(define (bib-export-save new-body f)
  (with s
    (or (and (url-exists? f)
          (let* ((imported (bib-cache-bibtex f))
                 (old-s (string-load f))
                 (doc (string-load imported))
                 (t (convert doc "texmacs-document" "texmacs-stree"))
                 (body (tmfile-extract t 'body))
                ) ;
            (and body (conservative-bib-export body old-s new-body))
          ) ;let*
        ) ;and
      (convert new-body "texmacs-stree" "bibtex-document")
    ) ;or
    (string-save s f)
    (set-message "Exported bibliographic entries" "export bibliography")
  ) ;with
) ;define

(tm-define (bib-export-tree f t)
  (when (tm-func? t 'document)
    (let* ((l1 (list-filter (tm-children t) bib-entry?))
           (l2 (map tm->stree l1))
           (l3 (map (lambda (x) (if (tm-func? x 'bib-entry 3) x (db->bib x))) l2))
           (doc `(document ,@l3))
          ) ;
      (bib-export-save doc f)
    ) ;let*
  ) ;when
) ;tm-define

(tm-define (bib-export-all f)
  (with-database (bib-database)
    (let* ((l (db-search (list (cons "type" bib-types-list) (list :order "name" #t))))
           (i (map db-load-entry l))
           (doc `(document ,@i))
          ) ;
      (bib-export-save doc f)
      (db-confirm-exported f)
    ) ;let*
  ) ;with-database
) ;tm-define

(tm-define (bib-exportable?)
  (cond ((selection-active-any?) (contains-bib-entry? (selection-tree)))
        ((and (in-bib?) (db-url? (current-buffer))) #t)
        ((and (in-bib?) (not (db-url? (current-buffer))))
         (contains-bib-entry? (buffer-tree))
        ) ;
        ((nnull? (bib-attachments #f)) #t)
        (else #f)
  ) ;cond
) ;tm-define

(tm-define (bib-export-bibtex f)
  (cond ((selection-active-any?) (bib-export-tree f (tm->stree (selection-tree))))
        ((and (in-bib?) (db-url? (current-buffer))) (bib-export-all f))
        ((and (in-bib?) (not (db-url? (current-buffer))))
         (bib-export-tree f (tm->stree (buffer-tree)))
        ) ;
        ((nnull? (bib-attachments #f)) (bib-export-attachments f))
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Retrieving entries
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bib-retrieve-one name)
  (and-with l
    (db-search (list (list "name" name)))
    (when (> (length l) 1)
      (with l*
        (db-search (list (list "name" name) (list "contributor" (get-default-user))))
        (when (pair? l*)
          (set! l l*)
        ) ;when
      ) ;with
    ) ;when
    (and (nnull? l) (with e (db-load-entry (car l)) (cons name e)))
  ) ;and-with
) ;define

(define (bib-retrieve-several names)
  (if (null? names)
    (list)
    (let* ((head (bib-retrieve-one (car names)))
           (tail (bib-retrieve-several (cdr names)))
          ) ;
      (if head (cons head tail) tail)
    ) ;let*
  ) ;if
) ;define

(define (bib-retrieve-attached names local?)
  (let* ((l (bib-attached-entries local?))
         (t (make-ahash-table))
         (get (lambda (name) (and-with val (ahash-ref t name) (cons name val))))
        ) ;
    (for (e l) (ahash-set! t (tm-ref e 2) e))
    (list-filter (map get names) identity)
  ) ;let*
) ;define

(define (bib-retrieve-entries-from-one names db)
  (cond ((== db :local) (bib-retrieve-attached names #t))
        ((== db :attached) (bib-retrieve-attached names #f))
        (else (with-database db (bib-retrieve-several names)))
  ) ;cond
) ;define

(define (bib-retrieve-entries-from names dbs)
  (if (null? dbs)
    (list)
    (let* ((r (bib-retrieve-entries-from-one names (car dbs)))
           (done (map car r))
           (remaining (list-difference names done))
          ) ;
      (append r (bib-retrieve-entries-from remaining (cdr dbs)))
    ) ;let*
  ) ;if
) ;define

(define (bib-get-db bib-file names)
  (cond ((== bib-file :default) (bib-database))
        ((== bib-file :local) :local)
        ((== bib-file :attached) :attached)
        ((== (url-suffix bib-file) "tmdb") (url->url bib-file))
        ((== (url->string bib-file) "tmfs://.bib") :local)
        (else (bib-cache-database bib-file names))
  ) ;cond
) ;define

(tm-define (bib-retrieve-entries names . bib-files)
  (set! names (list-remove-duplicates names))
  (if (null? names)
    names
    (with l
      (list-filter (map (cut bib-get-db <> names) bib-files) identity)
      (bib-retrieve-entries-from names l)
    ) ;with
  ) ;if
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Running bibtex or its internal replacement
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (bib-generate prefix style doc)
  (with m
    `(bibtex ,(string->symbol style))
    (module-provide m)
    (bib-process prefix style doc)
  ) ;with
) ;tm-define

(define (bib-difference l1 l2)
  (with t
    (list->ahash-set (map car l2))
    (list-filter l1 (lambda (x) (not (ahash-ref t (car x)))))
  ) ;with
) ;define

(define (bib-file? f)
  (and (url? f) (== (url-suffix f) "bib"))
) ;define

(define (bib-warning msg)
  (debug-message "bibtex-warning" msg)
) ;define

(define (bib-compile-sub prefix style names . bib-files)
  (set! names (list-remove-duplicates names))
  (when (and (not (supports-bibtex?)) (nin? style (bib-standard-styles)))
    (bib-warning "bibtex has not been installed on your system\n")
    (bib-warning "  using integrated replacement instead\n")
    (set! style (string-append "tm-" style))
    (when (nin? style (bib-standard-styles))
      (bib-warning "  switching to tm-plain style\n")
      (set! style "tm-plain")
    ) ;when
  ) ;when
  (if (in? style (bib-standard-styles))
    (let* ((all-files `(,:local ,@bib-files ,:default ,:attached))
           (l (apply bib-retrieve-entries (cons names all-files)))
           (bl (map db->bib (map cdr l)))
           (doc `(document ,@bl))
          ) ;
      (bib-generate prefix (string-drop style 3) doc)
    ) ;let*
    (receive (b1 b2)
      (list-partition `(,:local ,@bib-files ,:default ,:attached) bib-file?)
      (let* ((l1 (apply bib-retrieve-entries (cons names b1)))
             (names2 (list-difference names (map car l1)))
             (l2 (apply bib-retrieve-entries (cons names2 b2)))
             (bl2 (map db->bib (map cdr l2)))
             (doc2 `(document ,@bl2))
             (bib-docs (map string-load b1))
             (xdoc (convert doc2 "texmacs-stree" "bibtex-document"))
             (all-docs (append bib-docs (list "\n") (list xdoc)))
             (full-doc (apply string-append all-docs))
             (auto (url->url "$TEXMACS_HOME_PATH/system/bib/auto.bib"))
            ) ;
        ;; (display* auto "\n-----------------------------\n" full-doc "\n")
        (string-save full-doc auto)
        (bibtex-run prefix style auto names)
      ) ;let*
    ) ;receive
  ) ;if
) ;define

(tm-define (bib-compile prefix style names . bib-files)
  (when (and (tm? names) (tm-func? names 'document))
    (set! names (tm-children (tm->stree names)))
  ) ;when
  ;; (display* "Compile " style ", " names ", " bib-files "\n")
  (cond ((and (not (supports-db?))
           (not (and (list-1? bib-files)
                  (== (url->string (url-tail (car bib-files))) "texmacs.bib")
                ) ;and
           ) ;not
         ) ;and
         (tree "Error: database tool not activated")
        ) ;
        ((not (and (list? names) (list-and (map string? names))))
         (tree "Error: invalid bibliographic key list")
        ) ;
        (else (with t
                (apply bib-compile-sub (cons* prefix style names bib-files))
                (if (not (tm? t)) (tree "Error: failed to produce bibliography") (tm->tree t))
              ) ;with
        ) ;else
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Pretty printing of bibliography entries
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (extract-label t)
  (cond ((tm-func? t 'label 1) (tm-ref t 0))
        ((pair? t) (or (extract-label (car t)) (extract-label (cdr t))))
        (else #f)
  ) ;cond
) ;define

(define (remove-label t)
  (cond ((tm-func? t 'bibitem*) "")
        ((tm-func? t 'label 1) "")
        ((tm-func? t 'concat) (apply tmconcat (map remove-label (tm-children t))))
        (else t)
  ) ;cond
) ;define

(define (rewrite-bibitem t)
  (let* ((lab (extract-label t))
         (t* (remove-label t))
         (lab* (if (and (string? lab) (string-starts? lab "bib-")) (string-drop lab 4) "?")
         ) ;lab*
        ) ;
    `(db-result ,lab* ,t*)
  ) ;let*
) ;define

(tm-define (db-pretty l kind fm)
  (:require (and (== kind "bib") (== fm :pretty)))
  (let* ((bib (map db->bib l))
         (doc `(document ,@bib))
         (gen (bib-generate "bib" "siam" doc))
        ) ;
    (when (tm-func? gen 'bib-list)
      (set! gen (tm-ref gen :last))
    ) ;when
    (with r
      (if (tm-func? gen 'document) (tm-children gen) (list gen))
      (map rewrite-bibitem r)
    ) ;with
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Attaching the bibliography to the current document and automatic importation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (bib-attach prefix names . bib-files)
  (when (supports-db?)
    (when (and (tm? names) (tm-func? names 'document))
      (set! names (tm-children (tm->stree names)))
    ) ;when
    (when (and (list? names) (list-and (map string? names)))
      (set! names (list-remove-duplicates names))
      (let* ((all-files `(,:local ,@bib-files ,:default ,:attached))
             (l (apply bib-retrieve-entries (cons names all-files)))
             (doc `(document ,@(map cdr l)))
            ) ;
        (set-attachment (string-append prefix "-bibliography") doc)
      ) ;let*
    ) ;when
  ) ;when
) ;tm-define

(tm-define (bib-attachments local?)
  (with suffix
    (if local? "-biblio" "-bibliography")
    (with l (list-attachments) (list-filter l (cut string-ends? <> suffix)))
  ) ;with
) ;tm-define

(define (bib-attached-entries local?)
  (let* ((l (bib-attachments local?)) (bibs (map tm->stree (map get-attachment l))))
    (append-map (lambda (x) (if (string? x) (list) (tm-children x))) bibs)
  ) ;let*
) ;define

(tm-define (bib-export-attachments f)
  (let* ((b (bib-attached-entries #f))
         (doc `(document ,@(map db->bib b)))
         (bibtex-doc (convert doc "texmacs-stree" "bibtex-document"))
        ) ;
    (string-save bibtex-doc f)
    (set-message "Exported bibliographic references" "export bibliography")
  ) ;let*
) ;tm-define

(tm-define (notify-set-attachment name key val)
  (when (get-boolean-preference "auto bib import")
    (when (supports-db?)
      (when (string-ends? key "-bibliography")
        (with doc (tm->stree val) (with-database (bib-database) (bib-save doc)))
      ) ;when
    ) ;when
  ) ;when
  (former name key val)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Using the bibliographic database for the GUI
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (db-importable?) (:require (bib-importable?)) #t)

(tm-define (db-exportable?) (:require (bib-exportable?)) #t)

(tm-define (db-import-file name) (:require (in-bib?)) (bib-import-bibtex name))

(tm-define (db-export-file name) (:require (in-bib?)) (bib-export-bibtex name))

(tm-define (db-import-select)
  (:require (in-bib?))
  (choose-file bib-import-bibtex "Import from BibTeX file" "tmbib")
) ;tm-define

(tm-define (db-export-select)
  (:require (bib-exportable?))
  (choose-file bib-export-bibtex "Export to BibTeX file" "tmbib")
) ;tm-define

(tm-define (db-import-selection)
  (:require (bib-importable?))
  (bib-import-selection)
) ;tm-define

(tm-define (db-import-this-entry)
  (:require (bib-importable?))
  (and-with t (tree-innermost db-entry-any?) (bib-import-tree t))
) ;tm-define

(tm-define (db-import-current-buffer)
  (:require (bib-importable?))
  (bib-import-current-buffer)
) ;tm-define

(tm-define (open-bib-chooser cb)
  (open-db-chooser (bib-database) "bib" "Search bibliographic reference" cb)
) ;tm-define
