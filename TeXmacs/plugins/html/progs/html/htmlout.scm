
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : htmlout.scm
;; DESCRIPTION : generation of Html from scheme expressions
;; COPYRIGHT   : (C) 2002  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (html htmlout) (:use (convert tools output)))

(define preformatted? #f)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Data
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(logic-group htmlout-big-all%
  ;; Both the tag and the children are displayed in multi-line format.
  html
  head
  style
  body
  table
  tr
  ul
  ol
  dl
  ;; and for MathML
  mtable
  mtr
) ;logic-group

(logic-group htmlout-big-tag%
  ;; The tag is displayed in multi-line format.
  div
  p
  li
  dt
  dd
  center
  blockquote
) ;logic-group

(logic-rule (htmlout-big-tag% 'x) (htmlout-big-all% 'x))

(define (htmlout-big-all? op)
  (logic-in? op htmlout-big-all%)
) ;define

(define (htmlout-big-tag? op)
  (logic-in? op htmlout-big-tag%)
) ;define

(define (htmlout-big? x)
  (and (pair? x)
    (or (htmlout-big-all? (car x))
      (and (htmlout-big-tag? (car x))
        (list-any (lambda (x) (and (pair? x) (htmlout-big-tag? (car x)))) (cdr x))
      ) ;and
    ) ;or
  ) ;and
) ;define

(define (htmlout-p-simplify? x)
  ;; FIXME: font should not really be in the list here
  (and (func? x 'p 1)
    (pair? (cadr x))
    (in? (caadr x) '(div p li dt dd center blockquote ul ol dl))
  ) ;and
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Outputting main flow
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmlout-indent* s plus close?)
  (if (not preformatted?)
    (cond ((htmlout-big-tag? s) (output-indent plus) (output-lf))
          ((== s 'pre) (if (not close?) (output-lf-verbatim)))
    ) ;cond
  ) ;if
) ;define

(define (htmlout-indent s plus)
  (htmlout-indent* s plus #f)
) ;define

(define (htmlout-indent-close s plus)
  (htmlout-indent* s plus #t)
) ;define

(define (htmlout-text . ss)
  (if preformatted? (apply output-verbatim ss) (apply output-text ss))
) ;define

(define (htmlout-open s)
  (htmlout-text "<" (symbol->string s) ">")
  (htmlout-indent s 2)
) ;define

(define (htmlout-tag x)
  (if (== (cadr x) "<implicit>")
    (output-text " " (symbol->string (car x)))
    (begin
      (output-text " " (symbol->string (car x)) "=")
      (output-verbatim "\"" (cadr x) "\"")
    ) ;begin
  ) ;if
) ;define

(define (htmlout-open-tags s l)
  (with ll
    (ahash-table->list (list->ahash-table l))
    (htmlout-text "<" (symbol->string s))
    (for-each htmlout-tag ll)
    ;; If tag is a self-closing (void) element according to HTML5 spec, close it with "/>"
    ;; "Void elements only have a start tag; end tags must not be specified for void elements."
    ;; Reference: HTML5 spec, section 13.1.2 - Elements (https://html.spec.whatwg.org/multipage/syntax.html#void-elements)
    (if (member s '(area base br col embed hr img input link meta source track
                     wbr))
      (htmlout-text " />")
      (htmlout-text ">")
    ) ;if
    (htmlout-indent s 2)
  ) ;with
) ;define

(define (htmlout-close s)
  ;; Do not close the tag if it is a self-closing (void) element
  ;; Reference: HTML5 spec, section 13.1.2 - Elements (https://html.spec.whatwg.org/multipage/syntax.html#void-elements)
  (if (not (member s '(area base br col embed hr img input link meta source
                        track wbr))
      ) ;not
    (begin
      (htmlout-indent-close s -2)
      (htmlout-text "</" (symbol->string s) ">")
    ) ;begin
  ) ;if
) ;define

(define (htmlout-args-sub l big?)
  (if (nnull? l)
    (begin
      (htmlout (car l))
      (if (and big? (nnull? (cdr l))) (output-lf))
      (htmlout-args-sub (cdr l) big?)
    ) ;begin
  ) ;if
) ;define

(define (htmlout-args s l)
  (with big? (htmlout-big? (cons s l)) (htmlout-args-sub l big?))
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main output routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (htmlout-doctype l)
  (output-lf-verbatim "<!DOCTYPE html>")
  (output-lf)
) ;define

(define (htmlout x)
  (cond ((string? x) (htmlout-text x))
        ((null? x) (noop))
        ((or (func? x '!concat) (func? x '*TOP*)) (for-each htmlout (cdr x)))
        ((htmlout-p-simplify? x) (htmlout (cadr x)))
        ((func? x '*PI*)
         (output-lf-verbatim "<?" (symbol->string (cadr x)) " " (caddr x) "?>")
         (output-lf)
        ) ;
        ((func? x '*DOCTYPE*) (htmlout-doctype (cdr x)))
        ((== x '(br)) (htmlout-text "<br />"))
        ((null? (cdr x)) (htmlout-open (car x)) (htmlout-close (car x)))
        ((not (func? (cadr x) '@))
         (htmlout-open (car x))
         (htmlout-args (car x) (cdr x))
         (htmlout-close (car x))
        ) ;
        (else (htmlout-open-tags (car x) (cdadr x))
          (update-preformatted (cdadr x) (cut htmlout-args (car x) (cddr x)))
          (htmlout-close (car x))
        ) ;else
  ) ;cond
) ;define

(define (update-preformatted atts thunk)
  (let ((saved-preformatted preformatted?)
        (new-preformatted (cond ((assoc 'xml:space atts)
                                 =>
                                 (lambda (att)
                                   (cond ((== (second att) "preserve") #t)
                                         ((== (second att) "default") #f)
                                         (else preformatted?)
                                   ) ;cond
                                 ) ;lambda
                                ) ;
                                (else preformatted?)
                          ) ;cond
        ) ;new-preformatted
       ) ;
    (if (== new-preformatted saved-preformatted)
      (thunk)
      (dynamic-wind (lambda () (set! preformatted? new-preformatted))
        thunk
        (lambda () (set! preformatted? saved-preformatted))
      ) ;dynamic-wind
    ) ;if
  ) ;let
) ;define

(tm-define (serialize-html x) (htmlout x) (output-produce))
