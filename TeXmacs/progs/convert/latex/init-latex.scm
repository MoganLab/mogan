
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : init-latex.scm
;; DESCRIPTION : setup latex converters
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (data latex))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; LaTeX format
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (latex-recognizes-at? s pos)
  (set! pos (format-skip-spaces s pos))
  (cond ((format-test? s pos "\\document") #t)
        ((format-test? s pos "\\usepackage") #t)
        ((format-test? s pos "\\input") #t)
        ((format-test? s pos "\\includeonly") #t)
        ((format-test? s pos "\\chapter") #t)
        ((format-test? s pos "\\appendix") #t)
        ((format-test? s pos "\\section") #t)
        ((format-test? s pos "\\begin") #t)
        (else #f)))

(define (latex-recognizes? s)
  (and (string? s) (latex-recognizes-at? s 0)))

(define-format latex
  (:name "LaTeX")
  (:suffix "tex")
  (:recognize latex-recognizes?))

(define-format latex-class
  (:name "LaTeX class")
  (:suffix "ltx" "sty" "cls"))

(define-preferences
  ("texmacs->latex:transparent-tracking" "on" noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TeXmacs->LaTeX
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(lazy-define (convert latex texout) serialize-latex)
(lazy-define (convert latex tmtex) texmacs->latex)

(converter texmacs-stree latex-stree
  (:function-with-options texmacs->latex)
  (:option "texmacs->latex:source-tracking" "off")
  (:option "texmacs->latex:conservative" "on")
  (:option "texmacs->latex:transparent-source-tracking" "on")
  (:option "texmacs->latex:attach-tracking-info" "on")
  (:option "texmacs->latex:replace-style" "on")
  (:option "texmacs->latex:expand-macros" "on")
  (:option "texmacs->latex:expand-user-macros" "off")
  (:option "texmacs->latex:indirect-bib" "off")
  (:option "texmacs->latex:use-macros" "on")
  (:option "texmacs->latex:encoding" "ascii"))

(converter latex-stree latex-document
  (:function serialize-latex))

(converter latex-stree latex-snippet
  (:function serialize-latex))

(tm-define (texmacs->latex-document x opts)
  (serialize-latex (texmacs->latex (tm->stree x) opts)))

(converter texmacs-stree latex-document
  (:function-with-options conservative-texmacs->latex)
  ;;(:function-with-options tracked-texmacs->latex)
  (:option "texmacs->latex:source-tracking" "off")
  (:option "texmacs->latex:conservative" "on")
  (:option "texmacs->latex:transparent-source-tracking" "on")
  (:option "texmacs->latex:attach-tracking-info" "on")
  (:option "texmacs->latex:replace-style" "on")
  (:option "texmacs->latex:expand-macros" "on")
  (:option "texmacs->latex:expand-user-macros" "off")
  (:option "texmacs->latex:indirect-bib" "off")
  (:option "texmacs->latex:use-macros" "on")
  (:option "texmacs->latex:encoding" "ascii"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; LaTeX -> TeXmacs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (latex-document->texmacs x . opts)
  (if (list-1? opts) (set! opts (car opts)))
  (with as-pic (== (get-preference "latex->texmacs:fallback-on-pictures") "on")
    (conservative-latex->texmacs x as-pic)))

(converter latex-document latex-tree
  (:function parse-latex-document))

(converter latex-snippet latex-tree
  (:function parse-latex))

(converter latex-document texmacs-tree
  (:function-with-options latex-document->texmacs)
  (:option "latex->texmacs:fallback-on-pictures" "on")
  (:option "latex->texmacs:source-tracking" "off")
  (:option "latex->texmacs:conservative" "off")
  (:option "latex->texmacs:transparent-source-tracking" "off"))

(converter latex-class-document texmacs-tree
  (:function latex-class-document->texmacs))

(converter latex-tree texmacs-tree
  (:function latex->texmacs))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Post-processing imported LaTeX: insert space between d and differential
;; variables so they are not merged into a single operator in math mode.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (is-letter-char? c)
  (and (char? c)
       (or (and (char>=? c #\a) (char<=? c #\z))
           (and (char>=? c #\A) (char<=? c #\Z)))))

(define (is-word-boundary-before? s i)
  (or (= i 0)
      (not (is-letter-char? (string-ref s (- i 1))))))

(define (is-word-boundary-after? s i)
  (or (= i (- (string-length s) 1))
      (not (is-letter-char? (string-ref s (+ i 1))))))

(define (match-differential s i)
  (and (< i (- (string-length s) 1))
       (char=? (string-ref s i) #\d)
       (char=? (string-ref s (+ i 1)) #\*)
       (let ((rest (substring s (+ i 2) (string-length s))))
         (cond ((or (string-starts? rest "x")
                    (string-starts? rest "y")
                    (string-starts? rest "z")
                    (string-starts? rest "r"))
                (cons 1 (substring rest 0 1)))
               ((string-starts? rest "<rho>")
                (cons 5 "<rho>"))
               ((string-starts? rest "<varrho>")
                (cons 8 "<varrho>"))
               ((string-starts? rest "<theta>")
                (cons 7 "<theta>"))
               ((string-starts? rest "<vartheta>")
                (cons 10 "<vartheta>"))
               (else #f)))))

(define (transform-math-string s)
  (let* ((n (string-length s))
         (res '()))
    (let loop ((i 0) (last-idx 0))
      (cond ((>= i n)
             (if (null? res) s
                 (begin
                   (if (< last-idx n)
                       (set! res (append res (list (substring s last-idx n)))))
                   (cons 'concat res))))
            (else
             (let ((match (match-differential s i)))
               (if (and match
                        (is-word-boundary-before? s i)
                        (is-word-boundary-after? s (+ i 1 (car match))))
                   (let* ((match-len (car match))
                          (var (cdr match)))
                     (if (> i last-idx)
                         (set! res (append res (list (substring s last-idx i)))))
                     (set! res (append res (list "d" " " var)))
                     (loop (+ i 2 match-len) (+ i 2 match-len)))
                   (loop (+ i 1) last-idx))))))))

(define (transform-concat-children children)
  (cond ((null? children) '())
        ((and (pair? children) (pair? (cdr children)))
         (let* ((c1 (car children))
                (c2 (cadr children)))
           (if (and (string? c1) (string? c2)
                    (or (string=? c2 "<rho>") (string=? c2 "<varrho>")
                        (string=? c2 "<theta>") (string=? c2 "<vartheta>"))
                    (let ((len (string-length c1)))
                      (and (> len 0)
                           (char=? (string-ref c1 (- len 1)) #\d)
                           (or (= len 1)
                               (not (is-letter-char? (string-ref c1 (- len 2))))))))
               (let* ((len (string-length c1))
                      (prefix (if (> len 1) (substring c1 0 (- len 1)) #f))
                      (spaced-part (if prefix (list prefix "d" " " c2) (list "d" " " c2))))
                 (append spaced-part (transform-concat-children (cddr children))))
               (cons (car children) (transform-concat-children (cdr children))))))
        (else children)))

(define math-environments
  '(math equation equation* eqnarray eqnarray* align align* multline multline*))

(define (upgrade-latex-differentials-stree t in-math)
  (cond ((string? t)
         (if in-math
             (transform-math-string t)
             t))
        ((pair? t)
         (let* ((head (car t))
                (next-in-math (or in-math (memq head math-environments))))
           (if (and next-in-math (eq? head 'concat))
               (let* ((new-children (map (lambda (x) (upgrade-latex-differentials-stree x #t)) (cdr t)))
                      (transformed-children (transform-concat-children new-children)))
                 (cons 'concat transformed-children))
               (cons head (map (lambda (x) (upgrade-latex-differentials-stree x next-in-math)) (cdr t))))))
        (else t)))

(define (has-cwith-property? options row-start row-end col-start col-end property value-pred)
  (cond ((null? options) #f)
        ((and (pair? (car options))
              (eq? (caar options) 'cwith))
         (let* ((opt (car options))
                (r-start (and (> (length opt) 1) (list-ref opt 1)))
                (r-end   (and (> (length opt) 2) (list-ref opt 2)))
                (c-start (and (> (length opt) 3) (list-ref opt 3)))
                (c-end   (and (> (length opt) 4) (list-ref opt 4)))
                (prop    (and (> (length opt) 5) (list-ref opt 5)))
                (val     (and (> (length opt) 6) (list-ref opt 6))))
           (if (and (or (not row-start) (equal? r-start row-start))
                    (or (not row-end) (equal? r-end row-end))
                    (or (not col-start) (equal? c-start col-start))
                    (or (not col-end) (equal? c-end col-end))
                    (or (not property) (equal? prop property))
                    (and val (value-pred val)))
               #t
               (has-cwith-property? (cdr options) row-start row-end col-start col-end property value-pred))))
        (else (has-cwith-property? (cdr options) row-start row-end col-start col-end property value-pred))))

(define (find-table-num-rows children)
  (cond ((null? children) 0)
        ((and (pair? (car children)) (eq? (caar children) 'table))
         (length (cdar children)))
        (else (find-table-num-rows (cdr children)))))

(define (is-three-line-table-tformat? x)
  (if (and (pair? x) (eq? (car x) 'tformat))
      (let* ((options (cdr x))
             (num-rows (find-table-num-rows options)))
        (if (> num-rows 0)
            (let* ((has-top? (has-cwith-property? options "1" "1" #f #f "cell-tborder" (lambda (v) (not (equal? v "0ln")))))
                   (has-bottom? (has-cwith-property? options (number->string num-rows) (number->string num-rows) #f #f "cell-bborder" (lambda (v) (not (equal? v "0ln")))))
                   (has-vertical? (has-cwith-property? options #f #f #f #f "cell-lborder" (lambda (v) (not (equal? v "0ln")))))
                   (has-vertical-r? (has-cwith-property? options #f #f #f #f "cell-rborder" (lambda (v) (not (equal? v "0ln"))))))
              (and has-top? has-bottom? (not has-vertical?) (not has-vertical-r?)))
            #f))
      #f))

(define (transform-three-line-tables x)
  (cond ((null? x) '())
        ((and (pair? x) (eq? (car x) 'tformat))
         (let ((transformed-args (map transform-three-line-tables (cdr x))))
           (let ((new-tformat (cons 'tformat transformed-args)))
             (if (is-three-line-table-tformat? new-tformat)
                 (list 'three-line-table new-tformat)
                 new-tformat))))
        ((pair? x)
         (cons (transform-three-line-tables (car x))
               (transform-three-line-tables (cdr x))))
        (else x)))

(define latex->texmacs-original latex->texmacs)

(tm-define (latex->texmacs t)
  (let* ((res (latex->texmacs-original t))
         (st (tree->stree res))
         (new-st1 (upgrade-latex-differentials-stree st #f))
         (new-st (transform-three-line-tables new-st1)))
    (stree->tree new-st)))

(define latex-document->texmacs-original latex-document->texmacs)

(tm-define (latex-document->texmacs x . opts)
  (let* ((res (apply latex-document->texmacs-original (cons x opts)))
         (st (tree->stree res))
         (new-st1 (upgrade-latex-differentials-stree st #f))
         (new-st (transform-three-line-tables new-st1)))
    (stree->tree new-st)))

;; Re-register converters so that `converter-function` table points
;; to our wrapper definitions.  The `converter` macro resolves the
;; function symbol at registration time; simply redefining the symbol
;; afterwards leaves the old reference in the table.
(converter latex-tree texmacs-tree
  (:function latex->texmacs))
(converter latex-document texmacs-tree
  (:function-with-options latex-document->texmacs)
  (:option "latex->texmacs:fallback-on-pictures" "on")
  (:option "latex->texmacs:source-tracking" "off")
  (:option "latex->texmacs:conservative" "off")
  (:option "latex->texmacs:transparent-source-tracking" "off"))
