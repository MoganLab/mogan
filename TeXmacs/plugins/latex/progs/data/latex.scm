
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

(define (string-split-lines s)
  (let ((len (if (>= (string-length s) 1000) 1000 (string-length s))))
    (let loop
      ((i 0) (start 0) (result '()))
      (cond ((>= i len) (reverse (cons (substring s start i) result)))
            ((char=? (string-ref s i) #\newline)
             (loop (+ i 1) (+ i 1) (cons (substring s start i) result))
            ) ;
            (else (loop (+ i 1) start result))
      ) ;cond
    ) ;let
  ) ;let
) ;define

(define (backslash-from-string s)
  (if (not (string-null? s))
    (let* ((len (string-length s)) (limit (if (>= len 1000) 1000 len)))
      (let loop
        ((ref 0) (count 0))
        (if (>= ref limit)
          (/ count len)
          (loop (+ ref 1) (if (char=? (string-ref s ref) #\\) (+ count 1) count))
        ) ;if
      ) ;let
    ) ;let*
    #f
  ) ;if
) ;define

(define (backslash-line-from-string s)
  (let ((lines (string-split-lines s)))
    (if (null? lines)
      0
      (let loop
        ((count-lines 0) (count 0) (remaining-lines lines))
        (if (null? remaining-lines)
          (if (> count-lines 0) (/ count count-lines) 0)
          (let ((line (car remaining-lines)))
            (loop (+ count-lines 1)
              (if (string-contains? line "\\") (+ count 1) count)
              (cdr remaining-lines)
            ) ;loop
          ) ;let
        ) ;if
      ) ;let
    ) ;if
  ) ;let
) ;define

(define (parentheses-from-string s)
  (if (not (string-null? s))
    (let* ((len (string-length s)) (limit (if (>= len 1000) 1000 len)))
      (let loop
        ((ref 0) (count 0))
        (if (>= ref limit)
          (/ count len)
          (loop (+ ref 1)
            (if (or (char=? (string-ref s ref) (string-ref "{" 0))
                  (char=? (string-ref s ref) (string-ref "}" 0))
                ) ;or
              (+ count 1)
              count
            ) ;if
          ) ;loop
        ) ;if
      ) ;let
    ) ;let*
    #f
  ) ;if
) ;define

(define (determine-short-string s)
  (let* ((len (string-length s)))
    (cond ((and (> len 2)
             (char=? (string-ref s 0) #\$)
             (char=? (string-ref s (- len 1)) #\$)
           ) ;and
           #t
          ) ;
          ((>= (backslash-from-string s) 0.02) #t)
          (else #f)
    ) ;cond
  ) ;let*
) ;define

(define (is-short-latex-string? s)
  (if (<= (string-length s) 50) (determine-short-string s) #f)
) ;define

(define (is-latex-string? s)
  (let ((percent-slash (backslash-from-string s)))
    (if (and (>= percent-slash 0.01) (<= percent-slash 0.25))
      (let ((percent-parentheses (parentheses-from-string s)))
        (if (>= percent-parentheses 0.01)
          (let ((percent-backslash-line (backslash-line-from-string s)))
            (if (>= percent-backslash-line 0.25) #t #f)
          ) ;let
          #f
        ) ;if
      ) ;let
      #f
    ) ;if
  ) ;let
) ;define

(define (latex-recognizes-at? s pos)
  (set! pos (format-skip-spaces s pos))
  (cond ((format-test? s pos "\\document") #t)
        ((format-test? s pos "\\documentclass") #t)
        ((format-test? s pos "\\usepackage") #t)
        ((format-test? s pos "\\title") #t)
        ((format-test? s pos "\\newcommand") #t)
        ((format-test? s pos "\\input") #t)
        ((format-test? s pos "\\includeonly") #t)
        ((format-test? s pos "\\chapter") #t)
        ((format-test? s pos "\\appendix") #t)
        ((format-test? s pos "\\section") #t)
        ((format-test? s pos "\\footnote") #t)
        ((format-test? s pos "\\marginpar") #t)
        ((format-test? s pos "\\begin") #t)
        ((format-test? s pos "\\end") #t)
        ((format-test? s pos "\\begin{") #t)
        ((format-test? s pos "\\end{") #t)
        ((format-test? s pos "\\alpha") #t)
        ((format-test? s pos "\\beta") #t)
        ((format-test? s pos "\\gamma") #t)
        ((format-test? s pos "\\ref") #t)
        ((format-test? s pos "\\textbf") #t)
        ((format-test? s pos "\\textit") #t)
        ((format-test? s pos "\\mathbb") #t)
        ((format-test? s pos "\\mathcal") #t)
        ((format-test? s pos "\\frac") #t)
        ((format-test? s pos "\\cite") #t)
        ((format-test? s pos "\\item") #t)
        ((format-test? s pos "\\[") #t)
        ((format-test? s pos "\\(") #t)
        ((is-short-latex-string? s) #t)
        ((is-latex-string? s) #t)
        (else #f)
  ) ;cond
) ;define

(define (latex-recognizes? s)
  (and (string? s) (latex-recognizes-at? s 0))
) ;define

(define-format latex
  (:name "LaTeX")
  (:suffix "tex")
  (:recognize latex-recognizes?)
) ;define-format

(define-format latex-class (:name "LaTeX class") (:suffix "ltx" "sty" "cls"))

(define-preferences ("texmacs->latex:transparent-tracking" "on" noop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; TeXmacs->LaTeX
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(lazy-define (convert latex texout) serialize-latex)
(lazy-define (convert latex tmtex) texmacs->latex)

(converter texmacs-stree
  latex-stree
  (:function-with-options texmacs->latex)
  (:option "texmacs->latex:source-tracking" "off")
  (:option "texmacs->latex:conservative" "on")
  (:option "texmacs->latex:transparent-source-tracking" "on")
  (:option "texmacs->latex:attach-tracking-info" "on")
  (:option "texmacs->latex:replace-style" "on")
  (:option "texmacs->latex:expand-macros" "on")
  (:option "texmacs->latex:expand-user-macros" "off")
  (:option "texmacs->latex:indirect-bib" "off")
  (:option "texmacs->latex:use-macros" "off")
  (:option "texmacs->latex:encoding" "UTF-8")
) ;converter

(converter latex-stree latex-document (:function serialize-latex))

(converter latex-stree latex-snippet (:function serialize-latex))

(tm-define (texmacs->latex-document x opts)
  (serialize-latex (texmacs->latex (tm->stree x) opts))
) ;tm-define

(converter texmacs-stree
  latex-document
  (:function-with-options conservative-texmacs->latex)
  ;; (:function-with-options tracked-texmacs->latex)
  (:option "texmacs->latex:source-tracking" "off")
  (:option "texmacs->latex:conservative" "on")
  (:option "texmacs->latex:transparent-source-tracking" "on")
  (:option "texmacs->latex:attach-tracking-info" "on")
  (:option "texmacs->latex:replace-style" "on")
  (:option "texmacs->latex:expand-macros" "on")
  (:option "texmacs->latex:expand-user-macros" "off")
  (:option "texmacs->latex:indirect-bib" "off")
  (:option "texmacs->latex:use-macros" "on")
  (:option "texmacs->latex:encoding" "UTF-8")
) ;converter

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; LaTeX -> TeXmacs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (latex-document->texmacs x . opts)
  (if (list-1? opts) (set! opts (car opts)))
  (with as-pic
    (== (get-preference "latex->texmacs:fallback-on-pictures") "on")
    (conservative-latex->texmacs x as-pic)
  ) ;with
) ;tm-define

(converter latex-document latex-tree (:function parse-latex-document))

(converter latex-snippet latex-tree (:function parse-latex))

(converter latex-class-document
  texmacs-tree
  (:function latex-class-document->texmacs)
) ;converter

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Post-processing imported LaTeX: insert space between d and differential
;; variables so they are not merged into a single operator in math mode.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (is-letter-char? c)
  (and (char? c)
    (or (and (char>=? c #\a) (char<=? c #\z)) (and (char>=? c #\A) (char<=? c #\Z)))
  ) ;and
) ;define

(define (is-word-boundary-before? s i)
  (or (= i 0) (not (is-letter-char? (string-ref s (- i 1)))))
) ;define

(define (is-word-boundary-after? s i)
  (or (= i (- (string-length s) 1))
    (not (is-letter-char? (string-ref s (+ i 1))))
  ) ;or
) ;define

(define (match-differential s i)
  (and (< i (- (string-length s) 1))
    (char=? (string-ref s i) #\d)
    (char=? (string-ref s (+ i 1)) #\*)
    (let ((rest (substring s (+ i 2) (string-length s))))
      (cond ((or (string-starts? rest "x")
               (string-starts? rest "y")
               (string-starts? rest "z")
               (string-starts? rest "r")
               (string-starts? rest "t")
               (string-starts? rest "u")
               (string-starts? rest "v")
               (string-starts? rest "w")
             ) ;or
             (cons 1 (substring rest 0 1))
            ) ;
            ((string-starts? rest "<rho>") (cons 5 "<rho>"))
            ((string-starts? rest "<varrho>") (cons 8 "<varrho>"))
            ((string-starts? rest "<theta>") (cons 7 "<theta>"))
            ((string-starts? rest "<vartheta>") (cons 10 "<vartheta>"))
            ((string-starts? rest "<tau>") (cons 5 "<tau>"))
            ((string-starts? rest "<upsilon>") (cons 9 "<upsilon>"))
            ((string-starts? rest "<phi>") (cons 5 "<phi>"))
            ((string-starts? rest "<varphi>") (cons 8 "<varphi>"))
            ((string-starts? rest "<omega>") (cons 7 "<omega>"))
            (else #f)
      ) ;cond
    ) ;let
  ) ;and
) ;define

(define (transform-math-string s)
  (let* ((n (string-length s)) (res '()))
    (let loop
      ((i 0) (last-idx 0))
      (cond ((>= i n)
             (if (null? res)
               s
               (begin
                 (if (< last-idx n) (set! res (append res (list (substring s last-idx n)))))
                 (cons 'concat res)
               ) ;begin
             ) ;if
            ) ;
            (else (let ((match (match-differential s i)))
                    (if (and match
                          (is-word-boundary-before? s i)
                          (is-word-boundary-after? s (+ i 1 (car match)))
                        ) ;and
                      (let* ((match-len (car match)) (var (cdr match)))
                        (if (> i last-idx) (set! res (append res (list (substring s last-idx i)))))
                        (set! res (append res (list "d" " " var)))
                        (loop (+ i 2 match-len) (+ i 2 match-len))
                      ) ;let*
                      (loop (+ i 1) last-idx)
                    ) ;if
                  ) ;let
            ) ;else
      ) ;cond
    ) ;let
  ) ;let*
) ;define

(define (transform-concat-children children)
  (cond ((null? children) '())
        ((and (pair? children) (pair? (cdr children)))
         (let* ((c1 (car children)) (c2 (cadr children)))
           (if (and (string? c1)
                 (string? c2)
                 (or (string=? c2 "<rho>")
                   (string=? c2 "<varrho>")
                   (string=? c2 "<theta>")
                   (string=? c2 "<vartheta>")
                   (string=? c2 "<tau>")
                   (string=? c2 "<upsilon>")
                   (string=? c2 "<phi>")
                   (string=? c2 "<varphi>")
                   (string=? c2 "<omega>")
                 ) ;or
                 (let ((len (string-length c1)))
                   (and (> len 0)
                     (char=? (string-ref c1 (- len 1)) #\d)
                     (or (= len 1) (not (is-letter-char? (string-ref c1 (- len 2)))))
                   ) ;and
                 ) ;let
               ) ;and
             (let* ((len (string-length c1))
                    (prefix (if (> len 1) (substring c1 0 (- len 1)) #f))
                    (spaced-part (if prefix (list prefix "d" " " c2) (list "d" " " c2)))
                   ) ;
               (append spaced-part (transform-concat-children (cddr children)))
             ) ;let*
             (cons (car children) (transform-concat-children (cdr children)))
           ) ;if
         ) ;let*
        ) ;
        (else children)
  ) ;cond
) ;define

(define math-environments
  '(math equation equation* eqnarray eqnarray* align align* multline multline*)
) ;define

(define (upgrade-latex-differentials-stree t in-math)
  (cond ((string? t) (if in-math (transform-math-string t) t))
        ((pair? t)
         (let* ((head (car t)) (next-in-math (or in-math (memq head math-environments))))
           (if (and next-in-math (eq? head 'concat))
             (let* ((new-children (map (lambda (x) (upgrade-latex-differentials-stree x #t)) (cdr t))
                    ) ;new-children
                    (transformed-children (transform-concat-children new-children))
                   ) ;
               (cons 'concat transformed-children)
             ) ;let*
             (cons head
               (map (lambda (x) (upgrade-latex-differentials-stree x next-in-math)) (cdr t))
             ) ;cons
           ) ;if
         ) ;let*
        ) ;
        (else t)
  ) ;cond
) ;define

(define (has-cwith-property? options
          row-start
          row-end
          col-start
          col-end
          property
          value-pred
        ) ;has-cwith-property?
  (cond ((null? options) #f)
        ((and (pair? (car options)) (eq? (caar options) 'cwith))
         (let* ((opt (car options))
                (r-start (and (> (length opt) 1) (list-ref opt 1)))
                (r-end (and (> (length opt) 2) (list-ref opt 2)))
                (c-start (and (> (length opt) 3) (list-ref opt 3)))
                (c-end (and (> (length opt) 4) (list-ref opt 4)))
                (prop (and (> (length opt) 5) (list-ref opt 5)))
                (val (and (> (length opt) 6) (list-ref opt 6)))
               ) ;
           (if (and (or (not row-start) (equal? r-start row-start))
                 (or (not row-end) (equal? r-end row-end))
                 (or (not col-start) (equal? c-start col-start))
                 (or (not col-end) (equal? c-end col-end))
                 (or (not property) (equal? prop property))
                 (and val (value-pred val))
               ) ;and
             #t
             (has-cwith-property? (cdr options)
               row-start
               row-end
               col-start
               col-end
               property
               value-pred
             ) ;has-cwith-property?
           ) ;if
         ) ;let*
        ) ;
        (else (has-cwith-property? (cdr options)
                row-start
                row-end
                col-start
                col-end
                property
                value-pred
              ) ;has-cwith-property?
        ) ;else
  ) ;cond
) ;define

(define (find-table-num-rows children)
  (cond ((null? children) 0)
        ((and (pair? (car children)) (eq? (caar children) 'table))
         (length (cdar children))
        ) ;
        (else (find-table-num-rows (cdr children)))
  ) ;cond
) ;define

(define (is-three-line-table-tformat? x)
  (if (and (pair? x) (eq? (car x) 'tformat))
    (let* ((options (cdr x)) (num-rows (find-table-num-rows options)))
      (if (> num-rows 0)
        (let* ((has-top? (has-cwith-property? options
                           "1"
                           "1"
                           #f
                           #f
                           "cell-tborder"
                           (lambda (v) (not (equal? v "0ln")))
                         ) ;has-cwith-property?
               ) ;has-top?
               (has-bottom? (has-cwith-property? options
                              (number->string num-rows)
                              (number->string num-rows)
                              #f
                              #f
                              "cell-bborder"
                              (lambda (v) (not (equal? v "0ln")))
                            ) ;has-cwith-property?
               ) ;has-bottom?
               (has-vertical? (has-cwith-property? options
                                #f
                                #f
                                #f
                                #f
                                "cell-lborder"
                                (lambda (v) (not (equal? v "0ln")))
                              ) ;has-cwith-property?
               ) ;has-vertical?
               (has-vertical-r? (has-cwith-property? options
                                  #f
                                  #f
                                  #f
                                  #f
                                  "cell-rborder"
                                  (lambda (v) (not (equal? v "0ln")))
                                ) ;has-cwith-property?
               ) ;has-vertical-r?
              ) ;
          (and has-top? has-bottom? (not has-vertical?) (not has-vertical-r?))
        ) ;let*
        #f
      ) ;if
    ) ;let*
    #f
  ) ;if
) ;define

(define (transform-three-line-tables x)
  (cond ((null? x) '())
        ((and (pair? x) (eq? (car x) 'tformat))
         (let ((transformed-args (map transform-three-line-tables (cdr x))))
           (let ((new-tformat (cons 'tformat transformed-args)))
             (if (is-three-line-table-tformat? new-tformat)
               (list 'three-line-table new-tformat)
               new-tformat
             ) ;if
           ) ;let
         ) ;let
        ) ;
        ((pair? x)
         (cons (transform-three-line-tables (car x))
           (transform-three-line-tables (cdr x))
         ) ;cons
        ) ;
        (else x)
  ) ;cond
) ;define

(define (clean-multirow t)
  (cond ((null? t) (cons '() #f))
        ((and (pair? t) (eq? (car t) 'multirow))
         (let ((n (list-ref t 1))
               (w (list-ref t 2))
               (text (if (> (length t) 3) (list-ref t 3) ""))
              ) ;
           (cons text (cons n w))
         ) ;let
        ) ;
        ((pair? t)
         (let* ((res-car (clean-multirow (car t))) (res-cdr (clean-multirow (cdr t))))
           (cond ((cdr res-car) (cons (cons (car res-car) (car res-cdr)) (cdr res-car)))
                 ((cdr res-cdr) (cons (cons (car res-car) (car res-cdr)) (cdr res-cdr)))
                 (else (cons (cons (car res-car) (car res-cdr)) #f))
           ) ;cond
         ) ;let*
        ) ;
        (else (cons t #f))
  ) ;cond
) ;define

(define (process-row-cells cells r c options-acc new-cells-acc)
  (cond ((null? cells) (cons (reverse new-cells-acc) options-acc))
        (else (let* ((cell (car cells))
                     (cleaned-res (clean-multirow cell))
                     (new-cell (car cleaned-res))
                     (info (cdr cleaned-res))
                    ) ;
                (if info
                  (let* ((n (car info))
                         (row-str (number->string r))
                         (col-str (number->string c))
                         (new-opt1 (list 'cwith row-str row-str col-str col-str "cell-row-span" n))
                         (new-opt2 (list 'cwith row-str row-str col-str col-str "cell-valign" "c"))
                        ) ;
                    (process-row-cells (cdr cells)
                      r
                      (+ c 1)
                      (cons new-opt1 (cons new-opt2 options-acc))
                      (cons new-cell new-cells-acc)
                    ) ;process-row-cells
                  ) ;let*
                  (process-row-cells (cdr cells) r (+ c 1) options-acc (cons cell new-cells-acc))
                ) ;if
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (process-table-rows rows r options-acc new-rows-acc)
  (cond ((null? rows) (cons (reverse new-rows-acc) options-acc))
        (else (let* ((row (car rows))
                     (cells (cdr row))
                     (res-cells (process-row-cells cells r 1 '() '()))
                    ) ;
                (process-table-rows (cdr rows)
                  (+ r 1)
                  (append options-acc (cdr res-cells))
                  (cons (cons 'row (car res-cells)) new-rows-acc)
                ) ;process-table-rows
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (filter-table options)
  (cond ((null? options) '())
        ((and (pair? (car options)) (eq? (caar options) 'table))
         (filter-table (cdr options))
        ) ;
        (else (cons (car options) (filter-table (cdr options))))
  ) ;cond
) ;define

(define (collect-all-regions options num-rows)
  (let loop-r
    ((r 1) (regions '()))
    (if (> r num-rows)
      regions
      (let* ((r-str (number->string r))
             (row-regions (let loop-c
                            ((c 1) (c-acc '()))
                            (if (> c 50)
                              c-acc
                              (let* ((c-str (number->string c))
                                     (h-val (let loop-opt
                                              ((lst options))
                                              (cond ((null? lst) #f)
                                                    ((and (pair? (car lst))
                                                       (eq? (caar lst) 'cwith)
                                                       (equal? (list-ref (car lst) 1) r-str)
                                                       (equal? (list-ref (car lst) 3) c-str)
                                                       (equal? (list-ref (car lst) 5) "cell-row-span")
                                                     ) ;and
                                                     (list-ref (car lst) 6)
                                                    ) ;
                                                    (else (loop-opt (cdr lst)))
                                              ) ;cond
                                            ) ;let
                                     ) ;h-val
                                     (w-val (let loop-opt
                                              ((lst options))
                                              (cond ((null? lst) #f)
                                                    ((and (pair? (car lst))
                                                       (eq? (caar lst) 'cwith)
                                                       (equal? (list-ref (car lst) 1) r-str)
                                                       (equal? (list-ref (car lst) 3) c-str)
                                                       (equal? (list-ref (car lst) 5) "cell-col-span")
                                                     ) ;and
                                                     (list-ref (car lst) 6)
                                                    ) ;
                                                    (else (loop-opt (cdr lst)))
                                              ) ;cond
                                            ) ;let
                                     ) ;w-val
                                     (h (if h-val (string->number h-val) 1))
                                     (w (if w-val (string->number w-val) 1))
                                    ) ;
                                (if (or (> h 1) (> w 1))
                                  (loop-c (+ c 1) (cons (list r c h w) c-acc))
                                  (loop-c (+ c 1) c-acc)
                                ) ;if
                              ) ;let*
                            ) ;if
                          ) ;let
             ) ;row-regions
            ) ;
        (loop-r (+ r 1) (append regions row-regions))
      ) ;let*
    ) ;if
  ) ;let
) ;define

(define (is-cell-covered? ri ci regions)
  (cond ((null? regions) #f)
        (else (let* ((reg (car regions))
                     (r (list-ref reg 0))
                     (c (list-ref reg 1))
                     (h (list-ref reg 2))
                     (w (list-ref reg 3))
                    ) ;
                (if (and (>= ri r)
                      (< ri (+ r h))
                      (>= ci c)
                      (< ci (+ c w))
                      (not (and (= ri r) (= ci c)))
                    ) ;and
                  #t
                  (is-cell-covered? ri ci (cdr regions))
                ) ;if
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (clean-covered-cells-in-row cells r c regions new-cells-acc)
  (cond ((null? cells) (reverse new-cells-acc))
        (else (let* ((cell (car cells))
                     (new-cell (if (is-cell-covered? r c regions) '(cell "") cell))
                    ) ;
                (clean-covered-cells-in-row (cdr cells)
                  r
                  (+ c 1)
                  regions
                  (cons new-cell new-cells-acc)
                ) ;clean-covered-cells-in-row
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (clean-covered-cells-in-rows rows r regions new-rows-acc)
  (cond ((null? rows) (reverse new-rows-acc))
        (else (let* ((row (car rows))
                     (cells (cdr row))
                     (new-cells (clean-covered-cells-in-row cells r 1 regions '()))
                     (new-row (cons 'row new-cells))
                    ) ;
                (clean-covered-cells-in-rows (cdr rows)
                  (+ r 1)
                  regions
                  (cons new-row new-rows-acc)
                ) ;clean-covered-cells-in-rows
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (transform-multirow-tformat x)
  (if (and (pair? x) (eq? (car x) 'tformat))
    (let* ((options (cdr x))
           (table-cell-pair (let loop
                              ((lst options))
                              (cond ((null? lst) #f)
                                    ((and (pair? (car lst)) (eq? (caar lst) 'table)) (car lst))
                                    (else (loop (cdr lst)))
                              ) ;cond
                            ) ;let
           ) ;table-cell-pair
          ) ;
      (if table-cell-pair
        (let* ((table-rows (cdr table-cell-pair))
               (processed (process-table-rows table-rows 1 '() '()))
               (new-rows-temp (car processed))
               (new-options (cdr processed))
               (all-options (append (filter-table options) new-options))
               (num-rows (length new-rows-temp))
               (regions (collect-all-regions all-options num-rows))
               (new-rows (clean-covered-cells-in-rows new-rows-temp 1 regions '()))
               (rebuilt-options (append all-options (list (cons 'table new-rows))))
              ) ;
          (cons 'tformat rebuilt-options)
        ) ;let*
        x
      ) ;if
    ) ;let*
    x
  ) ;if
) ;define

(define (transform-multirow x)
  (cond ((null? x) '())
        ((and (pair? x) (eq? (car x) 'tformat))
         (let* ((new-t (transform-multirow-tformat x))
                (transformed-args (map transform-multirow (cdr new-t)))
               ) ;
           (cons 'tformat transformed-args)
         ) ;let*
        ) ;
        ((pair? x) (cons (transform-multirow (car x)) (transform-multirow (cdr x))))
        (else x)
  ) ;cond
) ;define

(define latex->texmacs-original latex->texmacs)

(tm-define (latex->texmacs t)
  (let* ((res (latex->texmacs-original t))
         (st (tree->stree res))
         (new-st1 (upgrade-latex-differentials-stree st #f))
         (new-st2 (transform-three-line-tables new-st1))
         (new-st (transform-multirow new-st2))
        ) ;
    (stree->tree new-st)
  ) ;let*
) ;tm-define

(define latex-document->texmacs-original latex-document->texmacs)

(tm-define (latex-document->texmacs x . opts)
  (let* ((res (apply latex-document->texmacs-original (cons x opts)))
         (st (tree->stree res))
         (new-st1 (upgrade-latex-differentials-stree st #f))
         (new-st2 (transform-three-line-tables new-st1))
         (new-st (transform-multirow new-st2))
        ) ;
    (stree->tree new-st)
  ) ;let*
) ;tm-define

(converter latex-tree texmacs-tree (:function latex->texmacs))
(converter latex-document
  texmacs-tree
  (:function-with-options latex-document->texmacs)
  (:option "latex->texmacs:fallback-on-pictures" "on")
  (:option "latex->texmacs:source-tracking" "off")
  (:option "latex->texmacs:conservative" "off")
  (:option "latex->texmacs:transparent-source-tracking" "off")
) ;converter
