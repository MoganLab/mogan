
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tm-states.scm
;; DESCRIPTION : Dynamic states around (tm-define)-d functions
;; COPYRIGHT   : (C) 2007  Joris van der Hoeven and Henri Lesourd
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel texmacs tm-states))

;; Low-level API
(define-public (state-create slotlist)
  ;; slotlist === ((<SLOT-DESCR>*) (<PROP-DESCR>*) (<COMPILED-PROP>*))
  slotlist
) ;define-public

(define (state-slots sr)
  (car sr)
) ;define

(define (state-props sr)
  (cadr sr)
) ;define

(define (state-cprops sr)
  (caddr sr)
) ;define

(define (slotlist-load l)
  (for (e l)
    (eval `(begin
             (if (not (defined? (quote ,(car e)))) (define-public ,(car e) ,#f))
             (set! ,(car e)
               ,(with val
                  (cadr e)
                  (if (and (pair? val) (eq? (car val) 'quote))
                    val
                    `(quote ,(eval val))))))
    ) ;eval
  ) ;for
) ;define

(define (proplist-load l funcs b)
  (for (e l) (if (not (defined? (car e))) (eval `(define-public ,(car e) ,#f))))
  (if b (for (f funcs) (f)))
) ;define

(define (slotlist-save l)
  (for (e l) (set-car! (cdr e) `(quote ,(eval (car e)))))
) ;define

;; User-level API

(define current-state #f)

(define-public (state-synchronize)
  (with old-current
    current-state
    (set! current-state #f)
    (state-save old-current)
  ) ;with
) ;define-public

(define-public (state-load sr . opt)
  (if sr
    (begin
      (if (not (eq? sr current-state))
        (begin
          (state-synchronize)
          (slotlist-load (state-slots sr))
        ) ;begin
      ) ;if
      (proplist-load (state-props sr) (state-cprops sr) (null? opt))
      (set! current-state sr)
    ) ;begin
  ) ;if
) ;define-public

(define-public (state-save sr)
  (if (and sr (not (eq? sr current-state))) (slotlist-save (state-slots sr)))
) ;define-public

(define (state-slotnames sr)
  (map car (state-slots sr))
) ;define

(define (state-propnames sr)
  (map car (state-props sr))
) ;define

(define-public (state-names sr)
  (append (state-slotnames sr) (state-propnames sr))
) ;define-public

(define-public (state-type sr name)
  (cond ((in? name (state-slotnames sr)) 'slot)
        ((in? name (state-propnames sr)) 'prop)
        (else #f)
  ) ;cond
) ;define-public

(define (seek-pred? pred? l)
  (define res #f)
  (for (e l) (if (pred? (car e)) (set! res e)))
  res
) ;define

(define (state-entry sr name)
  (define ref #f)
  (set! ref (seek-pred? (lambda (x) (eq? x name)) (state-slots sr)))
  (if (not ref)
    (set! ref (seek-pred? (lambda (x) (eq? x name)) (state-props sr)))
  ) ;if
  ref
) ;define

(define-public (state-read sr name)
  (define ref #f)
  (set! ref (state-entry sr name))
  (if ref (cadr ref))
) ;define-public

(define-public (state-write sr name val)
  (define ref #f)
  (set! ref (state-entry sr name))
  (if ref (set-car! (cdr ref) val))
) ;define-public

(define-public-macro (define-state name . slotlists)
  ;; slotlists === ((slots ((<NAME-SLOT1> <INIT1>) ... (<NAME-SLOTN> <INITN>)))
  ;;                (props ((<NAME-PROP1> <PROP1>) ... (<NAME-PROPN> <PROPN>))))
  (let* ((theslots (copy-tree slotlists))
         (slots (cadr (car theslots)))
         (props (cadr (cadr theslots)))
        ) ;
    `(begin
       (if (not (defined? (quote ,name))) (define-public ,name ,#f))
       (with cprops
         ,#f
         (set! cprops
           (map (lambda (x)
                  (eval (#_list-values
                         'lambda
                         ()
                         (#_list-values 'set! (car x) (cadr x)))))
             (quote ,props)))
         (set! ,name
           (state-create (append (quote (,slots ,props)) (#_list-values cprops))))))
  ) ;let*
) ;define-public-macro

(define-public-macro (with-state sr . body)
  `(begin (state-load ,sr) (with res (begin unquote body) (state-save ,sr) res))
) ;define-public-macro

(define-public-macro (with-state-by-name name . body)
  `(with sr ,name (with-state sr unquote body))
) ;define-public-macro

(define-public-macro (with-state-slots sr . body)
  `(begin
     (state-load ,sr ,#f)
     (with res (begin unquote body) (state-save ,sr) res))
) ;define-public-macro

(define-public-macro (with-state-slots-by-name name . body)
  `(with sr ,name (with-state-slots sr unquote body))
) ;define-public-macro
