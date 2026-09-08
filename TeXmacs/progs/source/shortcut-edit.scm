
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

(texmacs-module (source shortcut-edit) (:use (source macro-edit)))
(import (liii json))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Management of the list of user keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define user-shortcuts-file "$TEXMACS_HOME_PATH/system/shortcuts.json")

(define legacy-user-shortcuts-file "$TEXMACS_HOME_PATH/system/shortcuts.scm")

(define user-shortcuts-migration-marker-v1
  "$TEXMACS_HOME_PATH/system/shortcuts.scm->v1"
) ;define

(define user-shortcuts-version 1)

(define (make-shortcut-entry sh cmd)
  `((,"shortcut" . ,sh) (,"command" . ,cmd))
) ;define

(define (shortcut-entry-shortcut entry)
  (and (json-object? entry) (json-ref-string entry "shortcut" #f))
) ;define

(define (shortcut-entry-command entry)
  (and (json-object? entry) (json-ref-string entry "command" #f))
) ;define

(define (shortcut-entry-valid? entry)
  (and (json-object? entry)
    (string? (shortcut-entry-shortcut entry))
    (string? (shortcut-entry-command entry))
  ) ;and
) ;define

(define (shortcut-entries-valid? entries)
  (or (null? entries)
    (and (shortcut-entry-valid? (car entries))
      (shortcut-entries-valid? (cdr entries))
    ) ;and
  ) ;or
) ;define

(define (user-shortcuts-json-valid? data)
  (and (json-object? data)
    (let ((meta (json-ref data "meta")) (shortcuts (json-ref data "shortcuts")))
      (and (json-object? meta)
        (integer? (json-ref meta "version"))
        (let ((total (json-ref meta "total")))
          (and (integer? total)
            (>= total 0)
            (vector? shortcuts)
            (== total (vector-length shortcuts))
            (shortcut-entries-valid? (vector->list shortcuts))
          ) ;and
        ) ;let
      ) ;and
    ) ;let
  ) ;and
) ;define

(define (make-user-shortcuts-json entries)
  `((,"meta"
     (,"version" . ,user-shortcuts-version)
     (,"total" . ,(length entries)))
    (,"shortcuts" . ,(list->vector entries)))
) ;define

