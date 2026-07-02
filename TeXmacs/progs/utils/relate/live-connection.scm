
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : live-connection.scm
;; DESCRIPTION : Remote connection management for live shared documents
;; COPYRIGHT   : (C) 2015  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils relate live-connection)
  (:use (utils relate live-document))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Useful utility: conversions between patches and lists of modifications
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (patch->modlist p)
  (cond ((patch-pair? p) (list (modification->scheme (patch-direct p))))
        ((patch-compound? p) (append-map patch->modlist (patch-children p)))
        ((patch-branch? p) (patch->modlist (patch-ref p 0)))
        ((patch-birth? p) (list))
        ((patch-author? p) (patch->modlist (patch-ref p 0)))
        (else (list))
  ) ;cond
) ;tm-define

(tm-define (modlist->patch mods t)
  (with r
    (list)
    (while (nnull? mods)
      (let* ((m (scheme->modification (car mods)))
             (inv (modification-invert m t))
             (p (patch-pair m inv))
            ) ;
        (set! r (cons p r))
        (set! t (modification-apply t m))
        (set! mods (cdr mods))
      ) ;let*
    ) ;while
    (patch-compound (reverse r))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Manage live connections
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define live-connections (make-ahash-table))

(define (live-connections-table lid)
  (when (not (ahash-ref live-connections lid))
    (ahash-set! live-connections lid (make-ahash-table))
  ) ;when
  (ahash-ref live-connections lid)
) ;define

(tm-define (live-connect lid remote)
  (live-set-remote-state lid remote (live-current-state lid))
) ;tm-define

(tm-define (live-hang-up lid remote)
  (with t
    (live-connections-table lid)
    (ahash-remove! t remote)
    (when (== (ahash-size t) 0)
      (ahash-remove! live-connections lid)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (live-get-connections lid)
  (with t (live-connections-table lid) (map car (ahash-table->list t)))
) ;tm-define

(tm-define (live-set-remote-state lid remote state)
  (with t (live-connections-table lid) (ahash-set! t remote state))
) ;tm-define

(tm-define (live-get-remote-state lid remote)
  (with t (live-connections-table lid) (ahash-ref t remote))
) ;tm-define

(tm-define (live-remote-connections remote)
  (with lids
    (map car (ahash-table->list live-connections))
    (list-filter lids (lambda (lid) (in? remote (live-get-connections lid))))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Manage states in use
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (live-connection-states lid)
  (when (not (ahash-ref live-connections lid))
    (ahash-set! live-connections lid (make-ahash-table))
  ) ;when
  (with t (ahash-ref live-connections lid) (map cdr (ahash-table->list t)))
) ;tm-define

(tm-define (live-states-in-use lid)
  (append (former lid) (live-connection-states lid))
) ;tm-define
