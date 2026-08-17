
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : maxima-input.scm
;; DESCRIPTION : Initialize maxima plugin
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven, 2005  Andrey Grozin
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (maxima maxima-input) (:use (utils plugins plugin-convert)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Specific conversion routines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (maxima-input-var-row r)
  (if (nnull? r)
    (begin
      (plugin-output ", ")
      (plugin-input (car r))
      (maxima-input-var-row (cdr r))
    ) ;begin
  ) ;if
) ;define

(define (maxima-input-row r)
  (plugin-output "[")
  (plugin-input (car r))
  (maxima-input-var-row (cdr r))
  (plugin-output "]")
) ;define

(define (maxima-input-var-rows t)
  (if (nnull? t)
    (begin
      (plugin-output ", ")
      (maxima-input-row (car t))
      (maxima-input-var-rows (cdr t))
    ) ;begin
  ) ;if
) ;define

(define (maxima-input-rows t)
  (plugin-output "matrix(")
  (maxima-input-row (car t))
  (maxima-input-var-rows (cdr t))
  (plugin-output ")")
) ;define

(define (maxima-input-descend-last args)
  (if (null? (cdr args))
    (plugin-input (car args))
    (maxima-input-descend-last (cdr args))
  ) ;if
) ;define

(define (maxima-input-det args)
  (plugin-output "determinant(")
  (maxima-input-descend-last args)
  (plugin-output ")")
) ;define

(define (maxima-input-binom args)
  (plugin-output "binomial(")
  (plugin-input (car args))
  (plugin-output ",")
  (plugin-input (cadr args))
  (plugin-output ")")
) ;define

(define (maxima-input-sqrt args)
  (if (= (length args) 1)
    (begin
      (plugin-output "sqrt(")
      (plugin-input (car args))
      (plugin-output ")")
    ) ;begin
    (begin
      (plugin-output "(")
      (plugin-input (car args))
      (plugin-output ")^(1/(")
      (plugin-input (cadr args))
      (plugin-output "))")
    ) ;begin
  ) ;if
) ;define

(define (maxima-input-sum args)
  (if (nnull? args)
    (if (nnull? (cdr args))
      (begin
        ;; both lower and upper index
        (plugin-output "tmsum(")
        (plugin-input (car args))
        (plugin-output ",")
        (plugin-input (cadr args))
        (plugin-output ",")
      ) ;begin
      (begin
        ;; lower index only
        (plugin-output "tmlsum(")
        (plugin-input (car args))
        (plugin-output ",")
      ) ;begin
    ) ;if
    (plugin-output "tmsum(")
  ) ;if
) ;define

(define (maxima-input-prod args)
  (if (nnull? args)
    (begin
      (plugin-output "tmprod(")
      (plugin-input (car args))
      (if (nnull? (cdr args)) (begin (plugin-output ",") (plugin-input (cadr args))))
      (plugin-output ",")
    ) ;begin
    (plugin-output "tmprod(")
  ) ;if
) ;define

(define (maxima-input-int args)
  (if (nnull? args)
    (begin
      (plugin-output "tmint(")
      (plugin-input (car args))
      (if (nnull? (cdr args)) (begin (plugin-output ",") (plugin-input (cadr args))))
      (plugin-output ",")
    ) ;begin
    (plugin-output "integrate(")
  ) ;if
) ;define


(define (maxima-input-big-around args)
  (let* ((b `(big-around ,@args))
         (op (big-name b))
         (sub (big-subscript b))
         (sup (big-supscript b))
         (body (big-body b))
         (l (cond ((and sub sup) (list sub sup)) (sub (list sub)) (else (list))))
        ) ;
    (cond ((== op "sum") (maxima-input-sum l))
          ((== op "prod") (maxima-input-prod l))
          ((== op "int") (maxima-input-int l))
          (else (plugin-output op) (plugin-output "("))
    ) ;cond
    (plugin-input body)
    (plugin-output ")")
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Initialization
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(plugin-input-converters maxima
  (rows maxima-input-rows)
  (det maxima-input-det)
  (sqrt maxima-input-sqrt)
  (big-around maxima-input-big-around)
  (binom maxima-input-binom)

  ("<infty>" "inf")
  ("<emptyset>" "[]")
  ("<mathd>" "1,")
  ("<mathi>" "%i")
  ("<mathe>" "%e")
  ("<in>" "=")

  ("<times>" "~")
  ("<cdot>" ".")

  ("<gamma>" "%gamma")
  ("<pi>" "%pi")
) ;plugin-input-converters