(define (make-empty-user-shortcuts-json)
  (make-user-shortcuts-json '())
) ;define

(define current-user-shortcuts (make-empty-user-shortcuts-json))

(define (replace-current-user-shortcuts! next)
  (set! current-user-shortcuts next)
) ;define

(define (current-user-shortcuts-vector)
  (catch #t
    (lambda ()
      (let ((shortcuts (json-ref current-user-shortcuts "shortcuts")))
        (if (vector? shortcuts) shortcuts #())
      ) ;let
    ) ;lambda
    (lambda args #())
  ) ;catch
) ;define

(define (current-user-shortcuts-list)
  (vector->list (current-user-shortcuts-vector))
) ;define

(define (set-current-user-shortcuts-list entries)
  (replace-current-user-shortcuts! (make-user-shortcuts-json entries))
) ;define

(define (find-user-shortcut-entry sh)
  (let loop
    ((entries (current-user-shortcuts-list)))
    (and (nnull? entries)
      (let ((entry (car entries)))
        (if (== (shortcut-entry-shortcut entry) sh) entry (loop (cdr entries)))
      ) ;let
    ) ;and
  ) ;let
) ;define

(define (apply-user-shortcut sh cmd)
  (and-with val (string->object cmd) (eval `(kbd-map (,sh ,val))))
) ;define

(define (unapply-user-shortcut sh)
  (eval `(kbd-unmap ,sh))
) ;define

(define (reset-user-shortcuts)
  (replace-current-user-shortcuts! (make-empty-user-shortcuts-json))
  (save-user-shortcuts)
) ;define

(define (write-migration-marker marker-file)
  (string-save "migrated\n" (string->url marker-file))
) ;define

(define (load-legacy-scm-user-shortcuts)
  (and (url-exists? legacy-user-shortcuts-file)
    (catch #t
      (lambda ()
        (let ((loaded (load-object legacy-user-shortcuts-file)))
          (and (list? loaded) loaded)
        ) ;let
      ) ;lambda
      (lambda args #f)
    ) ;catch
  ) ;and
) ;define

(define (legacy-scm-user-shortcut-entry->json entry)
  (and (pair? entry)
    (pair? (cdr entry))
    (string? (car entry))
    (string? (cadr entry))
    (make-shortcut-entry (car entry) (cadr entry))
  ) ;and
) ;define

(define (shortcut-entry-mergeable? entry existing-entries)
  (let ((sh (shortcut-entry-shortcut entry)))
    (not (list-find existing-entries
           (lambda (existing) (== (shortcut-entry-shortcut existing) sh))
         ) ;list-find
    ) ;not
  ) ;let
) ;define

(define (merge-legacy-scm-user-shortcuts current entries)
  (let loop
    ((rest entries) (merged current))
    (if (null? rest)
      merged
      (with entry
        (car rest)
        (if (shortcut-entry-mergeable? entry merged)
          (loop (cdr rest) (append merged (list entry)))
          (loop (cdr rest) merged)
        ) ;if
      ) ;with
    ) ;if
  ) ;let
) ;define

(define (maybe-import-legacy-scm-user-shortcuts)
  (when (and (url-exists? legacy-user-shortcuts-file)
          (not (url-exists? user-shortcuts-migration-marker-v1))
        ) ;and
    (and-with legacy-shortcuts
      (load-legacy-scm-user-shortcuts)
      (let* ((entries (list-filter (map legacy-scm-user-shortcut-entry->json legacy-shortcuts)
                        (lambda (x) x)
                      ) ;list-filter
             ) ;entries
             (current (current-user-shortcuts-list))
             (merged (merge-legacy-scm-user-shortcuts current entries))
            ) ;
        (when (not (equal? merged current))
          (set-current-user-shortcuts-list merged)
        ) ;when
        (save-user-shortcuts)
        (write-migration-marker user-shortcuts-migration-marker-v1)
      ) ;let*
    ) ;and-with
  ) ;when
) ;define

(define (load-user-shortcuts)
  (replace-current-user-shortcuts! (make-empty-user-shortcuts-json))
  (when (url-exists? user-shortcuts-file)
    (let ((loaded (catch #t
                    (lambda () (string->json (string-load user-shortcuts-file)))
                    (lambda args #f)
                  ) ;catch
          ) ;loaded
         ) ;
      (if (user-shortcuts-json-valid? loaded)
        (replace-current-user-shortcuts! loaded)
        (reset-user-shortcuts)
      ) ;if
    ) ;let
  ) ;when
  (maybe-import-legacy-scm-user-shortcuts)
  (for (entry (current-user-shortcuts-list))
    (apply-user-shortcut (shortcut-entry-shortcut entry)
      (shortcut-entry-command entry)
    ) ;apply-user-shortcut
  ) ;for
) ;define

(define (save-user-shortcuts)
  (string-save (json->string current-user-shortcuts) user-shortcuts-file)
) ;define

(tm-define (init-user-shortcuts) (load-user-shortcuts))

(define (shortcut-rewrite s1)
  (let* ((s2 (string-replace s1 "A-" "~A"))
         (s3 (string-replace s2 "C-" "~C"))
         (s4 (string-replace s3 "M-" "~M"))
         (s5 (string-replace s4 "S-" "~S"))
        ) ;
    s5
  ) ;let*
) ;define

(define (shortcut<=? s1 s2)
  (string<=? (shortcut-rewrite s1) (shortcut-rewrite s2))
) ;define

(tm-define (user-shortcuts-list)
  (list-sort (map shortcut-entry-shortcut (current-user-shortcuts-list))
    shortcut<=?
  ) ;list-sort
) ;tm-define

(tm-define (set-user-shortcut sh cmd)
  (let* ((entries (current-user-shortcuts-list))
         (others (list-filter entries (lambda (entry) (!= (shortcut-entry-shortcut entry) sh)))
         ) ;others
         (next (append others (list (make-shortcut-entry sh cmd))))
        ) ;
    (set-current-user-shortcuts-list next)
  ) ;let*
  (save-user-shortcuts)
  (apply-user-shortcut sh cmd)
) ;tm-define

(tm-define (get-user-shortcut sh)
  (and-with entry (find-user-shortcut-entry sh) (shortcut-entry-command entry))
) ;tm-define

(tm-define (remove-user-shortcut sh)
  (set-current-user-shortcuts-list (list-filter (current-user-shortcuts-list)
                                     (lambda (entry) (!= (shortcut-entry-shortcut entry) sh))
                                   ) ;list-filter
  ) ;set-current-user-shortcuts-list
  (save-user-shortcuts)
  (unapply-user-shortcut sh)
) ;tm-define

(tm-define (has-user-shortcut? cmd)
  (in? cmd (map shortcut-entry-command (current-user-shortcuts-list)))
) ;tm-define

(tm-define (encode-shortcut sh) (translate (kbd-system-rewrite sh)))

(define (normalize-shortcut-string sh)
  (if (not (string? sh))
    sh
    (let* ((s1 (string-replace sh "<less>" "<"))
           (s2 (string-replace s1 "<gtr>" ">"))
           (l (list-filter (string-tokenize-by-char s2 #\space) (lambda (x) (!= x ""))))
          ) ;
      (string-join l " ")
    ) ;let*
  ) ;if
) ;define

(tm-define (decode-shortcut sh)
  (let* ((sh* (normalize-shortcut-string sh))
         (all (map (lambda (x) (cons (encode-shortcut x) x))
                (map shortcut-entry-shortcut (current-user-shortcuts-list))
              ) ;map
         ) ;all
        ) ;
    (or (assoc-ref all sh) (assoc-ref all sh*) sh*)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Editing keyboard shortcuts
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (in-shortcut-editor?)
  (tree-func? (cursor-tree) 'preview-shortcut 1)
) ;define

(tm-define (zoom-in x) (:require (in-shortcut-editor?)) (noop))

(tm-define (zoom-out x) (:require (in-shortcut-editor?)) (noop))

(tm-define (change-zoom-factor z) (:require (in-shortcut-editor?)) (noop))

(tm-define (keyboard-press key time)
  (if (not (in-shortcut-editor?))
    (former key time)
    (and-let* ((t (cursor-tree)) (sh (tm-ref t 0)) (old (tm->string sh)))
      (if (or (== (cAr (cursor-path)) 0) (== old ""))
        (tree-set! sh key)
        (tree-set! sh (string-append old " " key))
      ) ;if
      (tree-go-to t :end)
    ) ;and-let*
  ) ;if
) ;tm-define
