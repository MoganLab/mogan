
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
(import (liii njson))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Management of the list of user keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define user-shortcuts-file "$TEXMACS_HOME_PATH/system/shortcuts.json")
(define user-shortcuts-file-system
  (url->system (string->url user-shortcuts-file)))
(define user-shortcuts-version 1)

(define (make-shortcut-entry sh cmd)
  `(("shortcut" . ,sh)
    ("command" . ,cmd)))

(define (shortcut-entry-shortcut entry)
  (assoc-ref entry "shortcut"))

(define (shortcut-entry-command entry)
  (assoc-ref entry "command"))

(define (shortcut-entry-valid? entry)
  (and (pair? entry)
       (string? (shortcut-entry-shortcut entry))
       (string? (shortcut-entry-command entry))))

(define user-shortcuts-schema-v1
  (string->njson
   "{\"type\":\"object\",\"required\":[\"meta\",\"shortcuts\"],\"properties\":{\"meta\":{\"type\":\"object\",\"required\":[\"version\",\"total\"],\"properties\":{\"version\":{\"type\":\"integer\"},\"total\":{\"type\":\"integer\",\"minimum\":0}}},\"shortcuts\":{\"type\":\"array\",\"items\":{\"type\":\"object\",\"required\":[\"shortcut\",\"command\"],\"properties\":{\"shortcut\":{\"type\":\"string\"},\"command\":{\"type\":\"string\"}}}}}}"))

(define (njson-schema-valid? schema instance)
  (catch #t
    (lambda ()
      (let ((report (njson-schema-report schema instance)))
        (hash-table-ref report 'valid?)))
    (lambda args #f)))

(define (user-shortcuts-json-valid? data)
  (and (njson-schema-valid? user-shortcuts-schema-v1 data)
       (let-njson ((shortcuts (njson-ref data "shortcuts"))
                   (total (njson-ref data "meta" "total")))
         (and (integer? total)
              (== total (njson-size shortcuts))))))

(define (make-user-shortcuts-json entries)
  (json->njson
   `(("meta" . (("version" . ,user-shortcuts-version)
                ("total" . ,(length entries))))
     ("shortcuts" . ,(list->vector entries)))))

(define (make-empty-user-shortcuts-json)
  (make-user-shortcuts-json '()))

(define current-user-shortcuts (make-empty-user-shortcuts-json))

(define (replace-current-user-shortcuts! next)
  (njson-free current-user-shortcuts)
  (set! current-user-shortcuts next))

(define (current-user-shortcuts-vector)
  (catch #t
    (lambda ()
      (let-njson ((shortcuts (njson-ref current-user-shortcuts "shortcuts")))
        (if (njson-array? shortcuts) (njson->json shortcuts) #())))
    (lambda args #())))

(define (current-user-shortcuts-list)
  (vector->list (current-user-shortcuts-vector)))

(define (set-current-user-shortcuts-list entries)
  (replace-current-user-shortcuts!
   (make-user-shortcuts-json entries)))

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
  (replace-current-user-shortcuts! (make-empty-user-shortcuts-json))
  (save-user-shortcuts))

(define (load-user-shortcuts)
  (replace-current-user-shortcuts! (make-empty-user-shortcuts-json))
  (when (url-exists? user-shortcuts-file)
    (let ((loaded
           (catch #t
             (lambda ()
               (file->njson user-shortcuts-file-system))
             (lambda args #f))))
      (if (user-shortcuts-json-valid? loaded)
          (replace-current-user-shortcuts! loaded)
          (begin
            (catch #t
              (lambda () (njson-free loaded))
              (lambda args #f))
            (reset-user-shortcuts)))))
  (for (entry (current-user-shortcuts-list))
    (apply-user-shortcut (shortcut-entry-shortcut entry)
                         (shortcut-entry-command entry))))

(define (save-user-shortcuts)
  (njson->file user-shortcuts-file-system current-user-shortcuts))

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

(define (in-shortcut-editor?)
  (tree-func? (cursor-tree) 'preview-shortcut 1))

(tm-define (zoom-in x)
  (:require (in-shortcut-editor?))
  (noop))

(tm-define (zoom-out x)
  (:require (in-shortcut-editor?))
  (noop))

(tm-define (change-zoom-factor z)
  (:require (in-shortcut-editor?))
  (noop))

(tm-define (keyboard-press key time)
  (if (not (in-shortcut-editor?))
      (former key time)
      (and-let* ((t (cursor-tree))
                 (sh (tm-ref t 0))
                 (old (tm->string sh)))
        (if (or (== (cAr (cursor-path)) 0) (== old ""))
            (tree-set! sh key)
            (tree-set! sh (string-append old " " key)))
        (tree-go-to t :end))))
