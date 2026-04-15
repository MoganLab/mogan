;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : fish-edit.scm
;; DESCRIPTION : editing fish programs
;; COPYRIGHT   : (C) 2025   vesita
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code fish-edit)
  (:use (prog prog-edit)
        (code fish-mode)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Indentation policy
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (fish-tabstop) 4)

(tm-define (get-tabstop)
  (:mode in-prog-fish?)
  (fish-tabstop)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helpers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define fish-block-openers
  '("case" "command" "define" "else" "for" "foreach" "if" "loop"
    "section" "struct" "structure" "while")
) ;define

(define fish-block-closers
  '("end" "end_if" "endif" "end_loop" "end_section"
    "endcase" "endcommand" "endloop" "endsection")
) ;define

(define fish-block-middle
  '("case" "else")
) ;define

(define (fish-word-in? w lst)
  (cond
    ((null? lst) #f)
    ((== w (car lst)) #t)
    (else (fish-word-in? w (cdr lst)))
  ) ;cond
) ;define

(define (fish-string-prefix? s p)
  (and (>= (string-length s) (string-length p))
       (== (substring s 0 (string-length p)) p)
  ) ;and
) ;define

(define (fish-trim-left s)
  (let loop ((i 0) (n (string-length s)))
    (if (or (>= i n)
            (not (char-whitespace? (string-ref s i))))
        (substring s i n)
        (loop (+ i 1) n)
    ) ;if
  ) ;let
) ;define

(define (fish-trim-right s)
  (let loop ((i (- (string-length s) 1)))
    (if (< i 0) ""
        (if (char-whitespace? (string-ref s i))
            (loop (- i 1))
            (substring s 0 (+ i 1))
        ) ;if
    ) ;if
  ) ;let
) ;define

(define (fish-trim s)
  (fish-trim-right (fish-trim-left s))
) ;define

(define (fish-word-char? c)
  (or (char-alphabetic? c)
      (char-numeric? c)
      (== c #\_)
  ) ;or
) ;define

(define (fish-first-word s)
  (let* ((t (fish-trim-left s))
         (n (string-length t)))
    (let loop ((i 0))
      (if (or (>= i n)
              (not (fish-word-char? (string-ref t i))))
          (if (<= i 0) "" (substring t 0 i))
          (loop (+ i 1))
      ) ;if
    ) ;let
  ) ;let*
) ;define

(define (fish-line-continues? line)
  (let* ((t (fish-trim-right line))
         (n (string-length t)))
    (and (> n 0)
         (or (and (>= n 3)
                  (== (substring t (- n 3) n) "..."))
             (== (string-ref t (- n 1)) #\&)
         ) ;or
    ) ;and
  ) ;let*
) ;define

(define (fish-line-opens-block? line)
  (let* ((t (fish-trim line))
         (w (fish-first-word t)))
    (or (and (!= w "")
             (or (fish-word-in? w fish-block-openers)
                 (== w "caseof"))
             ) ;or
        (fish-string-prefix? t "if ")
        (== t "if")
        (fish-string-prefix? t "else if ")
        (and (or (fish-string-prefix? t "if ")
                 (fish-string-prefix? t "else if ")
                 (fish-string-prefix? t "caseof "))
             (or (== t "then")
                 (fish-string-prefix? t "then ")
                 (and (>= (string-length t) 4)
                      (== (substring t (- (string-length t) 4)
                                     (string-length t))
                          "then"
                      ) ;==
                 ) ;and
             ) ;or
        ) ;and
    ) ;or
  ) ;let*
) ;define

(define (fish-line-starts-with-outdent? line)
  (let ((w (fish-first-word line)))
    (or (fish-word-in? w fish-block-closers)
        (fish-word-in? w fish-block-middle)
    ) ;or
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Line access
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (fish-get-line row)
  (let ((s (program-row row)))
    (if s s "")
  ) ;let
) ;define

(define (fish-prev-nonempty-row row)
  (let loop ((r (- row 1)))
    (if (< r 0) -1
        (let* ((line (fish-get-line r))
               (t (fish-trim line)))
          (if (== t "") (loop (- r 1)) r)
        ) ;let*
    ) ;if
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Indentation computation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (fish-indent-level-from-prev row)
  (let* ((pr (fish-prev-nonempty-row row)))
    (if (< pr 0) 0
        (let* ((pline (fish-get-line pr))
               (base (string-get-indent pline))
               (tab (get-tabstop))
               (inc? (or (fish-line-continues? pline)
                         (fish-line-opens-block? pline)))
               ) ;inc?
          (+ base (if inc? tab 0))
        ) ;let*
    ) ;if
  ) ;let*
) ;define

(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-fish?)
  (let* ((tab (get-tabstop))
         (line (fish-get-line row))
         (base (fish-indent-level-from-prev row)))
    (if (fish-line-starts-with-outdent? line)
        (max 0 (- base tab))
        base
    ) ;if
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Commenting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (program-comment-start)
  (:mode in-prog-fish?)
  ";"
) ;tm-define

(tm-define (program-toggle-comment)
  (:mode in-prog-fish?)
  (prog-toggle-line-comment ";")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Paste import hook
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-paste)
  (:mode in-prog-fish?)
  (clipboard-paste-import "fish" "primary")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Brackets / quotes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (fish-bracket-open lbr rbr)
  (bracket-open lbr rbr "\\")
) ;tm-define

(tm-define (fish-bracket-close lbr rbr)
  (bracket-close lbr rbr "\\")
) ;tm-define

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-fish?)
  (select-brackets-after-movement "([{" ")]}" "\\")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard mappings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map
  (:mode in-prog-fish?)
  ("A-tab" (insert-tabstop))
  ("cmd S-tab" (remove-tabstop))
  ("{" (fish-bracket-open "{" "}" ))
  ("}" (fish-bracket-close "{" "}" ))
  ("(" (fish-bracket-open "(" ")" ))
  (")" (fish-bracket-close "(" ")" ))
  ("[" (fish-bracket-open "[" "]" ))
  ("]" (fish-bracket-close "[" "]" ))
  ("\"" (fish-bracket-open "\"" "\"" ))
  ("'" (fish-bracket-open "'" "'" ))
) ;kbd-map
