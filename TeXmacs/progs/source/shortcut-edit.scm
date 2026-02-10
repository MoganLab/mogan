
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : shortcut-edit.scm
;; DESCRIPTION : editing keyboard shortcuts
;; COPYRIGHT   : (C) 2020  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (source shortcut-edit)
  (:use (source macro-edit)))
(import (liii json))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Management of the list of user keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define user-shortcuts-file "$TEXMACS_HOME_PATH/system/shortcuts.json")
(define user-shortcuts-version 1)

(define (make-shortcut-entry sh cmd)
  `(("shortcut" . ,sh)
    ("command" . ,cmd)))

(define (shortcut-entry-shortcut entry)
  (json-ref entry "shortcut"))

(define (shortcut-entry-command entry)
  (json-ref entry "command"))

(define (shortcut-entry-valid? entry)
  (and (json-object? entry)
       (string? (shortcut-entry-shortcut entry))
       (string? (shortcut-entry-command entry))))

(define (shortcut-entries-valid? entries)
  (or (null? entries)
      (and (shortcut-entry-valid? (car entries))
           (shortcut-entries-valid? (cdr entries)))))

(define (user-shortcuts-json-valid? json)
  (and (json-object? json)
       (let* ((meta (json-ref json "meta"))
              (shortcuts (json-ref json "shortcuts"))
              (version (and (json-object? meta) (json-ref meta "version")))
              (total (and (json-object? meta) (json-ref meta "total"))))
         (and (json-object? meta)
              (integer? version)
              (integer? total)
              (>= total 0)
              (vector? shortcuts)
              (== total (vector-length shortcuts))
              (shortcut-entries-valid? (vector->list shortcuts))))))

(define (make-user-shortcuts-json shortcuts)
  `(("meta" . (("version" . ,user-shortcuts-version)
               ("total" . ,(vector-length shortcuts))))
    ("shortcuts" . ,shortcuts)))

(define (make-empty-user-shortcuts-json)
  (make-user-shortcuts-json #()))

(define current-user-shortcuts (make-empty-user-shortcuts-json))

(define (current-user-shortcuts-vector)
  (with shortcuts (json-ref current-user-shortcuts "shortcuts")
    (if (vector? shortcuts) shortcuts #())))

(define (current-user-shortcuts-list)
  (vector->list (current-user-shortcuts-vector)))

(define (normalize-user-shortcuts-json json)
  (let* ((shortcuts (if (and (json-object? json) (vector? (json-ref json "shortcuts")))
                        (json-ref json "shortcuts")
                        #()))
         (entries (vector->list shortcuts))
         (valid (list-filter entries shortcut-entry-valid?)))
    (make-user-shortcuts-json (list->vector valid))))

(define (set-current-user-shortcuts-list entries)
  (set! current-user-shortcuts
        (make-user-shortcuts-json (list->vector entries))))

(define (find-user-shortcut-entry sh)
  (let loop ((entries (current-user-shortcuts-list)))
    (and (nnull? entries)
         (let ((entry (car entries)))
           (if (== (shortcut-entry-shortcut entry) sh)
               entry
               (loop (cdr entries)))))))

(define (apply-user-shortcut sh cmd)
  (and-with val (string->object cmd)
    (eval `(kbd-map (,sh ,val)))))

(define (unapply-user-shortcut sh)
  (eval `(kbd-unmap ,sh)))

(define (reset-user-shortcuts)
  (set! current-user-shortcuts (make-empty-user-shortcuts-json))
  (save-user-shortcuts))

(define (load-user-shortcuts)
  (set! current-user-shortcuts (make-empty-user-shortcuts-json))
  (when (url-exists? user-shortcuts-file)
    (let ((loaded
           (catch #t
             (lambda ()
               (string->json (string-load user-shortcuts-file)))
             (lambda args #f))))
      (if (user-shortcuts-json-valid? loaded)
          (set! current-user-shortcuts
                (normalize-user-shortcuts-json loaded))
          (reset-user-shortcuts))))
  (for (entry (current-user-shortcuts-list))
    (apply-user-shortcut (shortcut-entry-shortcut entry)
                         (shortcut-entry-command entry))))

(define (save-user-shortcuts)
  (string-save (json->string current-user-shortcuts)
               user-shortcuts-file))

(tm-define (init-user-shortcuts)
  (load-user-shortcuts))

(define (shortcut-rewrite s1)
  (let* ((s2 (string-replace s1 "A-" "~A"))
         (s3 (string-replace s2 "C-" "~C"))
         (s4 (string-replace s3 "M-" "~M"))
         (s5 (string-replace s4 "S-" "~S")))
    s5))

(define (shortcut<=? s1 s2)
  (string<=? (shortcut-rewrite s1) (shortcut-rewrite s2)))

(tm-define (user-shortcuts-list)
  (list-sort (map shortcut-entry-shortcut (current-user-shortcuts-list))
             shortcut<=?))

(tm-define (set-user-shortcut sh cmd)
  (let* ((entries (current-user-shortcuts-list))
         (others (list-filter entries
                              (lambda (entry)
                                (!= (shortcut-entry-shortcut entry) sh))))
         (next (append others (list (make-shortcut-entry sh cmd)))))
    (set-current-user-shortcuts-list next))
  (save-user-shortcuts)
  (apply-user-shortcut sh cmd))

(tm-define (get-user-shortcut sh)
  (and-with entry (find-user-shortcut-entry sh)
    (shortcut-entry-command entry)))

(tm-define (remove-user-shortcut sh)
  (set-current-user-shortcuts-list
    (list-filter (current-user-shortcuts-list)
                 (lambda (entry)
                   (!= (shortcut-entry-shortcut entry) sh))))
  (save-user-shortcuts)
  (unapply-user-shortcut sh))

(tm-define (has-user-shortcut? cmd)
  (in? cmd (map shortcut-entry-command (current-user-shortcuts-list))))

(tm-define (encode-shortcut sh)
  (translate (kbd-system-rewrite sh)))

(define (normalize-shortcut-string sh)
  (if (not (string? sh)) sh
      (let* ((s1 (string-replace sh "<less>" "<"))
             (s2 (string-replace s1 "<gtr>" ">"))
             (l (list-filter (string-tokenize-by-char s2 #\space)
                             (lambda (x) (!= x "")))))
        (string-join l " "))))

(tm-define (decode-shortcut sh)
  (let* ((sh* (normalize-shortcut-string sh))
         (all (map (lambda (x) (cons (encode-shortcut x) x))
                   (map shortcut-entry-shortcut (current-user-shortcuts-list)))))
    (or (assoc-ref all sh)
        (assoc-ref all sh*)
        sh*)))
            
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Editing keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (keyboard-press key time)
  (if (not (tree-func? (cursor-tree) 'preview-shortcut 1))
      (former key time)
      (and-let* ((t (cursor-tree))
                 (sh (tm-ref t 0))
                 (old (tm->string sh)))
        (if (or (== (cAr (cursor-path)) 0) (== old ""))
            (tree-set! sh key)
            (tree-set! sh (string-append old " " key)))
        (tree-go-to t :end))))
