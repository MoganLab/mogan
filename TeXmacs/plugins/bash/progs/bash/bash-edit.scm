;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : bash-edit.scm
;; DESCRIPTION : editing bash scripts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (bash bash-edit) (:use (prog prog-edit) (bash bash-mode)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Indentation policy
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (bash-tabstop) 2)

(tm-define (get-tabstop) (:mode in-prog-bash?) (bash-tabstop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helpers (lightweight, line-based)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bash-string-prefix? s p)
  (and (>= (string-length s) (string-length p))
    (== (substring s 0 (string-length p)) p)
  ) ;and
) ;define

(define (bash-trim-left s)
  (let loop
    ((i 0) (n (string-length s)))
    (if (or (>= i n) (not (char-whitespace? (string-ref s i))))
      (substring s i n)
      (loop (+ i 1) n)
    ) ;if
  ) ;let
) ;define

(define (bash-trim-right s)
  (let loop
    ((i (- (string-length s) 1)))
    (if (< i 0)
      ""
      (if (char-whitespace? (string-ref s i)) (loop (- i 1)) (substring s 0 (+ i 1)))
    ) ;if
  ) ;let
) ;define

(define (bash-trim s)
  (bash-trim-right (bash-trim-left s))
) ;define

;; Continuation heuristic: \, pipes, &&, ||, redirects, etc.

(define bash-continuation-ops
  '("|" "||" "&&" "&" ">" ">>" "<" "<<" "2>" "2>>" "1>" "1>>")
) ;define

(define bash-block-keywords
  '("if" "for" "while" "until" "case" "select" "function")
) ;define

(define (bash-line-continues? line)
  (let* ((t (bash-trim-right line)) (n (string-length t)))
    (if (<= n 0)
      #f
      (or (== (string-ref t (- n 1)) #\\)
        (let loop
          ((xs bash-continuation-ops))
          (if (null? xs)
            #f
            (let* ((op (car xs)) (m (string-length op)))
              (if (and (>= n m) (== (substring t (- n m) n) op)) #t (loop (cdr xs)))
            ) ;let*
          ) ;if
        ) ;let
      ) ;or
    ) ;if
  ) ;let*
) ;define

(define (bash-line-starts-with-closing-brace? line)
  (let ((t (bash-trim-left line)))
    (and (> (string-length t) 0) (== (string-ref t 0) #\}))
  ) ;let
) ;define

;; NOTE: this is still a heuristic (prefix-based). Consider word-boundary matching later.

(define (bash-line-starts-with-fi-done-esac? line)
  (let ((t (bash-trim-left line)))
    (or (bash-string-prefix? t "fi")
      (bash-string-prefix? t "done")
      (bash-string-prefix? t "esac")
      (bash-string-prefix? t "elif")
      (bash-string-prefix? t "else")
    ) ;or
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Line access
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bash-get-line row)
  (let ((s (program-row row)))
    (if s s "")
  ) ;let
) ;define

(define (bash-prev-nonempty-row row)
  (let loop
    ((r (- row 1)))
    (if (< r 0)
      -1
      (let* ((line (bash-get-line r)) (t (bash-trim line)))
        (if (== t "") (loop (- r 1)) r)
      ) ;let*
    ) ;if
  ) ;let
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Indentation computation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (bash-indent-level-from-prev row)
  (let* ((pr (bash-prev-nonempty-row row)))
    (if (< pr 0)
      0
      (let* ((pline (bash-get-line pr))
             (trimmed (bash-trim pline))
             (base (string-get-indent pline))
             (tab (get-tabstop))
             (inc? (or (bash-line-continues? pline)
                     ;; previous line ends with "{"
                     (and (> (string-length trimmed) 0)
                       (== (string-ref trimmed (- (string-length trimmed) 1)) #\{)
                     ) ;and
                     ;; crude: control keywords that often start blocks
                     (let loop
                       ((ws bash-block-keywords))
                       (if (null? ws)
                         #f
                         (let ((w (car ws)))
                           (if (bash-string-prefix? trimmed w) #t (loop (cdr ws)))
                         ) ;let
                       ) ;if
                     ) ;let
                   ) ;or
             ) ;inc?
            ) ;
        (+ base (if inc? tab 0))
      ) ;let*
    ) ;if
  ) ;let*
) ;define

(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-bash?)
  (let* ((tab (get-tabstop))
         (line (bash-get-line row))
         (base (bash-indent-level-from-prev row))
        ) ;
    (cond ((or (bash-line-starts-with-closing-brace? line)
             (bash-line-starts-with-fi-done-esac? line)
           ) ;or
           (max 0 (- base tab))
          ) ;
          (else base)
    ) ;cond
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Commenting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (program-comment-start) (:mode in-prog-bash?) "#")

(tm-define (program-toggle-comment)
  (:mode in-prog-bash?)
  (prog-toggle-line-comment "#")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Paste import hook
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-paste)
  (:mode in-prog-bash?)
  (clipboard-paste-import "bash" "primary")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Brackets / quotes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (bash-bracket-open lbr rbr) (bracket-open lbr rbr "\\"))

(tm-define (bash-bracket-close lbr rbr) (bracket-close lbr rbr "\\"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-bash?)
  (select-brackets-after-movement "([{" ")]}" "\\")
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard mappings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map (:mode in-prog-bash?)
 ("A-tab" (insert-tabstop))
 ("cmd S-tab" (remove-tabstop))
 ("{" (bash-bracket-open "{" "}"))
 ("}" (bash-bracket-close "{" "}"))
 ("(" (bash-bracket-open "(" ")"))
 (")" (bash-bracket-close "(" ")"))
 ("[" (bash-bracket-open "[" "]"))
 ("]" (bash-bracket-close "[" "]"))
 ("\"" (bash-bracket-open "\"" "\""))
 ("'" (bash-bracket-open "'" "'"))
) ;kbd-map
