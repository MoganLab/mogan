
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : lp-build.scm
;; DESCRIPTION : building programs from literate source files
;; COPYRIGHT   : (C) 2015  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils literate lp-build) (:use (utils literate lp-edit)))

(import (liii hash-table) (liii base) (liii list))

(define code-table (s7-make-hash-table))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Building the initial table (without substitutions)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (extract-lines t)
  (cond ((tm-atomic? t) (list t))
        ((tm-func? t 'document) (flat-map extract-lines (tm-children t)))
        ((or (tm-func? t 'folded-newline-before 1)
           (tm-func? t 'unfolded-newline-before 1)
         ) ;or
         (cons "" (extract-lines (tm-ref t 0)))
        ) ;
        (else (with l
                (filter (cut tm-func? <> 'document) (tm-children t))
                (if (null? l) (list t) (flat-map extract-lines l))
              ) ;with
        ) ;else
  ) ;cond
) ;define

(define (build-table t)
  (let* ((l (get-all-chunks)) (ht (s7-make-hash-table)))
    (for (c l)
      (let* ((name (tm->string (tm-ref c 0)))
             (body0 (tm-ref c 3))
             (body (if (not (tm-func? body0 'document)) `(document ,body0) body0))
             (old (tm-children (hash-table-ref/default ht name '(document))))
             (new (append old (extract-lines body)))
            ) ;
        (hash-table-set! ht name `(document ,@new))
      ) ;let*
    ) ;for
    ht
  ) ;let*
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Performing the necessary substitutions
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (ahash-table-split t pred?)
  (let* ((t1 (make-ahash-table)) (t2 (make-ahash-table)))
    (for (key (hash-table-keys t))
      (with im
        (ahash-ref t key)
        (if (pred? (ahash-ref t key)) (ahash-set! t1 key im) (ahash-set! t2 key im))
      ) ;with
    ) ;for
    (list t1 t2)
  ) ;let*
) ;define

(define (should-expand? t)
  (cond ((tm-atomic? t) #f)
        ((tm-func? t 'chunk-ref 1) #t)
        (else (list-or (map should-expand? (tm-children t))))
  ) ;cond
) ;define

(define (expand-names x t)
  (cond ((tm-atomic? x) x)
        ((tm-func? x 'document)
         (let* ((l1 (map (cut expand-names <> t) (tm-children x)))
                (l2 (map (lambda (y) (if (tm-func? y 'document) (tm-children y) (list y))) l1))
               ) ;
           `(document ,@(apply append l2))
         ) ;let*
        ) ;
        ((tm-func? x 'chunk-ref 1)
         (if (and (tm-atomic? (tm-ref x 0)) (ahash-ref t (tm->string (tm-ref x 0))))
           (ahash-ref t (tm->string (tm-ref x 0)))
           x
         ) ;if
        ) ;
        ((tm-func? x 'concat)
         (if (and (tm-func? (tm-ref x :last) 'chunk-ref 1)
               (tm-atomic? (tm-ref x :last 0))
               (ahash-ref t (tm->string (tm-ref x :last 0)))
             ) ;and
           (let* ((count (lambda (x) (if (tm-atomic? x) (tmstring-length (tm->string x)) 0)))
                  (i (apply + (map count (cDr (tm-children x)))))
                  (pre (apply string-append (map (lambda (y) " ") (.. 0 i))))
                  (doc (ahash-ref t (tm->string (tm-ref x :last 0))))
                  (lines (tm-children doc))
                 ) ;
             `(document (concat ,@(cDr (tm-children x)) ,(car lines))
                ,@(map (lambda (x) (tmconcat pre x)) (cdr lines)))
           ) ;let*
           x
         ) ;if
        ) ;
        (else x)
  ) ;cond
) ;define

(define (expand-table t)
  (with (todo done)
    (ahash-table-split t should-expand?)
    (with ok?
      #f
      (for (key (map car (ahash-table->list todo)))
        (with subst
          (expand-names (ahash-ref todo key) done)
          (when (not (should-expand? subst))
            (ahash-set! done key subst)
            (set! ok? #t)
          ) ;when
        ) ;with
      ) ;for
      (cond ((== (ahash-size todo) 0) done)
            (ok? (for (key (map car (ahash-table->list todo)))
                   (when (not (ahash-ref done key))
                     (ahash-set! done key (ahash-ref todo key))
                   ) ;when
                 ) ;for
              (expand-table done)
            ) ;ok?
            (else (for (key (map car (ahash-table->list todo)))
                    (display* "TeXmacs] Problematic chunk: " key "\n")
                  ) ;for
              (set-message "Error: cyclic or missing chunks detected" "build-all")
              done
            ) ;else
      ) ;cond
    ) ;with
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Debugging subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (display-table t)
  (for (key (map car (ahash-table->list t)))
    (display* key "\n----------------------------------\n")
    (with im (tm-children (ahash-ref t key)) (for (l im) (display* "  " l "\n")))
    (display* "----------------------------------\n\n")
  ) ;for
) ;define

(define (display-table* t*)
  (with t
    (verbatim-table t*)
    (for (key (map car (ahash-table->list t)))
      (display* key "\n----------------------------------\n")
      (display* (ahash-ref t key))
      (display* "----------------------------------\n\n")
    ) ;for
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Memorize which files have been built
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define lp-db (url->url "$TEXMACS_HOME_PATH/system/database/lp-master.tmdb"))

(define (lp-set-time-stamp src dir)
  (tmdb-keep-history lp-db #f)
  (let* ((src* (url->system src))
         (dir* (url->system dir))
         (id (string-append src* "-" dir*))
         (time (number->string (url-last-modified src)))
        ) ;
    (tmdb-set-field lp-db id "time-stamp" (list time) (current-time))
  ) ;let*
) ;define

(define (lp-get-time-stamp src dir)
  (tmdb-keep-history lp-db #f)
  (let* ((src* (url->system src))
         (dir* (url->system dir))
         (id (string-append src* "-" dir*))
        ) ;
    (with l
      (tmdb-get-field lp-db id "time-stamp" (current-time))
      (and l (nnull? l) (car l))
    ) ;with
  ) ;let*
) ;define

(define (lp-build-conditional src dir fun)
  (with stamp
    (lp-get-time-stamp src dir)
    (when (or (not stamp)
            (not (url-exists? src))
            (not (url-exists? dir))
            (< (string->number stamp) (url-last-modified src))
          ) ;or
      (fun src dir)
      (when (and (url-exists? src) (url-exists? dir))
        (lp-set-time-stamp src dir)
      ) ;when
    ) ;when
  ) ;with
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Main build process
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (verbatim-table ht)
  (with r
    (s7-make-hash-table)
    (for-each (lambda (key-val)
                (let* ((key (car key-val))
                       (val (cdr key-val))
                       (l (tm-children (tm->stree val)))
                       (l-code (map (cut texmacs->code <> "SourceCode") l))
                       (l-code-nl (map (cut string-append <> "\n") l-code))
                       (s-code (apply string-append l-code-nl))
                      ) ;
                  (hash-table-set! r key s-code)
                ) ;let*
              ) ;lambda
      (map values ht)
    ) ;for-each
    r
  ) ;with
) ;define

(define (write-table ht dir)
 ((box (map values ht))
  :for-each
  (lambda (pair)
    (let* ((key (car pair)) (val (cdr pair)))
      (with target
        (url-append dir key)
        (when (not (url-exists? (url-head target)))
          (system-mkdir (url-head target))
        ) ;when
        (when (!= (hash-table-ref/default code-table key "") val)
          (display* "TeXmacs] Building " (url->system target) "\n")
          (string-save val target)
          (hash-table-set! code-table key val)
        ) ;when
      ) ;with
    ) ;let*
  ) ;lambda
 ) ;
) ;define

(define (lp-build* file dir)
  (with doc
    (if (buffer-exists? file)
      (buffer-get-body file)
      (tmfile-extract (tree-import file "texmacs") 'body)
    ) ;if
    (with t
      (build-table doc)
      (with x (expand-table t) (with v (verbatim-table x) (write-table v dir)))
    ) ;with
  ) ;with
) ;define

(define (lp-build file dir)
  (if (buffer-exists? file)
    (lp-build* file dir)
    (lp-build-conditional file dir lp-build*)
  ) ;if
) ;define

(tm-define (lp-build-buffer)
  (update-all-chunk-states)
  (lp-build (current-buffer) (url-head (current-buffer)))
) ;tm-define

(tm-define (lp-build-buffer-in dir)
  (:argument dir "Build directory")
  (update-all-chunk-states)
  (lp-build (current-buffer) dir)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Building all files in a directory
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (lp-build-directory source target)
  (let* ((f (url-append (url-append source (url-any)) (url-wildcard "*.tm")))
         (l (url->list (url-expand (url-complete f "r"))))
        ) ;
    (for (src l)
      (let* ((rel (url-delta (url-append source "dummy") src))
             (obj (url-append target rel))
            ) ;
        (lp-build src (url-head obj))
      ) ;let*
    ) ;for
  ) ;let*
) ;define

(tm-define (lp-interactive-build-directory)
  (:interactive #t)
  (user-url "Directory" "directory" (lambda (src) (lp-build-directory src src)))
) ;tm-define

(tm-define (lp-interactive-build-directory-in)
  (:interactive #t)
  (user-url "Source directory"
    "directory"
    (lambda (src)
      (user-url "Destination directory"
        "directory"
        (lambda (dest) (lp-build-directory src dest))
      ) ;user-url
    ) ;lambda
  ) ;user-url
) ;tm-define
