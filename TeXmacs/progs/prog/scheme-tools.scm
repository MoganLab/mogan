
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : scheme-tools.scm
;; DESCRIPTION : Tools for scheme sessions
;; COPYRIGHT   : (C) 2012 Miguel de Benito Delgado
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; The contents of this file are preliminary and simple. Things TO-DO are:
;;  - Use gui:help-window-visible in init-texmacs.scm (or elsewhere)
;;  - this list
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (prog scheme-tools)
  (:use (convert rewrite init-rewrite)
    (doc apidoc-collect)
    (doc apidoc-widgets)
    (kernel texmacs tm-preferences)
    (kernel gui kbd-handlers)
  ) ;:use
) ;texmacs-module

(tm-define char-set:stopmark
  (char-set-adjoin char-set:whitespace #\( #\) #\" #\')
) ;tm-define
(tm-define (list-split lst what)
  (:synopsis "Return a list of lists splitting @lst by items equal? to @what")
  (letrec ((loop (lambda (lst what acc)
                   (cond ((null? lst) (list acc))
                         ((equal? what (car lst)) (cons acc (loop (cdr lst) what '())))
                         (else (loop (cdr lst) what (append acc (list (car lst)))))
                   ) ;cond
                 ) ;lambda
           ) ;loop
          ) ;
    (loop lst what '())
  ) ;letrec
) ;tm-define

(define (prep-math t props)
  (cond ((atomic-tree? t) t)
        ((tree-in? t '(math equation equation*))
         (string->tree (texmacs->latex-document t props))
        ) ;
        ((> (tree-arity t) 0) (tree-map-children (cut prep-math <> props) t))
        (else t)
  ) ;cond
) ;define

(define (sessions->verbatim t)
  (with tx
    (select t '(:* (:or input unfolded-io folded-io) 1))
    (with props
      (acons "texmacs->verbatim:encoding" "SourceCode" '())
      (string-join (map-in-order (lambda (x) (texmacs->verbatim x props))
                     (map-in-order (cut prep-math <> props) tx)
                   ) ;map-in-order
        "\n\n"
      ) ;string-join
    ) ;with
  ) ;with
) ;define
(tm-define (export-sessions url)
  (string-save (sessions->verbatim (buffer-tree)) url)
) ;tm-define
(tm-define (export-selected-sessions url)
  (string-save (sessions->verbatim (selection-tree)) url)
) ;tm-define

(define (string-load-clean file)
  (let* ((str (string-load file))
         (str1 (string-replace str "<" "&lt;"))
         (str2 (string-replace str1 ">" "<gtr>"))
        ) ;
    (string-replace str2 "&lt;" "<less>")
  ) ;let*
) ;define
(tm-define (import-sessions file)
  (let* ((str (string-load-clean file))
         (lst (list-split (string-split str #\newline) ""))
         (lst2 (list-filter lst (lambda (x) (nnull? x))))
         (inputs (map-in-order (lambda (x) `(input ,"Scheme] " (document ,@x))) lst2))
        ) ;
    (insert `(session ,"scheme" ,"default" (document ,@inputs)))
  ) ;let*
) ;tm-define
(tm-define (word-at str pos)
  "Returns the word at @pos in @str, delimited by char-set:stopmark"
  (if (<= pos (string-length str))
    (let* ((beg (string-rindex (substring str 0 pos) char-set:stopmark))
           (end (string-index (substring str pos (string-length str)) char-set:stopmark))
          ) ;
      (if (== end #f) (set! end (string-length str)) (set! end (+ pos end)))
      (if (== beg #f) (set! beg 0) (set! beg (+ 1 beg)))
      (substring str beg end)
    ) ;let*
    ""
  ) ;if
) ;tm-define
(tm-define (cursor-word)
  (:synopsis "Returns the word under the cursor, delimited by char-set:stopmark")
  (with ct (cursor-tree) (word-at (tree->string ct) (car (tree-cursor-path ct))))
) ;tm-define
(tm-define (scheme-popup-help word)
  (:synopsis "Pops up the help window for the scheme symbol @word")
  (help-window "scheme" word)
) ;tm-define
(tm-define (scheme-inbuffer-help word)
  (:synopsis "Opens a help buffer for the scheme symbol @word")
  (load-document (string-append "tmfs://apidoc/type=symbol&what="
                   (string-replace word ":" "%3A")
                 ) ;string-append
  ) ;load-document
) ;tm-define

(define (url-for-symbol s props)
  (with (file line column)
    props
    (if (and file line column)
      (let ((lno (number->string line))
            (cno (number->string column))
            (ss (string-replace s ":" "%3A"))
           ) ;
        (string-append file "?line=" lno "&column=" cno "&select=" ss)
      ) ;let
      (url-none)
    ) ;if
  ) ;with
) ;define
(tm-define (scheme-go-to-definition tmstr)
  (let* ((str (tmstring->string tmstr))
         (sym (string->symbol str))
         (defs (or (symbol-property sym 'defs) '()))
         (urls (map (lambda (x) (url-for-symbol tmstr x)) defs))
        ) ;
    (if (null? urls)
      (set-message "Symbol properties not found" tmstr)
      (go-to-url (list-fold url-or (car urls) (cdr urls)) (cursor-path))
    ) ;if
  ) ;let*
) ;tm-define

(define (get-current-doc-module)
  (let ((tt (select (buffer-tree) '(doc-module-header 0))))
    (if (null? tt) '() (string->module (tree->string (car tt))))
  ) ;let
) ;define

(define (exp-modules)
  (map symbol->string (or (module-exported (get-current-doc-module)) '()))
) ;define
(tm-define (ask-insert-symbol-doc ssym)
  (:argument ssym "Symbol")
  (:proposals ssym (exp-modules))
  (insert ($doc-symbol-template (string->symbol ssym) #t ""))
) ;tm-define
(kbd-map (:require (and developer-mode? (in-tmdoc?)))
 ("M-A-x" (interactive ask-insert-symbol-doc))
) ;kbd-map
(tm-define (run-scheme-file u)
  (:synopsis "Load the file @u into the scheme interpreter")
  (with s
    (url->string u)
    (with run
      (lambda (save?)
        (if save? (buffer-save u))
        (load s)
        (set-message `(replace ,"File %1 was executed" (verbatim ,s)) "")
      ) ;lambda
      (if (and (buffer-exists? u) (buffer-modified? u))
        (user-confirm `(replace ,"File %1 is currently open and modified. Save before running?"
                         (verbatim ,s))
          #t
          run
        ) ;user-confirm
        (run #f)
      ) ;if
    ) ;with
  ) ;with
) ;tm-define

(define cw "")

(define (cmd-click? mods)
  (== (logand mods Mod2Mask) Mod2Mask)
) ;define

(define (opt-click? mods)
  (== (logand mods Mod1Mask) Mod1Mask)
) ;define
(tm-define (mouse-event key x y mods time data)
  (:require (and developer-mode? (opt-click? mods) (in-prog-scheme?)))
  (with short
    (string-take key 4)
    (cond ((== short "pres")
           (mouse-any "release-left" x y 1 (+ time 0.0) data)
           (set! cw (cursor-word))
           (select-word cw (cursor-tree) (cAr (cursor-path)))
          ) ;
          ((== short "rele")
           (with cw2 (cursor-word) (if (== cw cw2) (help-window "scheme" cw)))
          ) ;
          (else (mouse-any key x y mods (+ time 0.0) data))
    ) ;cond
  ) ;with
) ;tm-define
(tm-define (mouse-event key x y mods time data)
  (:require (and developer-mode? (cmd-click? mods) (in-prog-scheme?)))
  (with short
    (string-take key 4)
    (cond ((== short "pres")
           (mouse-any "release-left" x y 1 (+ time 0.0) data)
           (set! cw (cursor-word))
           (select-word cw (cursor-tree) (cAr (cursor-path)))
          ) ;
          ((== short "rele")
           (with cw2 (cursor-word) (if (== cw cw2) (scheme-go-to-definition cw)))
          ) ;
          (else (mouse-any key x y mods (+ time 0.0) data))
    ) ;cond
  ) ;with
) ;tm-define
