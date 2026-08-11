
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tmtex-acm.scm
;; DESCRIPTION : special conversions for acm styles
;; COPYRIGHT   : (C) 2012  Joris van der Hoeven, Francois Poulain
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (latex convert-latex-tmtex-acm)
  (:use (latex convert-latex-tmtex) (latex convert-latex-define))
) ;texmacs-module

(tm-define (tmtex-transform-style x)
  (:mode acm-style?)
  (cond ((== x "acmconf") "acm_proc_article-sp")
        ((== x "sig-alternate") x)
        ((== x "acmsmall") '("format=acmsmall" "acmart"))
        ((== x "acmlarge") '("format=acmlarge" "acmart"))
        ((== x "acmtog") '("format=acmtog" "acmart"))
        ((== x "sigconf") '("format=sigconf" "acmart"))
        ((== x "sigchi") '("format=sigchi" "acmart"))
        ((== x "sigplan") '("format=sigplan" "acmart"))
        (else x)
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; New ACM metadata presentation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (tmtex-make-acm-art-title titles miscs tr)
  (let* ((titles (tmtex-concat-Sep (map cadr titles))) (content `(,@titles
                                                                  ,@miscs)))
    (if (null? content) '() `((title (!indent (!paragraph ,@content)))))
  ) ;let*
) ;define

(define (rewrite-author a)
  (cond ((not (func? a 'author 1)) (list a))
        ((not (func? (cadr a) '!paragraph)) (list a))
        (else (cons `(author ,(cadr (cadr a))) (cddr (cadr a))))
  ) ;cond
) ;define

(tm-define (tmtex-append-authors l)
  (:mode acm-art-style?)
  (set! l (filter nnull? l))
  (with r (append-map rewrite-author l) `((!document ,@r)))
) ;tm-define

(tm-define (tmtex-make-doc-data titles
             subtitles
             authors
             dates
             miscs
             notes
             subtits-l
             dates-l
             miscs-l
             notes-l
             tr
             ar
           ) ;tmtex-make-doc-data
  (:mode acm-art-style?)
  `(!document ,@(tmtex-make-acm-art-title titles miscs tr)
     ,@subtitles
     ,@notes
     ,@(tmtex-append-authors authors)
     ,@dates
     (maketitle))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; New ACM specific titlemarkup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tmtex-doc-subtitle t)
  (:mode acm-art-style?)
  `(subtitle ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-doc-note t)
  (:mode acm-art-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(titlenote ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-doc-misc t)
  (:mode acm-art-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(tmacmmisc ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-doc-date t) (:mode acm-art-style?) `(date ,(tmtex (cadr t))))

(tm-define (tmtex-author-name t)
  (:mode acm-art-style?)
  `(author ,(tmtex-inline (cadr t)))
) ;tm-define

(define (get-affiliation-lines aff)
  (if (func? aff 'concat)
    (list-filter (cdr aff) (lambda (x) (!= x '(next-line))))
    (list aff)
  ) ;if
) ;define

(tm-define (tmtex-author-affiliation t)
  (:mode acm-art-style?)
  (let* ((l (if (null? (cdr t)) '() (get-affiliation-lines (cadr t)))) (r (list)))
    (when (nnull? l)
      (set! r (rcons r `(institution ,(tmtex (car l)))))
      (set! l (cdr l))
    ) ;when
    (when (nnull? l)
      (set! r (rcons r `(streetaddress ,(tmtex (car l)))))
      (set! l (cdr l))
    ) ;when
    (when (nnull? l)
      (set! r (rcons r `(city ,(tmtex (car l)))))
      (set! l (cdr l))
    ) ;when
    (when (nnull? l)
      (set! r (rcons r `(country ,(tmtex (car l)))))
      (set! l (cdr l))
    ) ;when
    `(affiliation (!paragraph ,@r))
  ) ;let*
) ;tm-define

(tm-define (tmtex-author-email t)
  (:mode acm-art-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(email ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-author-homepage t)
  (:mode acm-art-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(tmacmhomepage ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-author-note t)
  (:mode acm-art-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(authornote ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-author-misc t)
  (:mode acm-art-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(tmacmmisc ,(tmtex (cadr t)))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Old ACM metadata presentation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tmtex-append-authors l)
  (:mode acm-conf-style?)
  (set! l (filter nnull? l))
  (if (null? l)
    l
    (let* ((n (number->string (length l)))
           (sep '(!concat (!linefeed) (alignauthor) (!linefeed)))
          ) ;
      (set! l (list-intersperse (map cadr l) sep))
      `((!document (numberofauthors ,n)
          (author (!indent (!concat (alignauthor) ," " ,@l)))))
    ) ;let*
  ) ;if
) ;tm-define

(tm-define (tmtex-make-author names
             affiliations
             emails
             urls
             miscs
             notes
             affs-l
             emails-l
             urls-l
             miscs-l
             notes-l
           ) ;tmtex-make-author
  (:mode acm-conf-style?)
  (let* ((names (tmtex-concat-Sep (map cadr names)))
         (result `(,@names ,@urls ,@notes ,@miscs ,@affiliations ,@emails))
        ) ;
    (if (null? result) '() `(author (!concat ,@result)))
  ) ;let*
) ;tm-define

(define (tmtex-make-acm-conf-title titles notes miscs)
  (let* ((titles (tmtex-concat-Sep (map cadr titles)))
         (result `(,@titles ,@notes ,@miscs))
        ) ;
    (if (null? result) '() `((title (!concat ,@result))))
  ) ;let*
) ;define

(tm-define (tmtex-make-doc-data titles
             subtitles
             authors
             dates
             miscs
             notes
             subtits-l
             dates-l
             miscs-l
             notes-l
             tr
             ar
           ) ;tmtex-make-doc-data
  (:mode acm-conf-style?)
  `(!document ,@(tmtex-make-acm-conf-title titles notes miscs)
     ,@subtitles
     ,@(tmtex-append-authors authors)
     ,@dates
     (maketitle))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; Old ACM specific titlemarkup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (acm-line-break t)
  `(!concat (!nextline) ,t)
) ;define

(tm-define (tmtex-doc-subtitle t)
  (:mode acm-conf-style?)
  `(subtitle ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-doc-note t)
  (:mode acm-conf-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(titlenote ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-doc-misc t)
  (:mode acm-conf-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(tmacmmisc ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-doc-date t) (:mode acm-conf-style?) `(date ,(tmtex (cadr t))))

(tm-define (tmtex-author-affiliation t)
  (:mode acm-conf-style?)
  (with aff-lines
    (if (list>0? (cadr t))
      (map (lambda (x) (if (== x '(next-line)) '(!nextline) `(affaddr ,(tmtex x))))
        (cdadr t)
      ) ;map
      (if (null? (cdr t)) '() `((affaddr ,(tmtex (cadr t)))))
    ) ;if
    (acm-line-break `(!concat ,@aff-lines))
  ) ;with
) ;tm-define

(tm-define (tmtex-author-email t)
  (:mode acm-conf-style?)
  (set! t (tmtex-remove-line-feeds t))
  (acm-line-break `(email ,(tmtex (cadr t))))
) ;tm-define

(tm-define (tmtex-author-homepage t)
  (:mode acm-conf-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(tmacmhomepage ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-author-note t)
  (:mode acm-conf-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(titlenote ,(tmtex (cadr t)))
) ;tm-define

(tm-define (tmtex-author-misc t)
  (:mode acm-conf-style?)
  (set! t (tmtex-remove-line-feeds t))
  `(tmacmmisc ,(tmtex (cadr t)))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ACM specific abstract markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tmtex-make-abstract-data keywords acm arxiv msc pacs abstract)
  (:mode acm-style?)
  (with result
    `(,@abstract ,@acm ,@arxiv ,@msc ,@pacs ,@keywords)
    (if (null? result) "" `(!document ,@result))
  ) ;with
) ;tm-define

(tm-define (tmtex-abstract-keywords t)
  (:mode acm-style?)
  (with args
    (tmtex-concat-sep (map tmtex (cdr t)))
    `(keywords ,@(map tmtex args))
  ) ;with
) ;tm-define

(tm-define (tmtex-abstract-acm t)
  (:mode acm-style?)
  (with l
    (cond ((== (length (cdr t)) 0) '("" "" ""))
          ((== (length (cdr t)) 1) (append (cdr t) '("" "")))
          ((== (length (cdr t)) 2) (append (cdr t) '("")))
          ((== (length (cdr t)) 3) (cdr t))
          (else (append (sublist (cdr t) 0 3)
                  `((!option ,(fourth (cdr t))))
                  (sublist (cdr t) 4 (length (cdr t)))
                ) ;append
          ) ;else
    ) ;cond
    `(category ,@(map tmtex l))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;; ACM specific misc markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tmtex-acm-conferenceinfo s l)
  (:mode acm-style?)
  `(conferenceinfo ,@(map tmtex l))
) ;tm-define

(tm-define (tmtex-acm-copyright-year s l)
  (:mode acm-style?)
  `(CopyrightYear ,@(map tmtex l))
) ;tm-define

(tm-define (tmtex-acm-crdata s l) (:mode acm-style?) `(crdata ,@(map tmtex l)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Put 'maketitle' after abstract
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define removed-maketitle? #f)

(define added-maketitle? #f)

(define (remove-maketitle t)
  (cond ((nlist? t) t)
        ((and (func? t '!document) (== (cAr t) '(maketitle)))
         (set! removed-maketitle? #t)
         (cDr t)
        ) ;
        (else (map remove-maketitle t))
  ) ;cond
) ;define

(define (add-maketitle-sub l)
  (cond ((null? l) l)
        ((and (pair? (car l)) (== (caar l) '(!begin "abstract")))
         (set! added-maketitle? #t)
         (cons (car l) (cons '(maketitle) (cdr l)))
        ) ;
        (else (cons (add-maketitle (car l)) (add-maketitle-sub (cdr l))))
  ) ;cond
) ;define

(define (add-maketitle t)
  (cond ((nlist? t) t)
        ((func? t '!document) (cons (car t) (add-maketitle-sub (cdr t))))
        (else (map add-maketitle t))
  ) ;cond
) ;define

(tm-define (tmtex-postprocess x)
  (:mode acm-style?)
  (set! removed-maketitle? #f)
  (set! added-maketitle? #f)
  (let* ((y (remove-maketitle x)) (z (add-maketitle y)))
    (if (and removed-maketitle? added-maketitle?) z x)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ACM specific macros
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(logic-group latex-texmacs-3% (:mode acm-style?) category)

(smart-table latex-texmacs-macro
  (:mode acm-style?)
  (qed #f)
  (nequiv #f)
  (category "")
) ;smart-table

(smart-table latex-texmacs-environment (:mode acm-style?) ("proof" #f))

;; (tm-define (tmtex-cite-detail s l)
;;  (:mode acm-style?)
;;  (tmtex-cite-detail-poor s l))

(smart-table latex-texmacs-env-preamble
  (:mode acm-art-style?)
  ("theorem" #f)
  ("conjecture" #f)
  ("proposition" #f)
  ("lemma" #f)
  ("corollary" #f)
  ("definition" #f)
  ("example" #f)
) ;smart-table

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Missing theorem types
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-macro (acm-theorem prim name)
  `(latex-texmacs-thmenv ,prim ,name ,() ,() acm-art-style?)
) ;define-macro

(define-macro (acm-remark prim name)
  `(latex-texmacs-thmenv ,prim
     ,name
     ("\\theoremstyle{acmdefinition}\n")
     ("\n\\theoremstyle{acmplain}")
     acm-art-style?)
) ;define-macro

(define-macro (acm-exercise prim name)
  `(latex-texmacs-thmenv ,prim
     ,name
     ("\\theoremstyle{acmdefinition}\n")
     ("\n\\theoremstyle{acmplain}")
     acm-art-style?)
) ;define-macro

(acm-theorem "axiom" "Axiom")
(acm-theorem "notation" "Notation")
(acm-remark "remark" "Remark")
(acm-remark "note" "Note")
(acm-remark "convention" "Convention")
(acm-remark "warning" "Warning")
(acm-remark "acknowledgments" "Acknowledgments")
(acm-remark "answer" "Answer")
(acm-remark "question" "Question")
(acm-remark "remark" "Remark")
(acm-remark "problem" "Problem")
(acm-remark "solution" "Solution")
(acm-exercise "exercise" "Exercise")
(acm-exercise "problem" "Problem")
(acm-exercise "solution" "Solution")
