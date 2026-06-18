
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : ref-edit.scm
;; DESCRIPTION : editing routines for references
;; COPYRIGHT   : (C) 2020  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (link ref-edit)
  (:use (utils edit variants)
    (generic generic-edit)
    (generic document-part)
    (text text-drd)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Finding all standard types of labels/references in a document
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (label-context? t) (tree-in? t (label-tag-list)))

(tm-define (reference-context? t) (tree-in? t (reference-tag-list)))

(tm-define (citation-context? t) (tree-in? t (citation-tag-list)))

(tm-define (tie-context? t)
  (or (label-context? t) (reference-context? t) (citation-context? t))
) ;tm-define

(define ((named-context? pred? . ids) t)
  (and (pred? t)
    (exists? (lambda (id) (exists? (cut tm-equal? <> id) (tm-children t))) ids)
  ) ;and
) ;define

(tm-define (and-nnull? l) (and (nnull? l) l))

(tm-define (search-labels t) (tree-search t label-context?))

(tm-define (search-label t . ids)
  (tree-search t (apply named-context? (cons label-context? ids)))
) ;tm-define

(tm-define (search-references t) (tree-search t reference-context?))

(tm-define (search-reference t id)
  (tree-search t (named-context? reference-context? id))
) ;tm-define

(tm-define (search-citations t) (tree-search t citation-context?))

(tm-define (search-citation t id)
  (tree-search t (named-context? citation-context? id))
) ;tm-define

(tm-define (search-tie t id)
  (let* ((id1 (if (string-starts? id "bib-") (string-drop id 4) id))
         (id2 (string-append "bib-" id1))
        ) ;
    (tree-search t (named-context? tie-context? id1 id2))
  ) ;let*
) ;tm-define

(tm-define (search-duplicate-labels t)
  (let* ((labs (search-labels t))
         (labl (map (lambda (lab) (tm->string (tm-ref lab 0))) labs))
         (freq (list->frequencies labl))
         (filt (lambda (lab)
                 (with f (ahash-ref freq (tm->string (tm-ref lab 0))) (> (or f 0) 1))
               ) ;lambda
         ) ;filt
        ) ;
    (list-filter labs filt)
  ) ;let*
) ;tm-define

