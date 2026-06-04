
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
    (let loop ((i 0)
              (start 0)
              (result '()))
      (cond ((>= i len)
             (reverse (cons (substring s start i) result)))
            ((char=? (string-ref s i) #\newline)
             (loop (+ i 1)
                   (+ i 1)
                   (cons (substring s start i) result)))
            (else (loop (+ i 1) start result))))))

(define (backslash-from-string s)
  (if (not (string-null? s)) 
      (let* ((len (string-length s))
            (limit (if (>= len 1000) 1000 len)))
        (let loop ((ref 0)
                  (count 0))
          (if (>= ref limit)
            (/ count len)
            (loop (+ ref 1)
                  (if (char=? (string-ref s ref) #\\)
                      (+ count 1)
                      count)))))
      #f))

(define (backslash-line-from-string s)
  (let ((lines (string-split-lines s)))
    (if (null? lines)
      0
      (let loop ((count-lines 0)
                 (count 0)
                 (remaining-lines lines))
        (if (null? remaining-lines)
            (if (> count-lines 0)
                (/ count count-lines)
                0)
            (let ((line (car remaining-lines))) 
              (loop (+ count-lines 1)
                (if (string-contains? line "\\")
                    (+ count 1)
                    count)
                (cdr remaining-lines))))))))

(define (parentheses-from-string s)
  (if (not (string-null? s)) 
      (let* ((len (string-length s))
            (limit (if (>= len 1000) 1000 len)))
        (let loop ((ref 0)
                  (count 0))
          (if (>= ref limit)
            (/ count len)
            (loop (+ ref 1)
                  (if (or 
                        (char=? (string-ref s ref) (string-ref "{" 0)) 
                        (char=? (string-ref s ref) (string-ref "}" 0)))
                      (+ count 1)
                      count)))))
      #f))

(define (determine-short-string s)
  (let* ((len (string-length s)))
    (cond ((and (> len 2)
                (char=? (string-ref s 0) #\$) 
                (char=? (string-ref s (- len 1)) #\$)) 
          #t)
          ((>= (backslash-from-string s) 0.02) #t)
          (else #f))))

(define (is-short-latex-string? s)
  (if (<= (string-length s) 50)
      (determine-short-string s)
      #f))

(define (is-latex-string? s)
  (let ((percent-slash (backslash-from-string s)))
    (if (and (>= percent-slash 0.01) 
             (<= percent-slash 0.25))
        (let ((percent-parentheses (parentheses-from-string s)))
          (if (>= percent-parentheses 0.01)
              (let ((percent-backslash-line (backslash-line-from-string s)))
                (if (>= percent-backslash-line 0.25)
                    #t
                    #f))
              #f))
        #f)))

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
  (:option "texmacs->latex:use-macros" "off")
  (:option "texmacs->latex:encoding" "UTF-8"))

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
  (:option "texmacs->latex:encoding" "UTF-8"))

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
