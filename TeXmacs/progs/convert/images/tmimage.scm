;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tmimage.scm
;; DESCRIPTION : convert texmacs fragment (selection) to image formats.
;; COPYRIGHT   : (C) 2012-2022  Philippe Joyez
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; convert active selection to various graphics format
;; If target is svg embed texmacs code of the selection in the image for
;; re-edition (Could be done for other formats too).
;; the svg produced by this method can be pasted in inkscape via the clipboard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (convert images tmimage) (:use (convert tmml tmmlout)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; private functions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (url-temp-ext ext)
  ;; temporary files with extensions
  (url-glue (url-temp) (string-append "." ext))
) ;define

(if (not (defined? 'string-contains))
  (define (string-contains ss s)
    (string-position s ss)
  ) ;define
) ;if

(define (debug . args)
  (when (debug-get "convert")
    (apply display* args)
  ) ;when
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; commodity functions for tree manipulations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (remove-node! node)
  ;; removes node (with children if any)
  (tree-remove! (tree-ref node :up) (tree-index node) 1)
) ;define

(define (copy-node! node parent-dest pos)
  ;; insert an existing node (with children if any) as new child of parent-dest
  ;; FIXME: No sanity check! parent should not be in node's subtree!
  (tree-insert! parent-dest pos `(,node))
) ;define

(define (move-node! node parent-dest pos)
  ;; moves an existing node (with children if any) and
  ;; insert it as new child of parent-dest
  ;; FIXME: No sanity check! parent should not be in node's subtree!
  (copy-node! node parent-dest pos)
  (remove-node! node)
) ;define

(define (selection-trim-ending)
  (if (selection-active-any?)
    (with st
      (selection-tree)
      (if (and (not (tree-atomic? st)) (tree-empty? (tree-ref st :last)))
        (begin
          (selection-set (selection-get-start)
            (path-previous (root-tree) (selection-get-end))
          ) ;selection-set
          (selection-trim-ending)
        ) ;begin
      ) ;if
    ) ;with
  ) ;if
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main functions needed for making reeditable svg
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; define latex and texmacs string representation of selection
;; we escape them to ascii so that they do not interfere with xml
;; <  -> &lt;  > -> &gt; \ -> \\, all characters above #127->\xXX ...
;; see  TeXmacs/langs/encodings/cork-escaped-to-ascii.scm

(define (latex-encode tm-fragment-tree)
  ;; for the latex representation we mimick what is done when
  ;; "copy to latex" is performed
  (let* ((latex-tree (latex-expand tm-fragment-tree))
         ;; expand or not macros according to preferences
         (latex-code (texmacs->generic latex-tree "latex-snippet"))
        ) ;
    ;; actual conversion
    (escape-to-ascii latex-code)
  ) ;let*
) ;define

(define (tm-encode tm-fragment-tree)
  (escape-to-ascii (serialize-texmacs tm-fragment-tree))
) ;define

(define (refactor-svg dest tm-fragment relbaseline)
  ;; dest is the url of the svg file to be edited
  ;; We reorganize & cleanup the svg file and inject attributes containing:
  ;; - the tm code of equation
  ;; - style info from the original document (style, fonts, layout, ...)
  ;; - A latex fragment for compatibility with 'textext' inkscape extension
  ;; - the relative position of the baseline to enable vertical alignement
  ;;   in an external application
  ;; FIXME : no error checking, no return value...

  (let* (
          ;; 1: load svg and transform to an active tree in
          ;; temporary buffer so that we can manipulate it
          ;; using texmacs primitives for trees
          (svg-in (string-load dest))
          ;; load svg file as string
          (s-svg-in (parse-xml svg-in))
          ;; parse to stree
          (mybuf (buffer-new))
          ;; create temporary buffer for subsequent manipulations of svg tree
          (void (buffer-set-body mybuf (tree-assign-node! (stree->tree s-svg-in) 'concat))
          ) ;void
          ;; populate buffer with tree
          ;; replace *TOP* node by concat otherwise displaying
          ;; that buffer crashes texmacs

          ;; 2: define a bunch of locations in the tree
          (buftree (buffer-get-body mybuf))
          ;; the whole tree
          (svgroot (car (select buftree '(:* svg))))
          ;; the <svg > node
          (groups (select svgroot '(g)))
          (maingroup (if (list>1? groups)
                       (begin
                         (tree-insert-node! svgroot 0 '(svg))
                         (with oldroot
                           (tree-ref svgroot 'svg)
                           (move-node! (tree-ref oldroot '@) svgroot 0)
                           (tree-insert! oldroot 0 (list '(@)))
                           (tree-assign-node! oldroot 'g)
                           oldroot
                         ) ;with
                       ) ;begin
                       (car groups)
                     ) ;if
          ) ;maingroup
          ;; the main group in the svg, containing the drawing layout
          ;; (if more than one group, we group everything in a new group)
          (maingroup-attrib (car (select maingroup '(@))))
          ;; attributes of the main group
          (defs (select svgroot '(defs)))
          (defs (if (list>0? defs) (car defs) #f))
          ;; the defs, containing the glyph vector outlines,
          ;; hyperlinked from the drawing (a.k.a cloned)
          (bgframe (select maingroup '(:* path @ style :%1)))
          (bgframe (if (null? bgframe) #f (car bgframe)))
          ;; the solid background we introduced
          ;; 3: the new data we want to insert in the tree
          (latex-code (latex-encode tm-fragment))
          (tm-code (tm-encode tm-fragment))
          (tm-inits (tm-encode (get-all-inits)))
          (tm-style (tm-encode (get-style-tree)))
          ;;; define new attributes containing latex and texmacs code:
          (extra-latex-attrib `((xmlns:ns0 "http://www.iki.fi/pav/software/textext/")
                                (ns0:text ,latex-code)
                                (ns0:preamble "texmacs_latex.sty"))
          ) ;extra-latex-attrib
          (extra-tm-attrib `((xmlns:ns1 "https://www.texmacs.org/")
                             (ns1:texmacscode ,tm-code)
                             (ns1:texmacsstyle ,tm-inits)
                             (ns1:texmacsstyle2 ,tm-style)
                             (ns1:texmacsbaseline ,relbaseline))
          ) ;extra-tm-attrib
          ;; OK, the texmacs namespace maybe not correctly described at that url ...
        ) ;

    ;; 4: modify tree
    ;; 4.1 set the background fully transparent
    (if (and bgframe
          (== (tree->string bgframe)
            " stroke:none;fill-rule:nonzero;fill:rgb(100%,100%,100%);fill-opacity:0.007;"
          ) ;==
        ) ;and
      (tree-set! bgframe
        " stroke:none;fill-rule:nonzero;fill:rgb(100%,100%,100%);fill-opacity:0.00"
      ) ;tree-set!
    ) ;if
    ;; 4.2 move defs containing the glyph outlines inside main group
    ;; so that they remain together in inkscape
    (if defs (move-node! defs maingroup 1))
    ;; 4.3 (not optional!), add our own new attributes for re-editting equation
    (tree-insert! maingroup-attrib 0 extra-latex-attrib)
    ;; for textext compatibility
    (tree-insert! maingroup-attrib 1 extra-tm-attrib)

    ;; 5: finally create output
    (let* (
            ;; convert back to stree, recreate the *TOP* node,
            ;; and restore *PI* xml
            ;; (instead of *PI* "xml" given by tree->stree -
            ;; otherwise serialize-html fails)
            (s-svg-out (append '(*TOP* (*PI* xml
                                         "version=\"1.0\" encoding=\"UTF-8\""))
                         ;; actually we use only ascii
                         (cddr (tree->stree buftree))
                       ) ;append
            ) ;s-svg-out
            (xml-svg-out (begin
                           (output-flush)
                           ;; necessary??
                           (serialize-tmml s-svg-out)
                         ) ;begin
            ) ;xml-svg-out
          ) ;
      ;; close temporary buffer
      (buffer-pretend-saved mybuf)
      (buffer-close mybuf)
      (string-save xml-svg-out dest)

    ) ;let*
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; public interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (clipboard-copy-image void)
  (:synopsis "Places an image of the current selection on the clipboard")
  (:argument void "not used")
  (:returns "nothing")
  ;; the format of the graphics is set in the preferences
  (if (not (qt-gui?))
    (set-message "Qt GUI only, sorry. Use \"Export selection...\"" "")
    (if (not (selection-active-any?))
      (set-message "no selection!" "")
      (let* ((format (get-preference "texmacs->image:format"))
             (tmpurl (url-temp-ext format))
            ) ;
        (export-selection-as-graphics tmpurl)
        ;; first generate an image file
        (graphics-file-to-clipboard tmpurl)
        ;; place that image on the clipboard
        (system-remove tmpurl)
      ) ;let*
    ) ;if
  ) ;if
) ;tm-define

(tm-define (export-selection-as-graphics myurl)
  (:synopsis "Generates graphics format of the current selection")
  (:argument myurl "A full file url with extension")
  (:returns "nothing")
  ;; for svg export, the texmacs code of the selection as well as
  ;; global document parameters such as style, fonts, etc. are
  ;; embedded in the drawing to enable re-editing from inkscape

  ;; we try reasonably hard to have the wysiwyg behavior:
  ;; the image should ideally reproduce what is on the screen, including line breaks if any
  (if (not (selection-active-any?))
    (set-message "no selection!" "")
    (begin

      ;; Check the suffix of the URL
      (if (== (url-suffix myurl) "")
        (with format
          (get-preference "texmacs->image:format")
          (show-message (string-append "No file extension specified, defaulting to " format)
            "No image format given"
          ) ;show-message
          (with suffix
            (format-default-suffix format)
            (set! myurl (url-glue myurl (string-append "." suffix)))
          ) ;with
        ) ;with
      ) ;if

      ;; Check the converter from PDF to suffix format
      (with suffix
        (url-suffix myurl)
        (when (not (file-converter-exists? "x.pdf" (string-append "y." suffix)))
          (show-message (string-append "Sorry, pdf to "
                          suffix
                          " converter is missing. Generating pdf instead"
                        ) ;string-append
            "Image format not available"
          ) ;show-message
          (let* ((sufl (string-length suffix))
                 (surl (url->string myurl))
                 (sl (string-length surl))
                ) ;
            (set! myurl (string->url (string-append (substring surl (- sl sufl) sl) "pdf")))
          ) ;let*
        ) ;when
      ) ;with

      ;; TODO Handle when output file already exists (presently we overwritte without warning)

      (selection-trim-ending)

      (let* ((suffix (url-suffix myurl))
             ;; step 1 prepare and typeset selection
             ;; if selection is inside inline or display math preserve inline/display style
             (issomemath (nnot (match? (tree->stree (selection-tree))
                                 '(:or (equation* :*)
                                    (equation :*)
                                    (eqnarray :*)
                                    (eqnarray* :*)
                                    (math :*)
                                    (align :*)
                                    (align* :*))
                               ) ;match?
                         ) ;nnot
             ) ;issomemath
             (inmath (== (tree->string (get-env-tree-at "mode" (selection-get-start))) "math")
             ) ;inmath
             (indisplaymath (== (tree->string (get-env-tree-at "math-display" (selection-get-start)))
                              "true"
                            ) ;==
             ) ;indisplaymath

             (tm-fragment (cond (issomemath (debug "selection tree is a math tag \n") (selection-tree))
                                (inmath (if indisplaymath
                                          (begin
                                            (debug "selection tree is in display math \n")
                                            (stree->tree `(equation* ,(selection-tree)))
                                          ) ;begin
                                          (begin
                                            (debug "selection tree is in inline math \n")
                                            (stree->tree `(math ,(selection-tree)))
                                          ) ;begin
                                        ) ;if
                                ) ;inmath
                                (else (debug "selection not purely math \n") (selection-tree))
                          ) ;cond
             ) ;tm-fragment
             ;; is selection wider than 1par (and needs linebreaks and or hyphenation)?
             (maxwidth (length-decode "1par"))
             (partmpt (string-append (number->string maxwidth) "tmpt"))
             (parcm (length-add "0cm" partmpt))
             ;; get-page-width return dimension in the cpp "SI" unit which seems to be == "10tmpt"
             ;; output scale of image?

             ;; We compute the baseline position only if it's a single-line content
             ;; (this excludes selections begining with 'document)
             ;; If the selection is eqnarray or similar compute it only if the table
             ;; has a single row
             (iseqnarray (nnot (match? (tree->stree tm-fragment)
                                 '(:or (eqnarray :*)
                                    (eqnarray* :*)
                                    (align :*)
                                    (align* :*))
                               ) ;match?
                         ) ;nnot
             ) ;iseqnarray
             (table-t (if iseqnarray (tree-ref (selection-tree) :* 'table) #f))
             (eqarraynrows (if table-t (length (select table-t '(:%0 row))) #f))
             (simpleeqnarray (and iseqnarray (== 1 eqarraynrows)))
             (tmppng (url-temp-ext "png"))
             (extents (print-snippet tmppng (selection-tree) #t))
             (rawwidth (- (third extents) (first extents)))
             (rawwidthOK (< rawwidth maxwidth))
             (needbaseline (if (match? (tree->stree tm-fragment) '(document :*))
                             #f
                             (if iseqnarray (if (and simpleeqnarray rawwidthOK) #t #f) rawwidthOK)
                           ) ;if
             ) ;needbaseline

             ;; the baseline calculation is relative to the size of the background frame
             ;; poppler puts a background frame in the svg image only if not fully transparent (otherwise no)
             ;; (during svg postprocessing  we'll set the opacity of the background to 0)
             (fillcolor (if (and needbaseline (== suffix "svg")) "#ffffff02" "#ffffffff"))

             ;; if selection is an equation array, make table width minimal to avoid wide white frame
             (tm-fragment1 (if iseqnarray
                             (with tfmt
                               (tree-ref tm-fragment :* 'tformat)
                               (tree-insert tfmt 0 '((twith "table-hmode" "min")))
                               tm-fragment
                             ) ;with
                             tm-fragment
                           ) ;if
             ) ;tm-fragment1

             (tm-fragment-formated (if needbaseline
                                     ;; if needbaseline insert fragment in table having a background

                                     `(with (tabular (tformat (twith ,"table-width"
                                                                ,parcm)
                                                       (twith "table-hmode"
                                                         "min")
                                                       (twith "table-valign"
                                                         "B")
                                                       ;; =baseline of top line
                                                       (cwith ,"1"
                                                         ,"1"
                                                         ,"1"
                                                         ,"1"
                                                         ,"cell-background"
                                                         ,fillcolor)
                                                       (cwith "1"
                                                         "1"
                                                         "1"
                                                         "1"
                                                         "cell-lsep"
                                                         "0spc")
                                                       (cwith "1"
                                                         "1"
                                                         "1"
                                                         "1"
                                                         "cell-tsep"
                                                         "0sep")
                                                       (cwith "1"
                                                         "1"
                                                         "1"
                                                         "1"
                                                         "cell-rsep"
                                                         "0spc")
                                                       (cwith "1"
                                                         "1"
                                                         "1"
                                                         "1"
                                                         "cell-bsep"
                                                         "0sep")
                                                       (table (row (cell ,tm-fragment1))))))
                                     ;; otherwise (multiline selection) use doc-at to get proper pagewidth
                                     `(with ,"fill-color"
                                        ,fillcolor
                                        ,"doc-at-width"
                                        ,parcm
                                        ,"doc-at-hmode"
                                        ,(if (or iseqnarray
                                               indisplaymath
                                               inmath)
                                           "min"
                                           "exact")
                                        ,"doc-at-padding"
                                        ,"0spc"
                                        (document-at (document ,tm-fragment1)
                                          (point "0par" "0")))
                                   ) ;if
             ) ;tm-fragment-formated

             ;; step 2 generate output according to desired output format
             (extents (print-snippet myurl (stree->tree tm-fragment-formated) #t))
             ;; compute relative position of baseline from returned box dimensions  see tmhtml.scm
             (height (- (fourth extents) (second extents)))
             (relbaseline (if needbaseline
                            (number->string (exact->inexact (/ (- (sixth extents)) height)))
                            "0.0"
                          ) ;if
             ) ;relbaseline
            ) ;

        (system-remove tmppng)
        (if (== suffix "svg")
          (begin
            ;; modify svg, embedding texmacs code
            (debug "relbaseline= " relbaseline "\n")
            (refactor-svg myurl tm-fragment relbaseline)
          ) ;begin
        ) ;if
      ) ;let*
    ) ;begin
  ) ;if
) ;tm-define
