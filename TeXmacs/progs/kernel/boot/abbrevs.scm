
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : abbrevs.scm
;; DESCRIPTION : useful abbreviations
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (kernel boot abbrevs))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Common notations
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public == equal?)
(define-public (!= x y) (not (equal? x y)))

(define-public (nsymbol? x) (not (symbol? x)))
(define-public (nstring? x) (not (string? x)))
(define-public (nnull? x) (not (null? x)))
(define-public (npair? x) (not (pair? x)))
(define-public (nlist? x) (not (list? x)))
(define-public (nnot x) (not (not x)))
(define-public-macro (toggle! x) `(set! ,x (not ,x)))
(define-public (safe-car l) (and (pair? l) (car l)))
(define-public (safe-cdr l) (and (pair? l) (cdr l)))

(define-public (list-1? x) (and (pair? x) (null? (cdr x))))
(define-public (nlist-1? x) (not (list-1? x)))
(define-public (list-2? x) (and (list? x) (= (length x) 2)))
(define-public (nlist-2? x) (not (list-2? x)))
(define-public (list-3? x) (and (list? x) (= (length x) 3)))
(define-public (nlist-3? x) (not (list-3? x)))
(define-public (list-4? x) (and (list? x) (= (length x) 4)))
(define-public (nlist-4? x) (not (list-4? x)))
(define-public (list>0? x) (and (pair? x) (list? x)))
(define-public (nlist>0? x) (not (list>0? x)))
(define-public (list>1? x) (and (list? x) (> (length x) 1)))
(define-public (nlist>1? x) (not (list>1? x)))

(define-public (in? x l) (not (not (member x l))))
(define-public (nin? x l) (not (member x l)))
(define-public (cons-new x l) (if (in? x l) l (cons x l)))

(define-public (always? . l) #t)
(define-public (never? . l) #f)
(define-public (root? t) (== (reverse (tree-ip t)) (buffer-path)))
(define-public (nroot? t) (!= (reverse (tree-ip t)) (buffer-path)))
(define-public (leaf? t) (== (tree-ip t) (cdr (reverse (cursor-path)))))
(define-public (nleaf? t) (!= (tree-ip t) (cdr (reverse (cursor-path)))))
(define-public (true? . l) #t)
(define-public (false? . l) #f)

(provide-public (identity x) x)
(provide-public (ignore . l) (noop))
(provide-public (negate pred?) (lambda x (not (apply pred? x))))

(define-public (keyword->string x) (symbol->string (keyword->symbol x)))
(define-public (string->keyword x) (symbol->keyword (string->symbol x)))
(define-public (keyword->number x)
  (string->number (string-tail (symbol->string (keyword->symbol x)) 1))
) ;define-public
(define-public (number->keyword x)
  (symbol->keyword (string->symbol (string-append "%" (number->string x))))
) ;define-public

(define-public (save-object file value)
  (string-save (let-temporarily (((*s7* 'print-length) 9223372036854775807))
                 (object->string value)
               ) ;let-temporarily
    file
  ) ;string-save
) ;define-public

(define-public (load-object file)
  (with-input-from-string (string-load file) (lambda () (read)))
) ;define-public

(define-public (persistent-ref dir key)
  (and (persistent-has? dir key) (persistent-get dir key))
) ;define-public

(define-public (sourcify x)
  (if (and (procedure? x) (procedure-source x)) (procedure-source x) x)
) ;define-public

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Common programming constructs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public-macro (with var val . body)
  (if (or (pair? var) (null? var))
    `(apply (lambda ,var ,@body) ,val)
    `(let ((,var ,val)) ,@body)
  ) ;if
) ;define-public-macro

(define-public-macro (with-define fun fun-body . body)
  `(let ((,(car fun) (lambda ,(cdr fun) ,fun-body))) ,@body)
) ;define-public-macro

