
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : document-edit.scm
;; DESCRIPTION : setting global document properties
;; COPYRIGHT   : (C) 2001  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic document-edit)
  (:use (utils base environment)
    (utils library length)
    (utils library cursor)
    (generic generic-edit)
    (generic document-style)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Projects
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-property (project-attach master) (:argument master "file" "Master file"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Preamble mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (in-source-mode?) (== (get-env "preamble") "true"))

(tm-define (toggle-source-mode)
  (:synopsis "Toggle source code editing mode")
  (:check-mark "v" in-source-mode?)
  (let ((new (if (string=? (get-env "preamble") "true") "false" "true")))
    (init-env "preamble" new)
  ) ;let
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Global environment variables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test-default? . vals)
  (if (null? vals)
    #t
    (and (not (init-has? (car vals))) (apply test-default? (cdr vals)))
  ) ;if
) ;tm-define

(tm-define (init-default . args)
  (:check-mark "*" test-default?)
  (for-each init-default-one args)
) ;tm-define

(tm-define (get-init-env s)
  (with t
    (get-init-tree s)
    (cond ((tree-atomic? t) (tree->string t))
          ((and (tree-func? t 'macro 1) (tree-atomic? (tree-ref t 0)))
           (tree->string (tree-ref t 0))
          ) ;
          (else #f)
    ) ;cond
  ) ;with
) ;tm-define

(tm-define (test-init? var val) (== (get-init-tree var) (string->tree val)))

(tm-property (init-env var val) (:check-mark "*" test-init?))

(tm-define (set-init-env s val)
  (with old
    (get-init-tree s)
    (if (and (tree-func? old 'macro 1) (not (tm-is? val 'macro)))
      (init-env-tree s `(macro ,val))
      (init-env-tree s val)
    ) ;if
  ) ;with
) ;tm-define

(tm-define (init-interactive-env var)
  (:interactive #t)
  (interactive (lambda (s) (set-init-env var s))
    (list (or (logic-ref env-var-description% var) var) "string" (get-init-env var))
  ) ;interactive
) ;tm-define

(tm-define (test-init-true? var) (test-init? var "true"))

(tm-define (toggle-init-env var)
  (:check-mark "*" test-init-true?)
  (with new
    (if (== (get-init-env var) "true") "false" "true")
    (init-default var)
    (delayed (when (!= new (get-init-env var)) (set-init-env var new)))
  ) ;with
) ;tm-define

(tm-define (init-multi l)
  (when (and (nnull? l) (nnull? (cdr l)))
    (cond ((and (== (car l) "font") (== (cadr l) :default))
           (remove-font-packages)
           (init-default "font")
          ) ;
          ((== (car l) "font") (init-font (cadr l)))
          ((== (cadr l) :default) (init-default (car l)))
          (else (init-env (car l) (cadr l)))
    ) ;cond
    (init-multi (cddr l))
  ) ;when
) ;tm-define

(tm-define (test-init-font? val . opts)
  (== (font-family-main (get-init "font")) val)
) ;tm-define

(tm-define (remove-font-packages)
  (with l
    (get-style-list)
    (with f
      (list-filter l (lambda (p) (not (string-ends? p "-font"))))
      (set-style-list f)
    ) ;with
  ) ;with
) ;tm-define

(define (font-package-name val)
  (cond ((== val "Fira") "fira-font")
        ((== val "Linux Biolinum") "biolinum-font")
        ((== val "Linux Libertine") "libertine-font")
        (else (string-append val "-font"))
  ) ;cond
) ;define

(tm-define (init-font val . opts)
  (:check-mark "*" test-init-font?)
  (cond ((== val "TeXmacs Computer Modern") (init-font "roman" "roman"))
        ((and (== val "roman") (!= opts (list "roman"))) (init-font "roman" "roman"))
        ((string-starts? val "Stix") (init-font "stix" "math-stix"))
        ((string-starts? val "TeX Gyre Bonum") (init-font "bonum" "math-bonum"))
        ((string-starts? val "TeX Gyre Pagella") (init-font "pagella" "math-pagella"))
        ((string-starts? val "TeX Gyre Schola") (init-font "schola" "math-schola"))
        ((string-starts? val "TeX Gyre Termes") (init-font "termes" "math-termes"))
        (else (init-env "font" val)
          (when (nnull? opts)
            (init-env "math-font" (car opts))
          ) ;when
          (init-env "font-family" "rm")
          (remove-font-packages)
          (with pack
            (font-package-name val)
            (with dir
              "$TEXMACS_PATH/packages/customize/fonts"
              (when (url-exists? (url-append dir (string-append pack ".ts")))
                (init-default "font")
                (init-default "font-family")
                (add-style-package pack)
              ) ;when
            ) ;with
          ) ;with
        ) ;else
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Initial environment management in specific buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (initial-set-tree u var val)
  (when (tm? val)
    (with-buffer u (init-env-tree var val))
  ) ;when
) ;tm-define

(tm-define (initial-set u var val)
  (when (string? val)
    (with-buffer u (init-env var val))
  ) ;when
) ;tm-define

(tm-define (initial-get-tree u var)
  (or (with-buffer u (get-init-tree var)) (tree ""))
) ;tm-define

(tm-define (initial-get u var) (or (with-buffer u (get-init-env var)) ""))

(tm-define (initial-defined? u var) (with-buffer u (style-has? var)))

(tm-define (initial-has? u var) (with-buffer u (init-has? var)))

(tm-define (initial-default u . vars) (with-buffer u (apply init-default vars)))


(tm-define (buffer-get-metadata u kind)
  (or (with-buffer u (get-metadata kind)) "")
) ;tm-define

(tm-define (buffer-has-biblio? u)
  (with-buffer u
    (with l
      (list-attachments)
      (nnull? (list-filter l (cut string-ends? <> "-bibliography")))
    ) ;with
  ) ;with-buffer
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Text and paragraph properties
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (test-default-document-language?)
  (null? (list-intersection (get-style-list) supported-languages))
) ;tm-define

(tm-define (set-default-document-language)
  (:check-mark "*" test-default-document-language?)
  (let* ((old (get-style-list)) (new (list-difference old supported-languages)))
    (when (!= new old)
      (set-style-list new)
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (get-document-language)
  (with l
    (list-intersection (get-style-list) supported-languages)
    (if (null? l) (get-init "language") (car l))
  ) ;with
) ;tm-define

(tm-define (test-document-language? s) (== s (get-document-language)))

(tm-define (set-document-language lan)
  (:check-mark "*" test-document-language?)
  (when (in? lan supported-languages)
    (let* ((old (get-style-list))
           (rem (list-difference old (append supported-languages (list "table-captions-above")))
           ) ;rem
           (nlan (append rem (if (== lan "english") (list) (list lan))))
           (new (append nlan (if (== lan "chinese") (list "table-captions-above") (list))))
          ) ;
      (when (!= new old)
        (set-style-list new)
      ) ;when
    ) ;let*
  ) ;when
) ;tm-define

(define (search-env-var t which)
  (cond ((nlist? t) #f)
        ((null? t) #f)
        ((match? t '(associate "language" :%1)) (caddr t))
        (else (let ((val (search-env-var (car t) which)))
                (if val val (search-env-var (cdr t) which))
              ) ;let
        ) ;else
  ) ;cond
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main page layout
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (test-default-page-medium?)
  (test-default? "page-medium")
) ;define
(tm-define (init-default-page-medium)
  (:check-mark "*" test-default-page-medium?)
  (init-default "page-medium")
  (notify-page-change)
) ;tm-define

(define (test-page-medium? s)
  (== (get-init "page-medium") s)
) ;define
(tm-define (init-page-medium s)
  (:check-mark "*" test-page-medium?)
  (init-env "page-medium" s)
  (notify-page-change)
) ;tm-define

(define (test-default-page-type?)
  (test-default? "page-type" "page-width" "page-height")
) ;define
(tm-define (default-page-type)
  (:check-mark "*" test-default-page-type?)
  (init-default "page-type" "page-width" "page-height")
  (notify-page-change)
) ;tm-define

(define (test-page-type? s)
  (== (get-init "page-type") s)
) ;define
(tm-define (init-page-type s)
  (:check-mark "*" test-page-type?)
  (init-env "page-type" s)
  (init-env "page-width" "auto")
  (init-env "page-height" "auto")
  (notify-page-change)
) ;tm-define

(tm-define (init-page-size w h)
  (:argument w "Page width")
  (:argument h "Page height")
  (init-env "page-type" "user")
  (init-env "page-width" w)
  (init-env "page-height" h)
  (notify-page-change)
) ;tm-define

(define (test-default-page-orientation?)
  (test-default? "page-orientation")
) ;define
(tm-define (init-default-page-orientation)
  (:check-mark "*" test-default-page-orientation?)
  (init-default "page-orientation")
  (notify-page-change)
) ;tm-define

(define (test-page-orientation? s)
  (string=? (get-env "page-orientation") s)
) ;define
(tm-define (init-page-orientation s)
  (:check-mark "*" test-page-orientation?)
  (init-env "page-orientation" s)
  (notify-page-change)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Wrapper for global page rendering
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (panorama-packets)
  (let* ((nr (nr-pages))
         (ww (get-window-width))
         (wh (get-window-height))
         (pw (get-page-width #f))
         (ph (get-page-height #f))
         (best-n 0)
         (best-f 0)
        ) ;
    (for (n (.. 1 (+ nr 1)))
      (let* ((r (quotient (+ nr (- n 1)) n))
             (tw (* n pw))
             (th (* r ph))
             (aw (- ww (* n 5120)))
             (ah (- wh (* r 5120)))
             (fw (/ (* 1.0 aw) tw))
             (fh (/ (* 1.0 ah) th))
             (f (min fw fh))
            ) ;
        (when (or (== n 1) (> f best-f))
          (set! best-n n)
          (set! best-f f)
        ) ;when
      ) ;let*
    ) ;for
    (cond ((> best-n 0) best-n)
          ((> nr 10) 10)
          (else (inexact->exact (ceiling (sqrt (* 1.0 nr)))))
    ) ;cond
  ) ;let*
) ;define

(define (test-default-page-rendering?)
  (test-default? "page-medium")
) ;define
(tm-define (init-default-page-rendering)
  (:check-mark "*" test-default-page-rendering?)
  (init-default "page-medium")
  (init-default "page-border")
  (init-default "page-packet")
  (init-default "page-offset")
  (notify-page-change)
) ;tm-define

(tm-define (get-init-page-rendering)
  (cond ((== (get-init "page-border") "attached") "book")
        ((and (== (get-init "page-packet") "2")
           (== (get-init "page-medium") "paper")
           (== (get-init "page-border") "none")
         ) ;and
         "book"
        ) ;
        ((!= (get-init "page-packet") "1") "panorama")
        ((and (== (get-init "page-medium") "paper") (nnot (tree-innermost 'slideshow)))
         "slideshow"
        ) ;
        (else (get-init "page-medium"))
  ) ;cond
) ;tm-define

(tm-define (page-rendering-label s)
  (cond ((== s "paper") "Single Page")
        ((== s "papyrus") "Continuous Scroll")
        ((== s "automatic") "Screen")
        ((== s "beamer") "Beamer")
        ((== s "book") "Two Page")
        ((== s "panorama") "Panorama")
        ((== s "slideshow") "Slideshow")
        (else s)
  ) ;cond
) ;tm-define

(define (test-page-rendering? s)
  (== (get-init-page-rendering) s)
) ;define
(tm-define (init-page-rendering s)
  (:check-mark "*" test-page-rendering?)
  (when (in? s (list "paper" "papyrus"))
    (set-preference "page medium" s)
  ) ;when
  (save-zoom (get-init-page-rendering))
  (cond ((== s "book")
         (init-env "page-medium" "paper")
         (init-env "page-border" "none")
         (init-env "page-packet" "2")
         (init-env "page-offset" "0")
         (notify-page-change)
         (delayed (:idle 25) (fit-to-screen-width))
        ) ;
        ((== s "panorama")
         (init-env "page-medium" "paper")
         (init-env "page-packet" (number->string (panorama-packets)))
         (init-default "page-border")
         (init-default "page-offset")
         (notify-page-change)
         (delayed (:idle 25) (fit-all-to-screen))
        ) ;
        ((== s "slideshow")
         (init-env "page-medium" "paper")
         (init-default "page-packet")
         (init-default "page-border")
         (init-default "page-offset")
         (notify-page-change)
         (delayed (:idle 25) (restore-zoom "slideshow"))
        ) ;
        (else (init-env "page-medium" s)
          (init-default "page-border")
          (init-default "page-packet")
          (init-default "page-offset")
          (notify-page-change)
          (delayed (:idle 25) (restore-zoom s))
        ) ;else
  ) ;cond
) ;tm-define

(tm-define (initial-get-page-rendering u)
  (with-buffer u (get-init-page-rendering))
) ;tm-define

(tm-define (initial-set-page-rendering u s)
  (with-buffer u (init-page-rendering s))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Further page layout settings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (visible-header-and-footer?)
  (== (get-env "page-show-hf") "true")
) ;define

(tm-define (toggle-visible-header-and-footer)
  (:synopsis "Toggle visibility of headers and footers in 'page' paper mode")
  (:check-mark "v" visible-header-and-footer?)
  (init-env "page-show-hf"
    (if (== (get-env "page-show-hf") "true") "false" "true")
  ) ;init-env
) ;tm-define

(define (page-width-margin?)
  (== (get-env "page-width-margin") "true")
) ;define

(tm-define (toggle-page-width-margin)
  (:synopsis "Toggle mode for determining margins from paragraph width")
  (:check-mark "v" page-width-margin?)
  (init-env "page-width-margin" (if (page-width-margin?) "false" "true"))
) ;tm-define

(define (not-page-screen-margin?)
  (== (get-env "page-screen-margin") "false")
) ;define

(tm-define (toggle-page-screen-margin)
  (:synopsis "Toggle mode for using special margins for screen editing")
  (:check-mark "v" not-page-screen-margin?)
  (init-env "page-screen-margin" (if (not-page-screen-margin?) "true" "false"))
) ;tm-define

(define (reduced-margins?)
  (test-init? "page-odd" "1cm")
) ;define

(tm-define (toggle-reduced-margins)
  (:synopsis "Toggle mode for using reduced margins to save paper")
  (:check-mark "v" reduced-margins?)
  (cond ((has-style-package? "reduced-margins")
         (remove-style-package "reduced-margins")
        ) ;
        ((has-style-package? "normal-margins") (remove-style-package "normal-margins"))
        ((reduced-margins?) (add-style-package "normal-margins"))
        (else (add-style-package "reduced-margins"))
  ) ;cond
) ;tm-define

(define (indent-paragraphs?)
  (nin? (get-init-env "par-first") (list "0fn" "0em" "0tab" "0cm" "0mm" "0in"))
) ;define

(tm-define (toggle-indent-paragraphs)
  (:synopsis "Toggle mode for using a first indentation for each paragraph")
  (:check-mark "v" indent-paragraphs?)
  (cond ((has-style-package? "indent-paragraphs")
         (remove-style-package "indent-paragraphs")
        ) ;
        ((has-style-package? "padded-paragraphs")
         (remove-style-package "padded-paragraphs")
        ) ;
        ((indent-paragraphs?) (add-style-package "padded-paragraphs"))
        (else (add-style-package "indent-paragraphs"))
  ) ;cond
) ;tm-define

(define (no-page-numbers?)
  (test-init? "no-page-numbers" "true")
) ;define

(tm-define (toggle-no-page-numbers)
  (:synopsis "Toggle mode for using standard page numbering")
  (:check-mark "v" no-page-numbers?)
  (cond ((has-style-package? "page-numbers") (remove-style-package "page-numbers"))
        ((has-style-package? "no-page-numbers")
         (remove-style-package "no-page-numbers")
        ) ;
        ((no-page-numbers?) (add-style-package "page-numbers"))
        (else (add-style-package "no-page-numbers"))
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Citing TeXmacs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (document-search-first lab)
  (safe-car (tree-search (buffer-tree) (cut tree-is? <> lab)))
) ;define

(define (tail-document doc)
  (when (and (tm-is? doc 'document)
          (tm-is? (tm-ref doc :last) 'screens)
          (tm-in? (tm-ref doc :last :last) '(shown hidden))
          (tm-is? (tm-ref doc :last :last :last) 'document)
        ) ;and
    (set! doc (tm-ref doc :last :last :last))
  ) ;when
  doc
) ;define

(define (add-biblio)
  (with bib
    (document-search-first 'bibliography)
    (when (and (not bib) (tree-is? (buffer-tree) 'document))
      (with doc
        (tail-document (buffer-tree))
        (tree-insert doc
          (tree-arity doc)
          '((bibliography "bib" "tm-plain" "" (document "")))
        ) ;tree-insert
      ) ;with
    ) ;when
  ) ;with
) ;define

(define (update-biblio)
  (update-document "bibliography")
  (delayed (:idle 1) (update-document "bibliography"))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Document updates
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define doc-update-times 1)

(define (notify-doc-update-times var val)
  (with n
    (cond ((string? val) (or (string->number val) 1))
          ((number? val) val)
          (else 1)
    ) ;cond
    (set! doc-update-times (min (max 1 n) 5))
  ) ;with
) ;define

(define-preferences ("document update times" "3" notify-doc-update-times))

(define (wait-update-current-buffer)
  (set-message "Updating current buffer ..." "please wait")
  (update-current-buffer)
) ;define

(tm-define (update-document what)
  (set-cursor-style "wait")
  (for (.. 0 doc-update-times)
    (delayed (:idle 1)
      (cursor-after (cond ((== what "all")
                           (generate-all-aux)
                           (inclusions-gc)
                           (picture-gc)
                           (wait-update-current-buffer)
                          ) ;
                          ((== what "bibliography") (generate-all-aux) (wait-update-current-buffer))
                          ((== what "buffer") (wait-update-current-buffer))
                          (else (generate-aux what))
                    ) ;cond
      ) ;cursor-after
    ) ;delayed
  ) ;for
  (delayed (:idle 1) (set-cursor-style "normal"))
) ;tm-define
