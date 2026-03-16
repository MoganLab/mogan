
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

(texmacs-module (bibtex bib-widgets)
  (:use (bibtex bib-complete) (bibtex bib-utils) (generic document-edit)))

(define bibwid-url (string->url ""))
(define bibwid-style "tm-plain")
(define bibwid-default-style "tm-plain")
(define bibwid-use-relative? #t)
(define bibwid-update-buffer? #t)
(define bibwid-buffer (string->url ""))

(define (bibwid-set-url u)
  (cond ((and (== bibwid-use-relative? #t) (url-rooted? u))
         (set! bibwid-url (url-delta bibwid-buffer u)))
        ((and (== bibwid-use-relative? #f) (not (url-rooted? u)))
         (set! bibwid-url (url-append (url-head bibwid-buffer) u)))
        (else (set! bibwid-url u))))

(define (safe-bib-standard-styles)
  (catch #t (lambda () (bib-standard-styles))
         (lambda (key . args) '("tm-plain"))))

(define (bibwid-set-style answer)
  (let ((styles (safe-bib-standard-styles)))
    (if (and (string? answer) (in? answer styles))
        (set! bibwid-style answer)
        (set! bibwid-style bibwid-default-style)))
  (refresh-now "bibwid-preview"))

(define (bibwid-preview-bg-color)
  (if (== (get-preference "gui theme") "liii-night") "#202020" "#ffffff"))

(define (bibwid-preview-fg-color)
  (if (== (get-preference "gui theme") "liii-night") "#ffffff" "#000000"))

(define (bibwid-output-content t style)
  (if (tree-is? t 'string) 
      `(with "bg-color" ,(bibwid-preview-bg-color)
         "color" ,(bibwid-preview-fg-color)
         (mini-paragraph "1250px"
           (document ,(replace "Please choose a valid %1 file" "BibTeX"))))
      `(with "bg-color" ,(bibwid-preview-bg-color)
         "color" ,(bibwid-preview-fg-color)
         (mini-paragraph "1250px" ,(bib-process "bib" style (tree->stree t))))))

(define (bibwid-output)
  (with style (if (and (>= (string-length bibwid-style) 3)
                       (== "tm-" (string-take bibwid-style 3)))
                  (string-drop bibwid-style 3)
                  bibwid-style)
    (when (== style "")
      (set! style bibwid-default-style))
    (catch #t (lambda () (eval `(use-modules (bibtex ,(string->symbol style)))))
           (lambda (key . args) (noop)))
    (with u (if (and bibwid-use-relative? (not (url-rooted? bibwid-url)))
                (url-append (url-head bibwid-buffer) bibwid-url)
                bibwid-url)
      (with t (if (url-exists? u)
                  (parse-bib (string-load u))
                  (tree ""))
        (stree->tree (bibwid-output-content t style))))))

(define (bibwid-insert doit?)
  (when doit?
    (if (not (make-return-after))
        (insert 
         (list 'bibliography "bib" bibwid-style (url->string bibwid-url)
               '(document ""))))
    (if bibwid-update-buffer? (update-document "bibliography"))))

(define (bibwid-modify doit?)
  (when doit?
    (with l (select (buffer-tree) '(:* bibliography))
      (when (> (length l) 0)
        (with t (car l)
          (tree-set! t 1 bibwid-style)
          (tree-set! t 2 (url->string bibwid-url))
          (if bibwid-update-buffer? (update-document "bibliography")))))))

(define (bibwid-set-filename u)
  (bibwid-set-url u)
  (refresh-now "bibwid-file-input")
  (refresh-now "bibwid-preview"))

(define (bibwid-set-relative val)
  (set! bibwid-use-relative? val)
  (bibwid-set-filename bibwid-url))

(tm-widget (bibwid-preview)
  (resize '("520px" "520px" "9999px") '("100px" "100px" "9999px")
    (scrollable
      (refreshable "bibwid-preview"
        (texmacs-output
         (bibwid-output)
         '(style "generic"))))))

(tm-widget ((bibliography-widget modify? msg) cmd)
  (padded
    (hlist >>> (text msg) >>>)
    ===
    (hlist 
      (text "File:") // //
      (refreshable "bibwid-file-input"
        (hlist
          (input (when (and answer (!= answer (url->string bibwid-url)))
                   (bibwid-set-url (string->url answer))
                   (refresh-now "bibwid-preview"))
                 "file" (list (url->string bibwid-url)) "40em")
          // //
          (explicit-buttons 
            ("" (choose-file bibwid-set-filename "Choose" "tmbib"))))))
    ===
    (hlist
      (text "Use relative path:") //
      (toggle (bibwid-set-relative answer)
              bibwid-use-relative?)
      // //
      (text "Update buffer:") //
      (toggle (set! bibwid-update-buffer? answer)
              bibwid-update-buffer?)
      // //
      (text "Style:") // //
      (verb (enum (bibwid-set-style answer) (safe-bib-standard-styles)
            bibwid-style "10em")))
    ===
    (hlist // (dynamic (bibwid-preview)) //)
    ===
    (bottom-buttons >>>
      ("Cancel" (cmd #f)) // //
      (if modify? ("Modify" (cmd #t)))
      (if (not modify?) ("Insert" (cmd #t))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (open-bibliography-inserter)
  (set! bibwid-buffer (current-buffer))
  (let ((u (current-bib-file #f))
        (s (current-bib-style #f)))
    (if (and (not (url-none? u)) (!= s ""))
        (with msg (translate "Modifying bibliography in the current document")
          (bibwid-set-url u)
          (set! bibwid-style s)
          (dialogue-window (bibliography-widget #t msg)
                           bibwid-modify "Modify bibliography"))
        (with msg (translate "Inserting bibliography in the current document")
          (bibwid-set-url (string->url ""))
          (set! bibwid-style "tm-plain")
          (dialogue-window (bibliography-widget #f msg)
                           bibwid-insert "Insert bibliography")))))
