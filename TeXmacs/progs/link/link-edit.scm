
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : link-edit.scm
;; DESCRIPTION : editing routines for links
;; COPYRIGHT   : (C) 2006  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (link link-edit) (:use (link locus-edit)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Current link mode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define current-link-mode "bidirectional")

(define (in-link-mode? mode)
  (== current-link-mode mode)
) ;define

(tm-define (set-link-mode mode)
  (:synopsis "Set the current linking mode to @mode")
  (:check-mark "v" in-link-mode?)
  (set! current-link-mode mode)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Utility routines for links
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (list->assoc-list l)
  (if (or (null? l) (null? (cdr l)))
    '()
    (cons (cons (car l) (cadr l)) (list->assoc-list (cddr l)))
  ) ;if
) ;define

(define (link-flatten-sub t)
  (cond ((tm-func? t 'script)
         (cons* 'script (tree->stree (tree-ref t 0)) (cdr (tree-children t)))
        ) ;
        ((tm-func? t 'attr) (list->assoc-list (cdr (tree->stree t))))
        (else (tree->stree t))
  ) ;cond
) ;define

(tm-define (link-flatten ln)
  (cons (tm-car ln) (map link-flatten-sub (tm-cdr ln)))
) ;tm-define

(tm-define (link-type ln)
  (if (tree? ln) (set! ln (link-flatten ln)))
  (and (func? ln 'link) (cadr ln))
) ;tm-define

(tm-define (link-attributes ln)
  (if (tree? ln) (set! ln (link-flatten ln)))
  (and (func? ln 'link) (caddr ln))
) ;tm-define

(tm-define (link-vertices ln)
  (if (tree? ln) (set! ln (link-flatten ln)))
  (and (func? ln 'link) (cdddr ln))
) ;tm-define

(tm-define (link-source ln) (car (link-vertices ln)))

(tm-define (link-target ln) (cadr (link-vertices ln)))

(tm-define (vertex->id r) (and (func? r 'id 1) (string? (cadr r)) (cadr r)))

(tm-define (vertex->url r) (and (func? r 'url 1) (string? (cadr r)) (cadr r)))

(tm-define (vertex->script r) (and (func? r 'script) (cadr r)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Entering the necessary information for the next link
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define link-participants (make-ahash-table))

(define (link-on-locus? nr)
  (if (not (inside? 'locus))
    ""
    (with-innermost t
      'locus
      (with tp
        (ahash-ref link-participants nr)
        (cond ((not (observer? tp)) "")
              ((== t (tree-pointer->tree tp)) "v")
              (else "o")
        ) ;cond
      ) ;with
    ) ;with-innermost
  ) ;if
) ;define

(tm-define (link-set-locus nr)
  (:synopsis "Set locus number @nr of a link")
  (:check-mark "auto" link-on-locus?)
  (with-innermost t
    'locus
    (ahash-set! link-participants nr (tree->tree-pointer t))
  ) ;with-innermost
) ;tm-define

(define (link-component-is-url? nr . args)
  (func? (ahash-ref link-participants nr) 'url)
) ;define
(tm-define (link-set-url nr name)
  (:synopsis "Set component @nr of a link to an url @name")
  (:check-mark "o" link-component-is-url?)
  (ahash-set! link-participants nr `(url ,name))
) ;tm-define

(tm-define (link-target-is-url? . args)
  (func? (ahash-ref link-participants 1) 'url)
) ;tm-define
(tm-define (link-set-target-url url)
  (:synopsis "Set target of link to an @url")
  (:check-mark "o" link-target-is-url?)
  (ahash-set! link-participants 1 `(url ,url))
) ;tm-define

(define (link-component-is-script? nr . args)
  (func? (ahash-ref link-participants nr) 'script)
) ;define
(tm-define (link-set-script nr script)
  (:synopsis "Set component @nr of a link to a @script")
  (:check-mark "o" link-component-is-script?)
  (ahash-set! link-participants nr `(script ,script))
) ;tm-define

(tm-define (link-target-is-script? . args)
  (func? (ahash-ref link-participants 1) 'script)
) ;tm-define
(tm-define (link-set-target-script script)
  (:synopsis "Set target of link to a @script")
  (:check-mark "o" link-target-is-script?)
  (ahash-set! link-participants 1 `(script ,script))
) ;tm-define

(tm-define (link-completed?)
  (:synopsis "Did enter all necessary information for constructing a link?")
  (and (ahash-ref link-participants 0)
    (ahash-ref link-participants 1)
    (cond ((== current-link-mode "simple") #t)
          ((== current-link-mode "bidirectional") #t)
          ((== current-link-mode "external") (inside? 'locus))
          (else #f)
    ) ;cond
  ) ;and
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Building the next link
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (link-get-trees nr)
  (with p
    (ahash-ref link-participants nr)
    (cond ((not p) '())
          ((observer? p)
           (with t (tree-pointer->tree p) (cons t (link-get-trees (+ nr 1))))
          ) ;
          (else (link-get-trees (+ nr 1)))
    ) ;cond
  ) ;with
) ;define

(define (link-vertices nr)
  (with p
    (ahash-ref link-participants nr)
    (cond ((not p) '())
          ((observer? p)
           (with t
             (tree-pointer->tree p)
             (cons `(id ,(locus-id t)) (link-vertices (+ nr 1)))
           ) ;with
          ) ;
          (else (cons p (link-vertices (+ nr 1))))
    ) ;cond
  ) ;with
) ;define

(define (link-remove-participant nr)
  (with p
    (ahash-ref link-participants nr)
    (ahash-remove! link-participants nr)
    (when (observer? p)
      (tree-pointer-detach p)
    ) ;when
  ) ;with
) ;define

(define (link-clean nr)
  (and-with tp
    (ahash-ref link-participants nr)
    (ahash-remove! link-participants nr)
    (if (observer? tp) (tree-pointer-detach tp))
    (link-clean (+ nr 1))
  ) ;and-with
) ;define

(define (link-build type)
  (let* ((ts (link-get-trees 0))
         (vs (link-vertices 0))
         (ln (tm->tree `(link ,type ,@vs)))
        ) ;
    (when (and (nnull? vs) (list-and (map locus-id ts)))
      (cond ((== current-link-mode "simple")
             (and-let* ((tp (ahash-ref link-participants 0))
                        (d (observer? tp))
                        (t (tree-pointer->tree tp))
                       ) ;
               (locus-insert-link t ln)
             ) ;and-let*
            ) ;
            ((== current-link-mode "bidirectional")
             (for-each (cut locus-insert-link <> ln) ts)
            ) ;
            ((== current-link-mode "external")
             (with-innermost t 'locus (locus-insert-link t ln))
            ) ;
            (else (set-message "Unsupported link mode" "Make link"))
      ) ;cond
    ) ;when
  ) ;let*
) ;define

(tm-define (make-link type)
  (:synopsis "Make a typed link")
  (:argument type "Link type")
  (when (link-completed?)
    (for-each link-build (string-tokenize-comma type))
    (link-clean 0)
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Destruction of links
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (locus-consider-link? ln check-mode?)
  (and (tm-func? ln 'link)
    (or (not check-mode?)
      (cond ((== current-link-mode "simple")
             (and-let* ((id1 (locus-id (tree-up ln))) (id2 (link-source ln))) (== id1 id2))
            ) ;
            ((== current-link-mode "bidirectional") #t)
            ((== current-link-mode "external") #t)
            (else #f)
      ) ;cond
    ) ;or
  ) ;and
) ;define

(tm-define (locus-link-types check-mode?)
  (:synopsis "Get all link types occurring in the current locus")
  (:argument check-mode? "Filter according to the current link mode?")
  (if (not (inside? 'locus))
    '()
    (with-innermost t
      'locus
      (let* ((l1 (cDr (tree-children t)))
             (l2 (list-filter l1 (cut locus-consider-link? <> check-mode?)))
             (l3 (filter-map link-type l2))
            ) ;
        (list-remove-duplicates l3)
      ) ;let*
    ) ;with-innermost
  ) ;if
) ;tm-define

(define (remove-link ln type)
  (with st
    (link-flatten ln)
    (when (and (func? st 'link) (== (cadr st) type))
      (cond ((== current-link-mode "simple") (locus-remove-link ln))
            ((== current-link-mode "bidirectional")
             (let* ((ids (filter-map vertex->id (link-vertices st)))
                    (ts1 (append-map id->loci ids))
                    (fun (lambda (t) (not (tree-eq? t (tree-up ln)))))
                    (ts2 (list-filter ts1 fun))
                   ) ;
               (for-each (cut locus-remove-all-links <> ln) ts2)
               (locus-remove-link ln)
             ) ;let*
            ) ;
            ((== current-link-mode "external") (locus-remove-link ln))
            (else (set-message "Unsupported link mode" "Remove link"))
      ) ;cond
    ) ;when
  ) ;with
) ;define

(define (remove-link-of-type type)
  (with-innermost t
    'locus
    (for-each (cut remove-link <> type) (reverse (cDr (tree-children t))))
  ) ;with-innermost
) ;define

(tm-define (remove-link-of-types type)
  (:synopsis "Remove all links of a given type")
  (:argument type "Link type")
  (:proposals type (locus-link-types #t))
  (for-each remove-link-of-type (string-tokenize-comma type))
) ;tm-define

(tm-define (remove-all-links)
  (:synopsis "Remove all links from the current locus")
  (for-each remove-link-of-type (locus-link-types #t))
) ;tm-define