;; handle multiple values in a way compatible with s7 (and backcompatible with guile)
(define-public-macro (with-global var val . body)
  (let ((old (gensym)) (new (gensym)))
    `(let ((,old ,var))
       (set! ,var ,val)
       (call-with-values (lambda ,() ,@body)
         (lambda vals (set! ,var ,old) (apply values vals))))
  ) ;let
) ;define-public-macro

(define-public-macro (and-with var val . body)
  `(with ,var ,val (and ,var (begin ,@body)))
) ;define-public-macro

(define-public-macro (with-result result . body)
  `(let* ((return ,result) (dummy (begin ,@body))) return)
) ;define-public-macro

(define (range-list start end delta)
  (if (< start end) (cons start (range-list (+ start delta) end delta)) '())
) ;define

(define (range-list* start end delta)
  (if (<= start end) (cons start (range-list* (+ start delta) end delta)) '())
) ;define

(define-public (.. start end . delta)
  (if (null? delta) (range-list start end 1) (range-list start end (car delta)))
) ;define-public

(define-public (... start end . delta)
  (if (null? delta) (range-list* start end 1) (range-list* start end (car delta)))
) ;define-public

(define-public-macro (for what . body)
  (let ((n (length what)))
    (cond ((== n 2)
           ;; range over values of a list
           `(for-each (lambda (,(car what)) ,@body) ,(cadr what))
          ) ;
          ((== n 3)
           ;; range over values from start to end with step 1
           `(do ((,(car what) ,(cadr what) (+ ,(car what) ,1)))
              ((>= ,(car what) ,(caddr what)) (noop))
              ,@body)
          ) ;
          ((== n 4)
           ;; range over values from start to end with step
           `(if (> ,(cadddr what) ,0)
              (do ((,(car what) ,(cadr what) (+ ,(car what) ,(cadddr what))))
                ((>= ,(car what) ,(caddr what)) (noop))
                ,@body)
              (do ((,(car what) ,(cadr what) (+ ,(car what) ,(cadddr what))))
                ((<= ,(car what) ,(caddr what)) (noop))
                ,@body))
          ) ;
          ((== n 5)
           ;; range over values from start to end with step and comparison
           `(do ((,(car what) ,(cadr what) (+ ,(car what) ,(cadddr what))))
              ((not (,(car (cddddr what)) ,(car what) ,(caddr what))) (noop))
              ,@body)
          ) ;
          (else '(noop))
    ) ;cond
  ) ;let
) ;define-public-macro

(define-public-macro (repeat n . body)
  (let ((x (gensym)))
    `(for (,x ,0 ,n) ,@body)
  ) ;let
) ;define-public-macro

(define-public-macro (twice . body) `(begin ,@body ,@body))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Small rewritings on top of C++ interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define-public (path->tree p) (and (path-exists? p) (cpp-path->tree p)))

(define-public selection-active? selection-active-any?)

(define-public (selection-active-non-small?)
  (and (selection-active?) (not (selection-active-small?)))
) ;define-public

(define-public (selection-active-large?)
  (and (selection-active?)
    (not (selection-active-small?))
    (not (selection-active-table?))
  ) ;and
) ;define-public

(define-public (go-to p)
  (let* ((r (buffer-path)) (lp (length p)) (lr (length r)))
    (and (or (and (<= lr lp) (== (sublist p 0 lr) r))
           (and-with buf (path->buffer p) (switch-to-buffer buf) #t)
         ) ;or
      (go-to-path p)
    ) ;and
  ) ;let*
) ;define-public

(define-public (choose-file fun title type . opts)
  (when (null? opts)
    (with prompt
      (cond ((string-starts? title "Save") "Save as:")
            ((string-starts? title "Export") "Export as:")
            ((== title "Select database") "Selected database:")
            (else "")
      ) ;cond
      (set! opts (list prompt))
    ) ;with
  ) ;when
  (when (null? (cdr opts))
    ;; Issue #327: Use last file dialog directory if current buffer is scratch
    (let* ((master (buffer-get-master (current-buffer)))
           (last-dir (and (url-scratch? master)
                       (defined? 'get-last-file-dialog-directory)
                       (get-last-file-dialog-directory)
                     ) ;and
           ) ;last-dir
          ) ;
      (if (and last-dir (string? last-dir) (not (string-null? last-dir)))
        (set! opts (list (car opts) (system->url last-dir)))
        (set! opts (list (car opts) master))
      ) ;if
    ) ;let*
  ) ;when
  (cpp-choose-file (lambda (u)
                     ;; u is return from tm_frame_rep::choose_file in tm_dialogue.cpp
                     ;; make sure u is a url, or car of u is a url
                     ;; and that it does not contain a wildcard
                     (if (or (url? u) (and (pair? u) (url? (car u))))
                       (let ((u-url (if (url? u) u (car u))))
                         (if (and (not (url-none? u-url)) (url-contains-wildcard? u-url))
                           (dialogue-window (message-widget "File name and path cannot contain ' * '")
                             (lambda () (choose-file fun title type (car opts) (cadr opts)))
                             "Invalid file name"
                           ) ;dialogue-window
                           (fun u)
                         ) ;if
                       ) ;let
                       (fun u)
                     ) ;if
                   ) ;lambda
    title
    type
    (car opts)
    (cadr opts)
  ) ;cpp-choose-file
) ;define-public

(define-public (alt-windows-delete l) (for-each alt-window-delete l))

(define-public (qt4-gui?) (== (gui-version) "qt4"))
(define-public (qt4-or-later-gui?) (in? (gui-version) (list "qt4" "qt5" "qt6")))
(define-public (qt5-gui?) (== (gui-version) "qt5"))
(define-public (qt5-or-later-gui?) (in? (gui-version) (list "qt5" "qt6")))
(define-public (qt6-gui?) (== (gui-version) "qt6"))
(define-public (qt6-or-later-gui?) (in? (gui-version) (list "qt6")))
