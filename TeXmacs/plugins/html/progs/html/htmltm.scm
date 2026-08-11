
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : htmltm.scm
;; DESCRIPTION : conversion of Html trees to TeXmacs trees
;; COPYRIGHT   : (C) 2003  David Allouche
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (convert html htmltm)
  (:use (convert tools tmlength)
    (convert tools tmcolor)
    (convert tools old-tmtable)
    (convert tools stm)
    (convert data sxml)
    (convert data sxhtml)
    (convert tools environment)
    (convert data xmltm)
    (convert mathml mathtm)
  ) ;:use
) ;texmacs-module

(define (assoc-string-ci key alist)
  (list-find alist (lambda (pair) (string-ci=? key (car pair))))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; HTML color library
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (html-color->tmcolor s)
  (let ((rgb255 (or (html-named-color->rgb255 s) (html-rgb-color->rgb255 s))))
    (if rgb255 (rgb255->tmcolor rgb255) #f)
  ) ;let
) ;define

(define html-named-colors
  '(("black" (0 0 0))
    ("silver" (192 192 192))
    ("gray" (128 128 128))
    ("white" (255 255 255))
    ("maroon" (128 0 0))
    ("red" (255 0 0))
    ("purple" (128 0 128))
    ("fuchsia" (255 0 255))
    ("green" (0 128 0))
    ("lime" (0 255 0))
    ("olive" (128 128 0))
    ("yellow" (255 255 0))
    ("navy" (0 0 128))
    ("blue" (0 0 255))
    ("teal" (0 128 128))
    ("aqua" (0 0 255)))
) ;define