(define (tm-keys t)
  (cond ((tm-in? t '(cite-detail)) (list (tm-ref t 0)))
        (else (tm-children t))
  ) ;cond
) ;define

(define ((tie-in? t) ref)
  (with l (map tm->string (tm-keys ref)) (forall? (lambda (s) (ahash-ref t s)) l))
) ;define

(define (strip-bib s)
  (if (string-starts? s "bib-") (string-drop s 4) s)
) ;define

(define (set-of-labels t)
  (let* ((labs (search-labels t))
         (labl (map (lambda (t) (strip-bib (tm->string (tm-ref t 0)))) labs))
         (labt (list->ahash-set labl))
        ) ;
    (if (project-attached?)
      (let* ((glob (list->ahash-set (map strip-bib (list-references* #t))))
             (loc (list->ahash-set (map strip-bib (list-references))))
            ) ;
        (ahash-table-append (ahash-table-difference glob loc) labt)
      ) ;let*
      labt
    ) ;if
  ) ;let*
) ;define

(define (non-auto? t)
  (not (and (tm-compound? t)
         (forall? (lambda (l) (and-with s (tm->string l) (string-starts? s "auto-")))
           (tm-children t)
         ) ;forall?
       ) ;and
  ) ;not
) ;define

(tm-define (search-broken-references t)
  (let* ((refs (list-filter (search-references t) non-auto?)) (labt (set-of-labels t)))
    (list-filter refs (non (tie-in? labt)))
  ) ;let*
) ;tm-define

(tm-define (search-broken-citations t)
  (let* ((refs (search-citations t)) (labt (set-of-labels t)))
    (list-filter refs (non (tie-in? labt)))
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Navigation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (list-go-to-first l) (tree-go-to (car l) :end))

(tm-define (list-go-to-last l) (tree-go-to (cAr l) :end))

(define (list-go-to-previous* l)
  (when (nnull? l)
    (if (path-inf? (tree->path (car l)) (tree->path (cursor-tree)))
      (tree-go-to (car l) :end)
      (list-go-to-previous* (cdr l))
    ) ;if
  ) ;when
) ;define

(tm-define (list-go-to-previous l) (list-go-to-previous* (reverse l)))

(tm-define (list-go-to-next l)
  (when (nnull? l)
    (if (path-inf? (tree->path (cursor-tree)) (tree->path (car l)))
      (tree-go-to (car l) :end)
      (list-go-to-next (cdr l))
    ) ;if
  ) ;when
) ;tm-define

(tm-define (list-go-to l dir)
  (cond ((nlist? l) (noop))
        ((== dir :first) (list-go-to-first l))
        ((== dir :last) (list-go-to-last l))
        ((== dir :previous) (list-go-to-previous l))
        ((== dir :next) (list-go-to-next l))
  ) ;cond
) ;tm-define

(define current-id '(none))

(tm-define (tie-id)
  (and-with t
    (tree-innermost tie-context? #t)
    (or (and (exists? (cut tm-equal? <> current-id) (tm-children t)) current-id)
      (and (tm-atomic? (tm-ref t 0))
        (with key
          (tm->string (tm-ref t 0))
          (if (string-starts? key "bib-") (string-drop key 4) key)
        ) ;with
      ) ;and
    ) ;or
  ) ;and-with
) ;tm-define

(tm-define (same-ties) (and-nnull? (search-tie (buffer-tree) (tie-id))))

(tm-define (duplicate-labels)
  (and-nnull? (search-duplicate-labels (buffer-tree)))
) ;tm-define

(tm-define (broken-references)
  (and-nnull? (search-broken-references (buffer-tree)))
) ;tm-define

(tm-define (broken-citations)
  (and-nnull? (search-broken-citations (buffer-tree)))
) ;tm-define

(tm-define (go-to-same-tie dir)
  (:applicable (same-ties))
  (set! current-id (tie-id))
  (list-go-to (same-ties) dir)
) ;tm-define

(tm-define (go-to-duplicate-label dir)
  (:applicable (duplicate-labels))
  (list-go-to (duplicate-labels) dir)
) ;tm-define

(tm-define (go-to-broken-reference dir)
  (:applicable (broken-references))
  (list-go-to (broken-references) dir)
) ;tm-define

(tm-define (go-to-broken-citation dir)
  (:applicable (broken-citations))
  (list-go-to (broken-citations) dir)
) ;tm-define

(tm-define (special-extremal t forwards?)
  (:require (focus-label t))
  (with lab
    (focus-label t)
    (tree-go-to lab :end)
    (special-extremal lab forwards?)
  ) ;with
) ;tm-define

(tm-define (special-incremental t forwards?)
  (:require (focus-label t))
  (with lab
    (focus-label t)
    (tree-go-to lab :end)
    (special-incremental lab forwards?)
  ) ;with
) ;tm-define

(tm-define (special-extremal t forwards?)
  (:require (tie-context? t))
  (go-to-same-tie (if forwards? :last :first))
) ;tm-define

(tm-define (special-incremental t forwards?)
  (:require (tie-context? t))
  (go-to-same-tie (if forwards? :next :previous))
) ;tm-define

(tm-define (special-navigate t dir)
  (:require (label-context? t))
  (go-to-duplicate-label dir)
) ;tm-define

(tm-define (special-navigate t dir)
  (:require (reference-context? t))
  (go-to-broken-reference dir)
) ;tm-define

(tm-define (special-navigate t dir)
  (:require (citation-context? t))
  (go-to-broken-citation dir)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Finding the label key from its number
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (number->labels num)
  (list-filter (find-references num)
    (lambda (x) (not (or (string-starts? x "auto-") (string-starts? x "bib-"))))
  ) ;list-filter
) ;tm-define

(define (abbr->type s)
  (cond ((in? s '("t" "th" "thm")) "theorem")
        ((in? s '("p" "pr" "prop")) "proposition")
        ((in? s '("l" "le" "lm" "lem")) "lemma")
        ((in? s '("co" "cor" "corr")) "corollary")
        ((in? s '("def" "dfn" "defn")) "definition")
        ((in? s '("ass")) "assumption")
        ((in? s '("not")) "notation")
        ((in? s '("ax")) "axiom")
        ((in? s '("conv")) "convention")
        ((in? s '("conj")) "conjecture")
        ((in? s '("rem")) "remark")
        ((in? s '("war")) "warning")
        ((in? s '("ex")) "example")
        ((in? s '("exc" "exe" "exer")) "exercise")
        ((in? s '("prb" "prob")) "problem")
        ((in? s '("sol")) "solution")
        ((in? s '("c" "ch" "chap")) "chapter")
        ((in? s '("s" "sec")) "section")
        ((in? s '("ss" "ssec" "subs" "subsec")) "subsection")
        ((in? s '("par" "para")) "paragraph")
        ((in? s '("e" "eq" "eqn" "equa")) "equation")
        ((in? s '("fig")) "figure")
        ((in? s '("tab")) "table")
        ((in? s '("alg" "algo")) "algorithm")
        (else s)
  ) ;cond
) ;define

(define (type->types type)
  (cond ((== type "section") (list "section" "subsection" "subsubsection"))
        ((== type "subsection") (list "subsection" "subsubsection"))
        ((== type "paragraph") (list "paragraph subparagraph"))
        ((== type "figure") (list "big-figure" "small-figure"))
        ((== type "table") (list "big-table" "small-table"))
        ((== type "algorithm")
         (list "algorithm"
           "specified-algorithm"
           "named-algorithm"
           "named-specified-algorithm"
         ) ;list
        ) ;
        ((== type "equation") (list "equation" "eqnarray" "eqnarray*"))
        (else (list type))
  ) ;cond
) ;define

(define (abbr->types s)
  (type->types (abbr->type s))
) ;define

(define (previous-word t)
  (cond ((not (and (tree? t) (tree-up t))) #f)
        ((tree-is? (tree-up t) 'concat)
         (and-let* ((p (tree-up t)) (i (- (tree-index t) 1)))
           (while (and (>= i 0) (not (tree-atomic? (tree-ref p i)))) (set! i (- i 1)))
           (and (>= i 0)
             (let* ((s (tm-string-trim-right (tree->string (tree-ref p i))))
                    (j (string-search-backwards " " (string-length s) s))
                   ) ;
               (if (>= j 0) (substring s (+ j 1) (string-length s)) s)
             ) ;let*
           ) ;and
         ) ;and-let*
        ) ;
        ((or (tree-atomic? t) (reference-context? t) (tree-is? t 'inactive))
         (previous-word (tree-up t))
        ) ;
        (else #f)
  ) ;cond
) ;define

(define type-list
  (with l
    (append (enunciation-tag-list)
      (section-tag-list)
      (equation-tag-list)
      (algorithm-tag-list)
    ) ;append
    (append (map symbol->string l) (list "figure" "table"))
  ) ;with
) ;define

(define (plural s)
  (cond ((== s "") s)
        ((string-ends? s "y")
         (string-append (substring s 0 (- (string-length s) 1)) "ies")
        ) ;
        (else (string-append s "s"))
  ) ;cond
) ;define

(define (word-matches? w t)
  (or (== w t)
    (== w (plural t))
    (== w (translate-from-to t "english" (get-init "language")))
    (== w (translate-from-to (plural t) "english" (get-init "language")))
  ) ;or
) ;define

(define (word->type s*)
  (with s
    (locase-all s*)
    (with f
      (list-find type-list (cut word-matches? s <>))
      (or f (cond ((== s "(") "equation") (else s)))
    ) ;with
  ) ;with
) ;define

(define (word->types s)
  ;; (display* "word->types " s "\n")
  (type->types (word->type s))
) ;define

(define (label-matches? t types)
  ;; (display* "label-matches? " (tree->path t) ", " types "\n")
  (and (tree? t)
    (or (tree-in? t (map string->symbol types))
      (and (tree-is? t 'concat)
        (or (in? "subsubsection" types) (in? "subparagraph" types))
        (exists? (cut label-matches? <> types) (tree-children t))
      ) ;and
    ) ;or
  ) ;and
) ;define

(define (matching-labels ids types)
  ;; (display* "matching-labels " ids ", " types "\n")
  (with labs
    (apply search-label (cons (buffer-tree) ids))
    (if (null? labs)
      (list)
      (let* ((pred? (cut label-matches? <> types))
             (test? (lambda (lab) (tree-search-upwards lab pred?)))
             (hits (list-filter labs test?))
             (text (lambda (lab) (tm->string (tm-ref lab 0))))
            ) ;
        (list-filter (map text hits) identity)
      ) ;let*
    ) ;if
  ) ;with
) ;define

(tm-define (number->label t)
  ;; (display* "number->label " t "\n")
  (and-let* ((s (tm->string t))
             (types (or (with i
                          (string-search-forwards ":" 0 s)
                          (and (>= i 0) (abbr->types (substring s 0 i)))
                        ) ;with
                      (and-with w (previous-word t) (word->types w))
                      (list)
                    ) ;or
             ) ;types
             (num (with i
                    (string-search-forwards ":" 0 s)
                    (if (>= i 0) (substring s (+ i 1) (string-length s)) s)
                  ) ;with
             ) ;num
             (labs (and-nnull? (number->labels num)))
            ) ;
    (if (null? (cdr labs))
      (car labs)
      (with f (matching-labels labs types) (if (null? f) (car labs) (car f)))
    ) ;if
  ) ;and-let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Activating references whose keys are inferred
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (in-inactive-reference?)
  (and (in-preview-ref?)
    (tree-atomic? (cursor-tree))
    (reference-context? (tree-up (cursor-tree)))
    (tree-is? (tree-up (tree-up (cursor-tree))) 'inactive)
  ) ;and
) ;define

(define (label-exists? s)
  (with t (set-of-labels (buffer-tree)) (ahash-ref t s))
) ;define

(tm-define (kbd-return)
  (:require (in-inactive-reference?))
  (with-innermost t
    reference-context?
    (for (c (tree-children t))
      (and-with s
        (tm->string c)
        (when (not (label-exists? s))
          (and-with id (number->label c) (tree-assign c id))
        ) ;when
      ) ;and-with
    ) ;for
    (former)
  ) ;with-innermost
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Generate preview document of the content that a reference points to
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (preview-context? t)
  (or (tree-is? t 'row) (and (tree-up t) (tree-is? (tree-up t) 'document)))
) ;define

(define (math-context? t)
  (tree-in? t '(equation equation* eqnarray eqnarray*))
) ;define

(define (uncell t)
  (if (tm-func? t 'cell 1) (tm-ref t 0) t)
) ;define

(define (clean-preview t)
  (cond ((tm-is? t 'document) `(document ,@(map clean-preview (tm-children t))))
        ((tm-is? t 'concat) (apply tmconcat (map clean-preview (tm-children t))))
        ((tm-in? t (section-tag-list))
         (with l (symbol-append (tm-label t) '*) `(,l ,@(tm-children t)))
        ) ;
        ((tm-in? t '(label item item* bibitem bibitem* eq-number)) "")
        ((or (tm-func? t 'equation 1) (tm-func? t 'equation* 1))
         `(equation* ,(clean-preview (tm-ref t 0)))
        ) ;
        ((tm-in? t '(eqnarray eqnarray* tformat table row cell))
         `(,(tm-label t) ,@(map clean-preview (tm-children t)))
        ) ;
        (else t)
  ) ;cond
) ;define

(define (get-binding-value id)
  (and-let* ((val (get-reference id)))
    (let ((v (if (and (tree? val) (== (tree-label val) 'tuple) (>= (tree-arity val) 1))
               (tree-ref val 0)
               val
             ) ;if
          ) ;v
         ) ;
      (and (not (== (tree-label v) 'uninit)) v)
    ) ;let
  ) ;and-let*
) ;define

(define (fix-fig_or_tb-number doc id)
  (let ((v (get-binding-value id))
        (tag (cond ((tm-in? doc '(small-figure big-figure)) "the-figure")
                   ((tm-in? doc '(small-table big-table)) "the-table")
                   (else #f)
             ) ;cond
        ) ;tag
       ) ;
    (when (and v tag)
      (set! doc `(with ,tag (macro ,v) ,doc))
    ) ;when
    doc
  ) ;let
) ;define

(define (preview-expand-context? t)
  (tree-in? t
    '(theorem proposition
       lemma
       corollary
       conjecture
       theorem*
       proposition*
       lemma*
       corollary*
       conjecture*
       definition
       axiom
       definition*
       axiom*)
  ) ;tree-in?
) ;define

(define (label-preview t id)
  (and-with doc
    (tree-search-upwards t preview-context?)
    (with math?
      (tree-search-upwards t math-context?)
      (when (and (tree-up doc) (tree-up (tree-up doc)) (tree-is? (tree-up doc) 'document))
        (with enc
          (tree-up (tree-up doc))
          (cond ((preview-expand-context? enc) (set! doc (tree-up doc)))
                ((and (tree-in? enc (algorithm-tag-list))
                   (< (tree-index (tree-up doc)) (- (tree-arity enc) 1))
                 ) ;and
                 (set! doc `(with ,"par-first" ,"0em" ,(tree-up doc)))
                ) ;
          ) ;cond
        ) ;with
      ) ;when
      (when (tm-is? doc 'row)
        (set! doc (apply tmconcat (map uncell (tm-children doc))))
      ) ;when
      (set! doc (clean-preview doc))
      (set! doc (fix-fig_or_tb-number doc id))
      (when math?
        (set! doc `(with ,"math-display" ,"true" (math ,doc)))
      ) ;when
      `(preview-balloon ,doc)
    ) ;with
  ) ;and-with
) ;define

(tm-define (ref-preview id)
  (and-with p
    (and-nnull? (label->path id))
    (with t
      (path->tree (cDr p))
      (cond ((label-context? t) (label-preview t id))
            ((tree-in? t
               '(glossary glossary-explain
                  glossary-dup
                  index
                  subindex
                  subsubindex
                  index-complex)
             ) ;tree-in?
             (label-preview t id)
            ) ;
            (else #f)
      ) ;cond
    ) ;with
  ) ;and-with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Previewing
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (preview-reference body body*)
  (:secure #t)
  (and-with ref
    (tree-up body)
    (with (x1 y1 x2 y2)
      (tree-bounding-rectangle ref)
      (and-let* ((id (and (tree-atomic? body*) (tree->string body*)))
                 (tip (and id (ref-preview id)))
                ) ;
        (show-tooltip id ref tip "Top" "Top" "default" 2.2)
      ) ;and-let*
    ) ;with
  ) ;and-with
) ;tm-define

(tm-define (update-preview-tooltip)
  (:secure #t)
  (let* ((id (and (tree-atomic? (cursor-tree)) (tm->string (cursor-tree))))
         (tip (and id (ref-preview id)))
        ) ;
    (if (and id tip)
      (begin
        (close-tooltip)
        (delayed (:idle 10)
          (show-tooltip id (cursor-tree) tip "Top" "Top" "keyboard" 2.2)
        ) ;delayed
      ) ;begin
      (close-tooltip)
    ) ;if
  ) ;let*
) ;tm-define

(tm-define (keyboard-press key time)
  (with before?
    (in-inactive-reference?)
    (former key time)
    (with after?
      (in-inactive-reference?)
      (when (and (or before? after?) (in-preview-ref?))
        (delayed (:idle 100)
          (let* ((id1 (and (in-inactive-reference?) (tm->string (cursor-tree))))
                 (tip1 (and id1 (ref-preview id1)))
                 (id2 (and id1 (not tip1) (number->label (cursor-tree))))
                 (tip2 (and id2 (ref-preview id2)))
                 (id (if tip1 id1 id2))
                 (tip (or tip1 tip2))
                ) ;
            (if tip
              (begin
                (show-tooltip id (cursor-tree) tip "Top" "Top" "keyboard" 1.8)
                (when (not tip1)
                  (set-message `(concat (verbatim ,id1)
                                  ," <rightarrow> "
                                  (verbatim ,id2)) "")
                ) ;when
              ) ;begin
              (begin
                (close-tooltip)
                (set-message "" "")
              ) ;begin
            ) ;if
          ) ;let*
        ) ;delayed
      ) ;when
    ) ;with
  ) ;with
) ;tm-define
