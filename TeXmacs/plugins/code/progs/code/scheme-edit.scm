
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : scheme-edit.scm
;; DESCRIPTION : editing scheme programs
;; COPYRIGHT   : (C) 2008  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code scheme-edit)
  (:use (prog prog-edit)
    (prog scheme-tools)
    (prog scheme-autocomplete)
    (utils misc tm-keywords)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Treatment of special characters
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (previous-special s col)
  (cond ((< col 0) col)
        ((in? (string-ref s col) '(#\( #\) #\space #\")) col)
        (else (previous-special s (- col 1)))
  ) ;cond
) ;define

(define (next-special s col)
  (cond ((>= col (string-length s)) col)
        ((in? (string-ref s col) '(#\( #\) #\space #\")) col)
        (else (next-special s (+ col 1)))
  ) ;cond
) ;define

(define (next-word doc row col)
  (and-with par
    (program-row row)
    (and (>= col 0)
      (<= col (string-length par))
      (substring par col (next-special par col))
    ) ;and
  ) ;and-with
) ;define

(define (quoted-backwards s col)
  (cond ((< col 0) col)
        ((== (string-ref s col) #\") (- col 1))
        (else (quoted-backwards s (- col 1)))
  ) ;cond
) ;define

(define (quoted-forwards s col)
  (cond ((>= col (string-length s)) col)
        ((== (string-ref s col) #\") (+ col 1))
        (else (quoted-forwards s (+ col 1)))
  ) ;cond
) ;define

(define (string-uncommented s col)
  (cond ((>= col (string-length s)) col)
        ((== (string-ref s col) #\;) col)
        ((== (string-ref s col) #\")
         (string-uncommented s (quoted-forwards s (+ col 1)))
        ) ;
        (else (string-uncommented s (+ col 1)))
  ) ;cond
) ;define

(define (string-uncommented-length s)
  (string-uncommented s 0)
) ;define

(define (previous-argument doc row col level)
  (and (>= row 0)
    (< row (tree-arity doc))
    (tree-atomic? (tree-ref doc row))
    (with par
      (tree->string (tree-ref doc row))
      (cond ((>= col (string-length par))
             (with len
               (string-uncommented-length par)
               (previous-argument doc row (- len 1) level)
             ) ;with
            ) ;
            ((< col 0) (previous-argument doc (- row 1) 1000000000 level))
            ((== (string-ref par col) #\()
             (cond ((== level 0) #f)
                   ((== level 1) (cons row col))
                   (else (previous-argument doc row (- col 1) (- level 1)))
             ) ;cond
            ) ;
            ((== (string-ref par col) #\))
             (previous-argument doc row (- col 1) (+ level 1))
            ) ;
            ((== (string-ref par col) #\space) (previous-argument doc row (- col 1) level))
            ((== (string-ref par col) #\")
             (with ncol
               (quoted-backwards par (- col 1))
               (if (== level 0) (cons row (+ ncol 1)) (previous-argument doc row ncol level))
             ) ;with
            ) ;
            (else (with ncol
                    (previous-special par (- col 1))
                    (if (== level 0) (cons row (+ ncol 1)) (previous-argument doc row ncol level))
                  ) ;with
            ) ;else
      ) ;cond
    ) ;with
  ) ;and
) ;define

(define (previous-arguments doc row col bound)
  (with arg
    (previous-argument doc row col 0)
    (cond ((not arg) '())
          ((<= bound 0) '((-1 -1)))
          (else (let* ((nrow (car arg))
                       (ncol (- (cdr arg) 1))
                       (nbound (if (== nrow row) bound (- bound 1)))
                      ) ;
                  (cons arg (previous-arguments doc nrow ncol nbound))
                ) ;let*
          ) ;else
    ) ;cond
  ) ;with
) ;define

(define (reference-type doc l)
  (if (null? l) "" (with arg (cAr l) (or (next-word doc (car arg) (cdr arg)) "")))
) ;define

(define (reference-argument l)
  (cond ((null? l) #f)
        ((null? (cdr l)) (car l))
        ((null? (cddr l)) (car l))
        (else (let* ((a1 (car l)) (a2 (cadr l)))
                (if (< (car a2) (car a1)) a1 (reference-argument (cdr l)))
              ) ;let*
        ) ;else
  ) ;cond
) ;define

(define (tm-count l inc)
  (cond ((null? l) 0)
        ((== (car l) #\<) (tm-count (cdr l) 0))
        ((== (car l) #\>) (+ (tm-count (cdr l) 1) 1))
        (else (+ (tm-count (cdr l) inc) inc))
  ) ;cond
) ;define

(define (get-offset doc a)
  (with s
    (tree->string (tree-ref doc (car a)))
    (tm-count (string->list (substring s 0 (cdr a))) 1)
  ) ;with
) ;define
(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-scheme?)
  (let* ((l (previous-arguments doc row col 10))
         (t (reference-type doc l))
         (i (indent-get-arity t))
         (n (length l))
         (a (reference-argument l))
        ) ;
    (cond ((not a) 0)
          ((not i) (get-offset doc a))
          ((<= n i) (+ (get-offset doc (cAr l)) 3))
          ((== n (+ i 1)) (+ (get-offset doc (cAr l)) 1))
          (else (get-offset doc a))
    ) ;cond
  ) ;let*
) ;tm-define
(tm-define (kbd-variant t forwards?)
  (:mode in-prog-scheme?)
  (if (not scheme-completions-built?) (scheme-completions-rebuild))
  (custom-complete (tm->tree (scheme-completions (cursor-word))))
) ;tm-define
(tm-define (scheme-bracket-open lbr rbr) (bracket-open lbr rbr "\\"))
(tm-define (scheme-bracket-close lbr rbr) (bracket-close lbr rbr "\\"))
(tm-define (kbd-select-enlarge)
  (:require prog-select-brackets?)
  (:mode in-prog-scheme?)
  (program-select-enlarge "(" ")")
) ;tm-define
(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-scheme?)
  (select-brackets-after-movement "([{" ")]}" "\\")
) ;tm-define

(define (notify-scheme-syntax var val)
  (syntax-read-preferences "scheme")
) ;define
(define-preferences ("syntax:scheme:none" "red" notify-scheme-syntax)
 ("syntax:scheme:comment" "brown" notify-scheme-syntax)
 ("syntax:scheme:keyword" "#309090" notify-scheme-syntax)
 ("syntax:scheme:error" "dark red" notify-scheme-syntax)
 ("syntax:scheme:constant_number" "#4040c0" notify-scheme-syntax)
 ("syntax:scheme:constant_string" "dark grey" notify-scheme-syntax)
 ("syntax:scheme:constant_char" "#333333" notify-scheme-syntax)
 ("syntax:scheme:variable_identifier" "#204080" notify-scheme-syntax)
 ("syntax:scheme:declare_category" "#d030d0" notify-scheme-syntax)
) ;define-preferences

(define (textual? x)
  (or (tm-atomic? x)
    (and (tm-in? x '(concat document)) (forall? textual? (tm-children x)))
  ) ;or
) ;define
(kbd-map (:mode in-prog-scheme?)
 ("(" (scheme-bracket-open "(" ")"))
 (")" (scheme-bracket-close "(" ")"))
 ("[" (scheme-bracket-open "[" "]"))
 ("]" (scheme-bracket-close "[" "]"))
 ("\"" (scheme-bracket-open "\"" "\""))
) ;kbd-map

(kbd-map (:require (and developer-mode? (in-prog-scheme?)))
 ("A-F1" (scheme-popup-help (cursor-word)))
 ("cmd A-F1" (scheme-inbuffer-help (cursor-word)))
 ("std F1" (scheme-go-to-definition (cursor-word)))
) ;kbd-map

(kbd-map (:require (and developer-mode?
                     (in-prog-scheme?)
                     (== "scheme-file" (file-format (current-buffer-url)))
                   ) ;and
         ) ;:require
 ("std R" (run-scheme-file (current-buffer-url)))
) ;kbd-map
