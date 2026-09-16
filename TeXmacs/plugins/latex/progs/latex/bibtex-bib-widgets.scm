
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : bib-widgets.scm
;; DESCRIPTION : Widgets for bibliography
;; COPYRIGHT   : (C) 2014 Miguel de Benito Delgado
;;                   2026 Yuki Lu
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; TODO:
;;  - Handle external BibTeX.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (latex bibtex-bib-widgets)
  (:use (latex bibtex-bib-complete)
    (latex bibtex-bib-utils)
    (generic document-edit)
  ) ;:use
) ;texmacs-module

(define bibwid-url (string->url ""))

(define bibwid-style "tm-plain")

(define bibwid-default-style "tm-plain")

(define bibwid-use-relative? #t)

(define bibwid-update-buffer? #t)

(define bibwid-buffer (string->url ""))

(define (bibwid-set-url u)
  (cond
   ((and (url-rooted? u) (url-rooted? (url-head bibwid-buffer)))
    (set! bibwid-url (url-delta bibwid-buffer u))
   ) ;
   (else (set! bibwid-url u))
  ) ;cond
) ;define

(define (safe-bib-standard-styles)
  (catch #t (lambda () (bib-standard-styles)) (lambda (key . args) '("tm-plain")))
) ;define

(define (bibwid-set-style answer)
  (let ((styles (safe-bib-standard-styles)))
    (if (and (string? answer) (in? answer styles))
      (set! bibwid-style answer)
      (set! bibwid-style bibwid-default-style)
    ) ;if
  ) ;let
  (refresh-now "bibwid-preview")
) ;define

(define (bibwid-preview-bg-color)
  (if (== (get-preference "gui theme") "liii-night") "#202020" "#ffffff")
) ;define

(define (bibwid-preview-fg-color)
  (if (== (get-preference "gui theme") "liii-night") "#ffffff" "#000000")
) ;define

(define (bibwid-output-content t style)
  (if (tree-is? t 'string)
    `(with ,"bg-color"
       ,(bibwid-preview-bg-color)
       ,"color"
       ,(bibwid-preview-fg-color)
       (mini-paragraph ,"1250px"
         (document ,(replace "Please choose a valid %1 file" "BibTeX"))))
    `(with ,"bg-color"
       ,(bibwid-preview-bg-color)
       ,"color"
       ,(bibwid-preview-fg-color)
       (mini-paragraph ,"1250px" ,(bib-process "bib" style (tree->stree t))))
  ) ;if
) ;define

(define (bibwid-output)
  (let* ((res (bib-to-tree (url->string bibwid-url) bibwid-style))
         (status (car res))
         (t (caddr res))
        ) ;
    (if (== status "valid") t (stree->tree (bibwid-output-content "" bibwid-style)))
  ) ;let*
) ;define

(define (bibwid-insert doit?)
  (when doit?
    (if (not (make-return-after))
      (insert (list 'bibliography "bib" bibwid-style (url->string bibwid-url) '(document ""))
      ) ;insert
    ) ;if
    (if bibwid-update-buffer? (update-document "bibliography"))
  ) ;when
) ;define

(define (bibwid-modify doit?)
  (when doit?
    (with l
      (select (buffer-tree) '(:* bibliography))
      (when (> (length l) 0)
        (with t
          (car l)
          (tree-set! t 1 bibwid-style)
          (tree-set! t 2 (url->string bibwid-url))
          (if bibwid-update-buffer? (update-document "bibliography"))
        ) ;with
      ) ;when
    ) ;with
  ) ;when
) ;define

(define (bibwid-set-filename u)
  (bibwid-set-url u)
  (refresh-now "bibwid-file-input")
  (refresh-now "bibwid-preview")
) ;define

(tm-widget (bibwid-preview)
  (resize '("520px" "520px" "9999px")
    '("100px" "100px" "9999px")
    (scrollable (refreshable "bibwid-preview"
                  (texmacs-output (bibwid-output) '(style "generic"))
                ) ;refreshable
    ) ;scrollable
  ) ;resize
) ;tm-widget

(tm-widget ((bibliography-widget modify? msg) cmd)
  (padded (hlist >>> (text msg) >>>)
    ===
    (hlist (text "File:")
      //
      //
      (refreshable "bibwid-file-input"
        (hlist
          (input
            (when (and answer (!= answer (url->string bibwid-url)))
              (bibwid-set-url (string->url answer))
              (refresh-now "bibwid-preview")
            ) ;when
            "file"
            (list (url->string bibwid-url))
            "40em"
          ) ;input
          //
          //
          (explicit-buttons ("" (choose-file bibwid-set-filename "Choose" "bibtex")))
        ) ;hlist
      ) ;refreshable
    ) ;hlist
    ===
    (hlist (text "Update buffer:")
      //
      (toggle (set! bibwid-update-buffer? answer) bibwid-update-buffer?)
      //
      //
      (text "Style:")
      //
      //
      (verb (enum (bibwid-set-style answer) (safe-bib-standard-styles) bibwid-style "10em")
      ) ;verb
    ) ;hlist
    ===
    (hlist // (dynamic (bibwid-preview)) //)
    ===
    (bottom-buttons >>>
     ("Cancel" (cmd #f))
     //
     //
     (if modify? ("Modify" (cmd #t)))
     (if (not modify?) ("Insert" (cmd #t)))
    ) ;bottom-buttons
  ) ;padded
) ;tm-widget

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 参考文献导入为 TeXmacs 树（用于 tmfs://aux/bib-preview 缓冲区预览）
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bib-has-entries? st)
  (and (list? st)
    (func? st 'document)
    (nnull? (list-filter (cdr st) (lambda (x) (func? x 'bib-entry))))
  ) ;and
) ;define

(tm-define (bib-to-tree file-str style-str)
  (let* ((u-str (if (string? file-str) file-str ""))
         (u (string->url u-str))
         (full-u
           (if
             (and (not (url-rooted? u)) (url-rooted? (url-head (current-buffer))))
             (url-append (url-head (current-buffer)) u)
             u
           ) ;if
         ) ;full-u
         (style (if (and (string? style-str) (!= style-str "")) style-str "tm-plain"))
         (actual-style
           (if (and (>= (string-length style) 3) (== "tm-" (string-take style 3)))
             (string-drop style 3)
             style
           ) ;if
         ) ;actual-style
        ) ;
    (catch #t
      (lambda ()
        (eval
          `(use-modules (latex ,(string->symbol (string-append "bibtex-"
                                                  actual-style))))
        ) ;eval
      ) ;lambda
      (lambda (key . args) (noop))
    ) ;catch
    (cond ((== u-str "") (list "empty" "" (tree "")))
          ((not (url-exists? full-u))
           (let ((msg (translate "File does not exist")))
             (list "not_found" msg (tree ""))
           ) ;let
          ) ;
          (else
            (let* ((t
                     (catch #t
                       (lambda () (parse-bib (string-load full-u)))
                       (lambda (key . args) (tree ""))
                     ) ;catch
                   ) ;t
                   (st (if (tree? t) (tree->stree t) ""))
                  ) ;
              (if (not (bib-has-entries? st))
                (let ((msg (translate "Invalid BibTeX file")))
                  (list "invalid" msg (tree ""))
                ) ;let
                (let* ((content (bibwid-output-content t actual-style)))
                  (list "valid" "" (stree->tree content))
                ) ;let*
              ) ;if
            ) ;let*
          ) ;else
    ) ;cond
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-bibliography-inserter)
  (set! bibwid-buffer (current-buffer))
  (let* ((u (current-bib-file #f))
         (s (current-bib-style #f))
         (modify? (and (not (url-none? u)) (!= s "")))
         (init-url (if modify? u (string->url "")))
         (init-style (if modify? s "tm-plain"))
         (doc-dir (url->string (url-head bibwid-buffer)))
        ) ;
    (if (qt-gui?)
      (let* ((config (list 'bibliography-config
                       (list 'modify? (if modify? "true" "false"))
                       (list 'file (url->string init-url))
                       (list 'style init-style)
                       (list 'update? (if bibwid-update-buffer? "true" "false"))
                       (list 'doc-dir doc-dir)
                       (cons 'styles (safe-bib-standard-styles))
                     ) ;list
             ) ;config
             (result (cpp-bibliography-dialog (stree->tree config)))
             (r (cdr (tree->stree result)))
            ) ;
        (when (nnull? r)
          (let ((file "") (style "tm-plain") (upd? #t))
            (for-each
              (lambda (kv)
                (cond ((== (cadr kv) "file") (set! file (caddr kv)))
                      ((== (cadr kv) "style") (set! style (caddr kv)))
                      ((== (cadr kv) "update") (set! upd? (== (caddr kv) "true")))
                ) ;cond
              ) ;lambda
              r
            ) ;for-each
            (set! bibwid-style style)
            (set! bibwid-update-buffer? upd?)
            (bibwid-set-url (string->url file))
            (if modify? (bibwid-modify #t) (bibwid-insert #t))
          ) ;let
        ) ;when
      ) ;let*
      (with msg
        (if modify?
          (translate "Modifying bibliography in the current document")
          (translate "Inserting bibliography in the current document")
        ) ;if
        (bibwid-set-url init-url)
        (set! bibwid-style init-style)
        (dialogue-window (bibliography-widget modify? msg)
          (if modify? bibwid-modify bibwid-insert)
          (if modify? "Modify bibliography" "Insert bibliography")
        ) ;dialogue-window
      ) ;with
    ) ;if
  ) ;let*
) ;tm-define
