;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : sql-edit.scm
;; DESCRIPTION : Editing SQL programs
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code sql-edit)
  (:use (prog prog-edit)
        (code sql-mode)))

(import (liii string))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Indentation settings
;;
;; SQL typically uses 2 or 4 spaces for indentation.
;; Common SQL style guides recommend 2 spaces for compactness.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (sql-tabstop) 2)

(tm-define (get-tabstop)
  (:mode in-prog-sql?)
  (sql-tabstop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Basic indentation heuristics for SQL
;;
;; SQL indentation patterns:
;; - Increase indent after SELECT (for multi-column lists), FROM (for subqueries)
;; - Increase indent after JOIN, WHERE, GROUP BY, HAVING, ORDER BY
;; - Increase indent after opening parenthesis '('
;; - Decrease indent before closing parenthesis ')'
;; - Decrease indent before keywords like END (for CASE statements)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define sql-increase-indent-keys
  '("SELECT" "FROM" "WHERE" "JOIN" "INNER" "LEFT" "RIGHT" "FULL" "OUTER"
    "GROUP" "HAVING" "ORDER" "WITH" "CASE" "WHEN" "AND" "OR"
    "select" "from" "where" "join" "inner" "left" "right" "full" "outer"
    "group" "having" "order" "with" "case" "when" "and" "or"))

(define sql-decrease-indent-keys
  '("END" "ELSE" "ELSEIF"
    "end" "else" "elseif"))

(define sql-end-keys
  '(")" "END"
        "end"))

;; Trim whitespace from left side of string
(define (string-strip-left s)
  (with char-set:not-whitespace (char-set-complement char-set:whitespace)
    (with n (string-length s)
      (with r (or (string-index s char-set:not-whitespace) 0)
        ; (string-take s (min n (+ 1 r)))
        (substring s r n)))))

;; Trim whitespace from right side of string
(define (string-strip-right s)
  (with char-set:not-whitespace (char-set-complement char-set:whitespace)
    (with n (string-length s)
      (with r (or (string-rindex s char-set:not-whitespace) n)
        (string-take s (min n (+ 1 r)))))))

(define (string-strip s) (string-strip-right (string-strip-left s)))

;; Helper function to check if string ends with a keyword
(define (ends-with-keyword? s keys)
  (and (not (null? keys))
       (or (string-ends? s (car keys))
           (ends-with-keyword? s (cdr keys)))))

;; Helper function to check if string starts with a keyword
(define (starts-with-keyword? s keys)
  (and (not (null? keys))
       (or (string-starts? s (string-append (car keys) " "))
           (starts-with-keyword? s (cdr keys)))))

;; Get indentation level of a line (number of leading spaces)
(define (string-get-indent s)
  (let loop ((i 0) (n (string-length s)))
    (if (or (>= i n) (not (char-whitespace? (string-ref s i))))
        i
        (loop (+ i 1) n))))

;; Compute indentation for SQL code
(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-sql?)
  (if (<= row 0) 0
      (let* ((prev-row (- row 1))
             (prev-line (program-row prev-row))
             (stripped-prev (string-strip-right (if prev-line prev-line "")))
             (prev-indent (string-get-indent stripped-prev))
             (tab-width (get-tabstop))
             (current-line (program-row row))
             (stripped-current (string-strip-left (if current-line current-line ""))))

        ;; Check for parentheses
        (let ((open-parens (string-count stripped-prev #\())
              (close-parens (string-count stripped-prev #\))))
          (cond
            ;; If previous line has more open than close parens, increase indent
            ((> open-parens close-parens)
             (+ prev-indent tab-width))

            ;; If previous line ends with increase-indent keyword
            ((ends-with-keyword? stripped-prev sql-increase-indent-keys)
             (+ prev-indent tab-width))

            ;; If current line starts with decrease-indent keyword
            ((starts-with-keyword? stripped-current sql-decrease-indent-keys)
             (max 0 (- prev-indent tab-width)))

            ;; If current line starts with closing paren or END keyword, decrease indent
            ((or (and stripped-current (string-starts? stripped-current ")"))
                 (starts-with-keyword? stripped-current sql-end-keys))
             (max 0 (- prev-indent tab-width)))

            ;; Otherwise maintain previous indent
            (else prev-indent))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Bracket and quote handling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (sql-bracket-open lbr rbr)
  ;; Insert a pair of brackets/quotes and position cursor between them
  (bracket-open lbr rbr "\\"))

(tm-define (sql-bracket-close lbr rbr)
  ;; Handle closing bracket/quote and position cursor appropriately
  (bracket-close lbr rbr "\\"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Comment toggling
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (program-comment-toggle)
  (:mode in-prog-sql?)
  ;; SQL uses -- for single-line comments
  (program-comment-toggle-line "--"))