(define (html-named-color->rgb255 s)
  (cond ((assoc-string-ci s html-named-colors) => second)
        (else #f)
  ) ;cond
) ;define

(define (html-rgb-color->rgb255 s)
  (let ((cs (string->list s)))
    (if (and (char=? #\# (car cs))
          (== 7 (length cs))
          (list-every char-hexadecimal? (cdr cs))
        ) ;and
      (with (hash r1 r2 g1 g2 b1 b2)
        cs
        (map hexlist->integer `((,r1 ,r2) (,g1 ,g2) (,b1 ,b2)))
      ) ;with
      #f
    ) ;if
  ) ;let
) ;define

(define (hexlist->integer cs)
  (let next
    ((i 0) (cs cs))
    (if (pair? cs)
      (next (+ (* 16 i) (hexadecimal-digit->integer (car cs))) (cdr cs))
      i
    ) ;if
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmltm-table env a c)
  ;; TODO: support @lang attributes
  ;; NOT SUPPORTED: @summary - for spoken rendering
  ;;                @title   - no tooltip feature
  ;;                @style   - no CSS support
  ;;                @events  - no event support
  (let ((cells (table-cells env a c)))
    (if (null? cells)
      '()
      ((cut table-align env a <>)
       (append (list (tmtable->stm (tmtable (table-formats env a c) cells)))
         (table-label env a)
       ) ;append
      ) ;
    ) ;if
  ) ;let
) ;define

(define (table-label env a)
  (let ((label (xmltm-attr->label a 'id)))
    (if label (list label) '())
  ) ;let
) ;define

(define (table-align env a stms)
  ;; (tmhtml-env shtml-attribute-list symbol (list stm) -> (list stm))
  ;; NOTE: may be generalized to support @align for P, DIV and Hn.
  (let ((m (and-let* ((p (assoc 'align a)) (list-length=2? p))
             (list-find '("left" "center" "right") (cut string-ci=? (second p) <>))
           ) ;and-let*
        ) ;m
       ) ;
    (if (not m)
      stms
      (list `(document (with ,"par-mode"
                         ,m
                         ,(stm-remove-unary-document (htmltm-serial (htmltm-preserve-space? env)
                                                       stms))))
      ) ;list
    ) ;if
  ) ;let
) ;define

(define (table-formats env a c)
  ;; As a convention, global properties are placed at the end of the list.
  ;; Remember that tmtable->stm reverses the list of table formats.
  (append (table-content-formats env c)
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;; TODO: table cellspacing and cellpadding
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    (table-background env a)
    (table-borders env a c)
    (list (tmformat-table "cell-hyphen" "t"))
    (table-width env a)
  ) ;append
) ;define

(define (table-width env a)
  ;; TODO: extend the typesetter to support hyphenated cells balancing
  (let ((len (htmltm-dimension a 'width)))
    (if (tmlength-null? len)
      (list (tmformat-frame "table-width" (tmlength 1 'par))
        (tmformat-frame "table-hmode" "min")
      ) ;list
      (list (tmformat-frame "table-width" len))
    ) ;if
  ) ;let
) ;define

(define (table-background env a)
  (or (and-let* ((html-color (shtml-attr-non-null a 'background))
                 (tmcolor (html-color->tmcolor html-color))
                ) ;
        (list (tmformat-table "cell-background" (html-color->tmcolor color)))
      ) ;and-let*
    '()
  ) ;or
) ;define

(define (table-borders env a c)
  ;; HTML: BORDER alone is understood as FRAME=BORDER.
  ;;
  ;; Mozilla: Without RULES, a BORDER or FRAME attribute enables
  ;; Netscape-style table rendering (with bevels). An additional FRAME
  ;; attribute (non-valid) may disable the frame bevels but not the cell
  ;; bevels. With RULES, borders are rules are drawn as simple lines.
  ;; An empty (or invalid) FRAME attribute is understood as FRAME=BORDER.
  ;; An empty (or invalid) BORDER attribute is understood as BORDER=1.
  ;; An empty (or invalid) RULES attribute is understood as RULES=ALL.
  ;; A BORDER attribute with a negative number value is understood as BORDER=0
  ;; and disables bevelled table rendering.
  ;;
  ;; TeXmacs cannot draw bevelled tables, so we let an explicit FRAME
  ;; attributes overrides an empty BORDER attribute.

  (define (format-frame . names)
    (lambda (value) (map (lambda (s) (tmformat-frame s value)) names))
  ) ;define

  (define frame-values-alist
    `((,"void" ,(delay (format-frame)))
      (,"above" ,(delay (format-frame "table-tborder")))
      (,"below" ,(delay (format-frame "table-bborder")))
      (,"hsides" ,(delay (format-frame "table-tborder" "table-bborder")))
      (,"lhs" ,(delay (format-frame "table-lborder")))
      (,"rhs" ,(delay (format-frame "table-rborder")))
      (,"vsides" ,(delay (format-frame "table-lborder" "table-rborder")))
      (,"box"
       ,(delay (format-frame "table-tborder"
                 "table-bborder"
                 "table-lborder"
                 "table-rborder")))
      (,"border"
       ,(delay (format-frame "table-tborder"
                 "table-bborder"
                 "table-lborder"
                 "table-rborder"))))
  ) ;define
  (define rules-values-alist
    `((,"none" ,(delay '()))
      (,"groups" ,(delay (rules-groups)))
      (,"rows"
       ,(delay (list (tmformat-table-but-bottom "cell-bborder" (tmlength 1 'px)))))
      (,"cols"
       ,(delay (list (tmformat-table-but-right "cell-rborder" (tmlength 1 'px)))))
      (,"all"
       ,(delay (list (tmformat-table-but-bottom "cell-bborder" (tmlength 1 'px))
                 (tmformat-table-but-right "cell-rborder" (tmlength 1 'px))))))
  ) ;define

  ;; Handle invalid values from FRAME and RULES here.
  (define (frame-value s)
    (let ((p (assoc-string-ci s frame-values-alist)))
      (if p (second p) (frame-value "border"))
    ) ;let
  ) ;define
  (define (rules-value s)
    (let ((p (assoc-string-ci s rules-values-alist)))
      (if p (second p) (rules-value "all"))
    ) ;let
  ) ;define
  (define (rules-groups)
    '()
  ) ;define
  ;;;;;;; TODO ;;;;;;;
  (let ((rules (rules-value "none")) (border 1) (frame (frame-value "void")))
    (define (enable-bevels! n)
      (set! frame (frame-value "border"))
      (set! rules (rules-value "all"))
      (set! border n)
    ) ;define
    (and-let* ((@border (assoc 'border a)))
      (if (list-length=2? @border)
        (let ((n (string->number (second @border))))
          (cond ((not n) (enable-bevels! 1))
                ((<= n 0) (set! border 0))
                (else (enable-bevels! n))
          ) ;cond
        ) ;let
        (enable-bevels! 1)
      ) ;if
    ) ;and-let*
    ;; Handle invalid empty FRAME and RULES here.
    (and-let* ((@frame (assoc 'frame a)))
      (if (list-length=2? @frame)
        (set! frame (frame-value (second @frame)))
        (set! frame (frame-value "border"))
      ) ;if
    ) ;and-let*
    (and-let* ((@rules (assoc 'rules a)))
      (if (list-length=2? @rules)
        (set! rules (rules-value (second @rules)))
        (set! rules (rules-value "all"))
      ) ;if
    ) ;and-let*
    (append (force rules)
      (if (= border 0) '() ((force frame) (tmlength border 'px)))
    ) ;append
  ) ;let
) ;define

(define (table-content-formats env c)
  (sxhtml-table-fold table-content-formats/kons '() `(h:table ,@c))
) ;define

(define (table-content-formats/kons msg i j kar kdr)
  ;; NOTE: ignored cell attributes: header, scope, abbr, axis, class, title,
  ;;   style, intrisic events.
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ;; TODO: cell attributes: nowrap, width, height, id, bgcolor, align, char,
  ;;   charoff, valign.
  ;; TODO: row and column attributes (beware of alignement inheritance rules).
  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  (cond ((eq? msg :cell)
         (let ((attrs (sxml-attr-list kar)))
           (define (span->format html-name tm-name)
             (let ((span (shtml-decode-span attrs html-name)))
               (if (= 1 span) '() (list (tmformat-cell (1+ i) (1+ j) tm-name span)))
             ) ;let
           ) ;define
           (append (span->format 'colspan "cell-col-span")
             (span->format 'rowspan "cell-row-span")
             kdr
           ) ;append
         ) ;let
        ) ;
        (else kdr)
  ) ;cond
) ;define

(define (table-cells env a c)
  ;; TODO: ID attributes on table elements
  (let ((table `(h:table ,@c)))
    (with (nrows ncols)
      (sxhtml-table-dimension table)
      ((cut <> :out-table #f #f #f)
       (sxhtml-table-fold (lambda (msg i j kar kdr) (kdr msg i j kar))
         (cut table-cells/table ncols env '() <...>)
         table
       ) ;sxhtml-table-fold
      ) ;
    ) ;with
  ) ;let
) ;define

(define (table-cells/table ncols env table msg i j kar)
  (cond ((eq? msg :out-table) (reverse! table))
        ((eq? msg :in-row-group)
         (xpath-descend env
           kar
           (lambda (new-env)
             (cut table-cells/row-group ncols (list new-env env) table <...>)
           ) ;lambda
         ) ;xpath-descend
        ) ;
        ;; no else clause
  ) ;cond
) ;define

(define (table-cells/row-group ncols envs table msg i j kar)
  (cond ((eq? msg :out-row-group)
         (cut table-cells/table ncols (second envs) table <...>)
        ) ;
        ((eq? msg :in-row)
         (xpath-descend (car envs)
           kar
           (lambda (new-env)
             (cut table-cells/row ncols (cons new-env envs) table 0 '() <...>)
           ) ;lambda
         ) ;xpath-descend
        ) ;
        ;; no else clause
  ) ;cond
) ;define

(define (cons-empty-cells n row)
  (do ((row row (cons "" row)) (n n (1- n)))
    ((zero? n) row)
  ) ;do
) ;define

(define (table-cells/row ncols envs table next-j row msg i j kar)
  (cond ((eq? msg :out-row)
         (cut table-cells/row-group
           ncols
           (cdr envs)
           (cons (reverse! (cons-empty-cells (- ncols next-j) row)) table)
           <...>
         ) ;cut
        ) ;
        ((eq? msg :cell)
         (cut table-cells/row
           ncols
           envs
           table
           (1+ j)
           (cons (xpath-descend (car envs)
                   kar
                   (lambda (new-env)
                     (htmltm-args-serial new-env (htmltm-space-mixed new-env (sxml-content kar)))
                   ) ;lambda
                 ) ;xpath-descend
             (cons-empty-cells (- j next-j) row)
           ) ;cons
           <...>
         ) ;cut
        ) ;
        ;; no else clause
  ) ;cond
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Specific markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; In the following functions, the attributes are:
;;  @env -- the dynamic environment of the converter
;;  @a   -- attribute list of the current element
;;  @c   -- contents of the current element

(define (htmltm-ol env a c)
  (let* ((class-attr (assoc 'class a))
         (class-name (if class-attr (cadr class-attr) #f))
         (tag (if (and class-name (string-starts? class-name "enumerate"))
                class-name
                "enumerate"
              ) ;if
         ) ;tag
         (head `(,(string->symbol tag)))
         (glued (htmltm-list-glue (htmltm-args-serial env c)))
         (labeled (xmltm-label-decorate a 'id glued))
        ) ;
    (list `(document ,(rcons head labeled)))
  ) ;let*
) ;define

(define (htmltm-ul env a c)
  (let* ((class-attr (assoc 'class a))
         (class-name (if class-attr (cadr class-attr) #f))
         (tag (if (and class-name (string-starts? class-name "itemize")) class-name "itemize")
         ) ;tag
         (head `(,(string->symbol tag)))
         (glued (htmltm-list-glue (htmltm-args-serial env c)))
         (labeled (xmltm-label-decorate a 'id glued))
        ) ;
    (list `(document ,(rcons head labeled)))
  ) ;let*
) ;define

(define (htmltm-dl env a c)
  (let* ((class-attr (assoc 'class a))
         (class-name (if class-attr (cadr class-attr) #f))
         (tag (if (and class-name (string-starts? class-name "description"))
                class-name
                "description"
              ) ;if
         ) ;tag
         (head `(,(string->symbol tag)))
         (glued (htmltm-list-glue (htmltm-args-serial env c)))
         (labeled (xmltm-label-decorate a 'id glued))
        ) ;
    (list `(document ,(rcons head labeled)))
  ) ;let*
) ;define

(define (htmltm-list-item env a c)
  ;; List markers are glued by the list handler.
  (list (xmltm-label-decorate a
          'id
          (htmltm-serial (htmltm-preserve-space? env)
            `((document (item)) ,@(htmltm-args env c))
          ) ;htmltm-serial
        ) ;xmltm-label-decorate
  ) ;list
) ;define

(define (htmltm-quote env a c)
  ;; WARNING: this is incomplete. Texmacs should have a macro for inline
  ;; quotation which puts in quotation marks in a language sensitive manner.
  (list (xmltm-label-decorate a
          'id
          (htmltm-serial (htmltm-preserve-space? env)
            `(,"``" ,@(htmltm-args env c) ,"''")
          ) ;htmltm-serial
        ) ;xmltm-label-decorate
  ) ;list
) ;define

(define (htmltm-anchor env a c)
  (list (htmltm-href->hlink a
          (xmltm-label-decorate a
            'id
            (xmltm-label-decorate a 'name (htmltm-args-serial env c))
          ) ;xmltm-label-decorate
        ) ;htmltm-href->hlink
  ) ;list
) ;define

(define (htmltm-href->hlink a body)
  (let ((href (shtml-attr-non-null a 'href)))
    (if href `(hlink ,body ,(xmltm-url-text href)) body)
  ) ;let
) ;define

(define (htmltm-dimension attrs name)
  (let ((s (shtml-attr-non-null attrs name)))
    (if (not s)
      (tmlength)
      (cond ((string-null? s) (tmlength))
            ((string->number s) => (lambda (n) (tmlength n 'px)))
            ((and (string-ends? s "%") (string->number (string-drop-right s 1)))
             =>
             (lambda (n) (tmlength (/ n 100) 'par))
            ) ;
            (else (tmlength))
      ) ;cond
    ) ;if
  ) ;let
) ;define

(define (htmltm-image env a c)
  (let* ((s (xmltm-url-text (or (shtml-attr-non-null a 'src) "")))
         (w (tmlength->string (htmltm-dimension a 'width)))
         (h (tmlength->string (htmltm-dimension a 'height)))
        ) ;
    (list (xmltm-label-decorate a
            'id
            (if (not (and (string-null? w) (string-null? h)))
              `(image ,s ,w ,h ,"" ,"")
              `(image ,s ,"0.6383w" ,"" ,"" ,"")
            ) ;if
          ) ;xmltm-label-decorate
    ) ;list
  ) ;let*
) ;define

(define (htmltm-font env a c)
  ;; WARNING: do as old filter, but is fragile and not conformant
  (list (htmltm-with-size a
          (htmltm-with-color a (xmltm-label-decorate a 'id (htmltm-args-serial env c)))
        ) ;htmltm-with-size
  ) ;list
) ;define

(define (htmltm-with-size a x)
  ;; Helper for htmltm-font
  (let* ((sz (shtml-attr-non-null a 'size))
         (mult (assoc sz
                 '(("-4" "0.5")
                   ("-3" "0.6")
                   ("-2" "0.7")
                   ("-1" "0.8")
                   ("+1" "1.2")
                   ("+2" "1.4")
                   ("+3" "1.7")
                   ("+4" "2"))
               ) ;assoc
         ) ;mult
        ) ;
    (if mult `(with ,"font-size" ,(second mult) ,x) x)
  ) ;let*
) ;define

(define (htmltm-with-color a x)
  ;; Helper for htmltm-font
  (or (and-let* ((html-color (shtml-attr-non-null a 'color))
                 (tmcolor (html-color->tmcolor html-color))
                ) ;
        `(with ,"color" ,(tmcolor->stm tmcolor) ,x)
      ) ;and-let*
    x
  ) ;or
) ;define

(define (htmltm-br env a c)
  (if (sxhtml-list? (xpath-parent env)) '() '((next-line)))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Unlike previously, Mathml nodes in HTML5 have no namespace prefix
;; => xmltm.scm prefixes them with h:
;; Yet, the import code in mathtm.scm expects m: prefix : replace prefix (hacky)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmltm-math env a c)
  (with cm
    `(m:math (@ ,@a) ,@(replace-nsprefix-in-stree c "h:" "m:"))
    `(,(mathtm-as-serial env cm))
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MathJax extension
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmltm-mathjax env a c)
  (if (and (list-1? c) (string? (car c)) (string->number (car c)))
    (let* ((id (string->number (car c)))
           (t (retrieve-mathjax id))
           (s (tree->string t))
           (l (parse-latex s))
           (r (latex->texmacs l))
          ) ;
      (list (tm->stree r))
    ) ;let*
    (htmltm-drop env a c)
  ) ;if
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special rules for improving Wikipedia rendering
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmltm-tex-image s)
  (with lt
    (string-append "$\\displaystyle " s "$")
    (with tm (convert lt "latex-snippet" "texmacs-stree") (list tm))
  ) ;with
) ;define

(define (htmltm-wikipedia-image env a c)
  (if (and (== (shtml-attr-non-null a 'class) "tex") (shtml-attr-non-null a 'alt))
    (htmltm-tex-image (shtml-attr-non-null a 'alt))
    (htmltm-image env a c)
  ) ;if
) ;define

(define (split-formula-by-newlines formula)
  (let* ((formula-list (if (and (list? formula) (eq? (car formula) 'concat))
                         formula
                         `(concat ,formula)
                       ) ;if
         ) ;formula-list
         (parts (cdr formula-list))
        ) ;

    (define (process-parts parts current segments)
      (if (null? parts)
        ;; last segment
        (if (> (length current) 1)
          (append segments (list (list 'equation* (list 'document current))))
          segments
        ) ;if
        ;; current segment
        (let ((part (car parts)) (rest (cdr parts)))
          (if (equal? part '(next-line))
            (if (> (length current) 1)
              (process-parts rest
                '(concat)
                (append segments (list (list 'equation* (list 'document current))))
              ) ;process-parts
              (process-parts rest '(concat) segments)
            ) ;if
            (process-parts rest (append current (list part)) segments)
          ) ;if
        ) ;let
      ) ;if
    ) ;define

    ;; package the result
    (let ((result (process-parts parts '(concat) '())))
      (cond ((> (length result) 1) (cons 'document result))
            ((= (length result) 1) (car result))
            (else '(document ""))
      ) ;cond
    ) ;let
  ) ;let*
) ;define

(define (htmltm-span env a c)
  (with class-value
    (shtml-attr-non-null a 'class)
    (cond ((== class-value "mwe-math-element")
           (if (and (pair? c) (func? (car c) 'h:span))
             (htmltm env (car c))
             (htmltm-pass env a c)
           ) ;if
          ) ;
          ((== class-value "texhtml") (list `(math ,(htmltm-args-serial env c))))

          ((and (== class-value "katex")
             (pair? c)
             (func? (car c) 'h:span)
             (sxml-has-attr-list? (car c))
             (== (shtml-attr-non-null (sxml-attr-list (car c)) 'class) "katex-mathml")
           ) ;and
           (begin
             (htmltm env (first c))
           ) ;begin
          ) ;

          ((and (== class-value "ztext-math"))
           (begin
             (let ((parsed-formula (tm->stree (latex->texmacs (parse-latex (shtml-attr-non-null a 'data-tex))))
                   ) ;parsed-formula
                  ) ;
               (list (split-formula-by-newlines parsed-formula))
             ) ;let
           ) ;begin
          ) ;

          ((and (string? class-value)
             (string-starts? class-value "container-")
             (string-ends? class-value "math-inline")
           ) ;and
           (begin
             (let ((parsed-formula (tm->stree (latex->texmacs (parse-latex (shtml-attr-non-null a 'data-custom-copy-text)))
                                   ) ;tm->stree
                   ) ;parsed-formula
                  ) ;
               (list (split-formula-by-newlines parsed-formula))
             ) ;let
           ) ;begin
          ) ;
          (else (begin (htmltm-pass env a c)))
    ) ;cond
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Special rules for improving Scilab documentation rendering
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmltm-scilab-pre env a c)
  (if (== (shtml-attr-non-null a 'class) "scilabcode")
    (list `(scilab-code ,(htmltm-args-serial env c)))
    (list `(code ,(htmltm-args-serial env c)))
  ) ;if
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main translation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmltm-drop env a c)
  '()
) ;define

(define (htmltm-pass env a c)
  (let ((l (htmltm-args env c)))
    (if (and (null? l) (not (assoc 'id a)))
      '()
      (list (xmltm-label-decorate a 'id (htmltm-serial (htmltm-preserve-space? env) l))
      ) ;list
    ) ;if
  ) ;let
) ;define

(define (htmltm-args env l)
  ;; Convert the content list of an HTML element.
  (append-map (lambda (x) (xpath-descend env x (cut htmltm <> x))) l)
) ;define

(define (htmltm-args-serial env l)
  (htmltm-serial (htmltm-preserve-space? env) (htmltm-args env l))
) ;define

(define (htmltm env t)
  ;; Convert a shxml element to texmacs.
  ;; Return a list which is either null, or contains a single serial node.
  ;; All methods must use this return convention.
  (sxml-dispatch (lambda (env t) (list (utf8->cork t))) htmltm-pass env t)
) ;define

(define (cleanup-root env root)
  (sxml-set-content root (htmltm-space-mixed env (sxml-content root)))
) ;define

(tm-define (htmltm-as-serial root)
  ;; As htmltm, but returns a serial node.
  ;; Actually also initializes the dynamic enviroment.
  ;; FIXME: move the html initialization elsewhere for symmetry with htmltm.
  (define (sub env)
    (htmltm-serial (htmltm-preserve-space? env)
      (htmltm env (cleanup-root env root))
    ) ;htmltm-serial
  ) ;define
  (initialize-xpath (environment) root (cut initialize-htmltm <> sub))
) ;tm-define

(define handler (cut htmltm-handler <> <> <> htmltm-args-serial))

(logic-dispatcher htmltm-methods%
  ;;; Document structure
  ((:or head title meta) htmltm-drop)
  ((:or html body) (handler :mixed :inline htmltm-pass))

  ;; Grouping
  (div (handler :mixed :block htmltm-pass))
  ;; TODO: convert 'align' attributes in div, p and headings
  (span (handler :collapse :inline htmltm-span))

  ;; Headings
  (h1 (handler :mixed :block "chapter*"))
  (h2 (handler :mixed :block "section*"))
  (h3 (handler :mixed :block "subsection*"))
  (h4 (handler :mixed :block "subsubsection*"))
  (h5 (handler :mixed :block "paragraph*"))
  (h6 (handler :mixed :block "subparagraph*"))

  ;; Address and text direction
  (address (handler :mixed :block htmltm-pass))
  (bdo (handler :collapse :inline htmltm-pass))

  ;;; Structured text
  ;; Phrase elements
  (em (handler :collapse :inline "em"))
  (strong (handler :collapse :inline "strong"))
  (cite (handler :collapse :inline "cite*"))
  (dfn (handler :collapse :inline "dfn"))
  (code (handler :collapse :inline "code*"))
  (samp (handler :collapse :inline "samp"))
  (kbd (handler :collapse :inline "kbd"))
  (var (handler :collapse :inline "var"))
  (abbr (handler :collapse :inline "abbr"))
  (acronym (handler :collapse :inline "acronym"))

  ;; Quotations
  ;; NOTE: there should be a texmacs macro for Q
  (q (handler :collapse :inline htmltm-quote))
  (blockquote (handler :mixed :block "quotation"))

  ;; Subscripts and superscripts
  (sub (handler :mixed :inline '(rsub)))
  (sup (handler :mixed :inline '(rsup)))

  ;; Lines and paragraphs
  (p (handler :mixed :block htmltm-pass))
  (br (handler :empty :inline htmltm-br))
  (pre (handler :pre :block htmltm-scilab-pre))

  ;; Document changes
  ((:or ins del) (handler :collapse :inline htmltm-pass))

  ;;; Lists
  (dl (handler :element :block htmltm-dl))
  (dt (handler :mixed :block "item*"))
  (dd (handler :mixed :block htmltm-pass))
  (ol (handler :element :block htmltm-ol))
  (ul (handler :element :block htmltm-ul))
  (li (handler :mixed :block htmltm-list-item))
  (menu (handler :element :block "itemize"))
  (dir (handler :element :block "itemize"))

  ;;; Tables
  (table (handler :element :block htmltm-table))
  ((:or col colgroup tbody thead tfoot tr td th)
   (handler :mixed :inline htmltm-pass)
  ) ;

  ;;; Links
  (a (handler :mixed :inline htmltm-anchor))
  ;; Elements allowed only in HEAD
  ((:or (link base)) htmltm-drop)

  ;;; Objects images and applets
  (object (handler :mixed :inline htmltm-drop))
  ;; TODO: handle cases where OBJECT is equivalent to IMG
  (param htmltm-drop)
  (img (handler :empty :inline htmltm-wikipedia-image))
  (applet (handler :mixed :inline htmltm-drop))
  (map (handler :element :inline htmltm-drop))
  (area htmltm-drop)

  ;;; Alignement, font styles and horizontal rules
  ;; Alignement (deprecated)
  ;; NOTE: the center macro is now deprecated.
  (center (handler :mixed :block '(with "par-mode" "center")))

  ;; Font style
  (tt (handler :collapse :inline '(with "font-family" "tt")))
  (i (handler :collapse :inline '(with "font-shape" "italic")))
  (b (handler :collapse :inline '(with "font-series" "bold")))
  (big (handler :collapse :inline '(with "font-size" "1.2")))
  (small (handler :collapse :inline '(with "font-size" "0.83")))
  ((:or s strike) (handler :collapse :inline htmltm-pass))
  (u (handler :collapse :inline "underline"))
  (style htmltm-drop)

  ;; Font modifiers (deprecated)
  (font (handler :collapse :inline htmltm-font))
  (basefont htmltm-drop)

  ;; Rules
  (hr (handler :empty :block '((hrule))))

  ;;; Frames
  (frameset htmltm-drop)
  (frame htmltm-drop)
  (noframes (handler :mixed :inline htmltm-pass))
  (iframe (handler :mixed :block htmltm-pass))

  ;;; Forms
  (form (handler :mixed :block htmltm-drop))
  ;; elements allowed only in FORM
  ((:or input button select optgroup option textarea isindex label fieldset legend)
   htmltm-drop
  ) ;

  ;;; Scripting
  (script htmltm-drop)
  (noscript (handler :mixed :block htmltm-pass))

  ;;; Extensions
  (mathjax (handler :collapse :inline htmltm-mathjax))

  ;;; Math tag in HTML5 (no namespace prefix)
  (math (handler :mixed :block htmltm-math))

  ;; Tags present in the previous converter
  ;; Unknown: FIG FN NOTE AU LANG PERSON
  ;; ABBREV instead of ABBR
  ;; BQ as a shorthand for BLOCKQUOTE
) ;logic-dispatcher

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Routines for replacement in stree
;; NOTA: made to be generic. To be moved and reused elsewhere.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (replace-symbol-in-stree st from to)
  (cond ((== st from) to)
        ((list? st) (map (lambda (x) (replace-symbol-in-stree x from to)) st))
        (else st)
  ) ;cond
) ;define

(define (replace-stree-in-stree st from to)
  (cond ((== st from) to)
        ((list? st) (map (lambda (x) (replace-stree-in-stree x from to)) st))
        (else st)
  ) ;cond
) ;define

(define (replace-string-in-stree st from to)
  (cond ((string? st) (string-replace st from to))
        ((list? st) (map (lambda (x) (replace-string-in-stree x from to)) st))
        (else st)
  ) ;cond
) ;define

(define (replace-str-by-st-in-stree st from to)
  (cond ((and (string? st) (string-contains? st from))
         (let* ((st (string-decompose st from)) (st (list-intersperse st to)))
           `(concat ,@st)
         ) ;let*
        ) ;
        ((list? st) (map (lambda (x) (replace-str-by-st-in-stree x from to)) st))
        (else st)
  ) ;cond
) ;define

(define (replace-in-stree st from to)
  (cond ((and (symbol? from) (symbol? to)) (replace-symbol-in-stree st from to))
        ((and (string? from) (string? to)) (replace-string-in-stree st from to))
        ((and (string? from) (list? to)) (replace-str-by-st-in-stree st from to))
        ((list? from) (replace-stree-in-stree st from to))
        (else st)
  ) ;cond
) ;define

(define (replace-nsprefix-in-stree st from to)
  (cond ((and (nnull? st) (symbol? st) (string-starts? (symbol->string st) from))
         (string->symbol (string-append to (string-drop (symbol->string st) (string-length from)))
         ) ;string->symbol
        ) ;
        ((list? st) (map (lambda (x) (replace-nsprefix-in-stree x from to)) st))
        (else st)
  ) ;cond
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Post processing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (html-postproc st)
  (let* ((st (replace-in-stree st "<varspace>" '(nbsp))))
    st
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (parse-html-snippet s) (htmltm-parse s))

(tm-define (parse-html-document s) `(!file ,(htmltm-parse s)))

(define (convert-html-texmacs html)
  (let* ((snippet? (not (func? html '!file 1)))
         (body (if snippet? html (cadr html)))
         (tm (html-postproc (htmltm-as-serial (sxhtml-correct-table body))))
        ) ;
    (if snippet?
      tm
      (let* ((aux (stm-unary-document tm))
             (doc (tree->stree (tree-simplify (stree->tree aux))))
             (body `(body ,doc))
             (style '(style "browser"))
            ) ;
        `(document ,body ,style)
      ) ;let*
    ) ;if
  ) ;let*
) ;define

(tm-define (html->texmacs html)
  (:type (-> stree stree))
  (:synopsis "Convert a parsed HTML stree @t into a TeXmacs stree.")
  (tree->stree (clean-html (convert-html-texmacs html)))
) ;tm-define
