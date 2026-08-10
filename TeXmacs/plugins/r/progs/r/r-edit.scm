;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; MODULE      : r-edit.scm
;; DESCRIPTION : editing R programs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (code r-edit)
  (:use (prog prog-edit)
        (code r-mode)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Indentation policy
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (r-tabstop) 2)

(tm-define (get-tabstop)
  (:mode in-prog-r?)
  (r-tabstop))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Helpers (lightweight, line-based)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (r-string-prefix? s p)
  (and (>= (string-length s) (string-length p))
       (== (substring s 0 (string-length p)) p)))

(define (r-trim-left s)
  (let loop ((i 0) (n (string-length s)))
    (if (or (>= i n)
            (not (char-whitespace? (string-ref s i))))
        (substring s i n)
        (loop (+ i 1) n))))

(define (r-trim-right s)
  (let loop ((i (- (string-length s) 1)))
    (if (< i 0) ""
        (if (char-whitespace? (string-ref s i))
            (loop (- i 1))
            (substring s 0 (+ i 1))))))

(define (r-trim s) (r-trim-right (r-trim-left s)))

;; Continuation cues: pipe, common operators, trailing comma, %...% infix
(define (r-line-continues? line)
  (let* ((t (r-trim-right line))
         (n (string-length t)))
    (if (<= n 0) #f
        (or
          (r-string-prefix? (r-trim-left t) "|>")
          (let ((ops '("+" "-" "*" "/" "^" "=" "<-" "<<-" "->" "->>" "&" "|" "&&" "||" ":" ",")))
            (let loop ((xs ops))
              (if (null? xs) #f
                  (let* ((op (car xs))
                         (m (string-length op)))
                    (if (and (>= n m)
                             (== (substring t (- n m) n) op))
                        #t
                        (loop (cdr xs)))))))
          (and (>= n 2)
               (== (string-ref t (- n 1)) #\%)
               (let ((j (- n 2)))
                 (let loop ((k j))
                   (cond
                     ((< k 0) #f)
                     ((== (string-ref t k) #\%) #t)
                     (else (loop (- k 1)))))))))))

(define (r-line-starts-with-closing-brace? line)
  (let ((t (r-trim-left line)))
    (and (> (string-length t) 0)
         (== (string-ref t 0) #\}))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Line access 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (r-get-line row)
  (let ((s (program-row row)))
    (if s s "")))

(define (r-prev-nonempty-row row)
  (let loop ((r (- row 1)))
    (if (< r 0) -1
        (let* ((line (r-get-line r))
               (t (r-trim line)))
          (if (== t "") (loop (- r 1)) r)))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Indentation computation
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (r-indent-level-from-prev row)
  (let* ((pr (r-prev-nonempty-row row)))
    (if (< pr 0) 0
        (let* ((pline   (r-get-line pr))
               (trimmed (r-trim pline))
               (base    (string-get-indent pline))
               (tab     (get-tabstop))
               (inc?
                 (or
                   (r-line-continues? pline)
                   ;; previous line ends with "{"
                   (and (> (string-length trimmed) 0)
                        (== (string-ref trimmed (- (string-length trimmed) 1)) #\{)))))
          (+ base (if inc? tab 0))))))

(tm-define (program-compute-indentation doc row col)
  (:mode in-prog-r?)
  (let* ((tab  (get-tabstop))
         (line (r-get-line row))
         (base (r-indent-level-from-prev row)))
    (if (r-line-starts-with-closing-brace? line)
        (max 0 (- base tab))
        base)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Commenting
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (program-comment-start)
  (:mode in-prog-r?)
  "#")

(tm-define (program-toggle-comment)
  (:mode in-prog-r?)
  (prog-toggle-line-comment "#"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Paste import hook
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (kbd-paste)
  (:mode in-prog-r?)
  (clipboard-paste-import "r" "primary"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Brackets / quotes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (r-bracket-open lbr rbr)
  (bracket-open lbr rbr "\\"))

(tm-define (r-bracket-close lbr rbr)
  (bracket-close lbr rbr "\\"))

(tm-define (notify-cursor-moved status)
  (:require prog-highlight-brackets?)
  (:mode in-prog-r?)
  (select-brackets-after-movement "([{" ")]}" "\\"))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Keyboard mappings
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(kbd-map
  (:mode in-prog-r?)
  ("A-tab" (insert-tabstop))
  ("cmd S-tab" (remove-tabstop))
  ("{" (r-bracket-open "{" "}" ))
  ("}" (r-bracket-close "{" "}" ))
  ("(" (r-bracket-open "(" ")" ))
  (")" (r-bracket-close "(" ")" ))
  ("[" (r-bracket-open "[" "]" ))
  ("]" (r-bracket-close "[" "]" ))
  ("\"" (r-bracket-open "\"" "\"" ))
  ("'" (r-bracket-open "'" "'" )))
