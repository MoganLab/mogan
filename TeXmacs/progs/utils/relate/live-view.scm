
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : live-view.scm
;; DESCRIPTION : Views on live shared documents
;; COPYRIGHT   : (C) 2015  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (utils relate live-view)
  (:use (utils plugins plugin-eval) (utils relate live-document))
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Live markup
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (live-context? t)
  (and (or (tree-func? t 'live-io 3) (tree-func? t 'live-io* 3))
    (tree-atomic? (tree-ref t 0))
    (tree-atomic? (tree-ref t 1))
  ) ;and
) ;tm-define

(tm-define (live-view-context? t)
  (and (tree->path t) (live-context? (tree-up t)) (== (tree-index t) 2))
) ;tm-define

(tm-define (live-view-id t)
  (or (and (live-context? t) (tree->string (tree-ref t 0)))
    (and (live-view-context? t) (live-view-id (tree-up t)))
  ) ;or
) ;tm-define

(tm-define (live-id t)
  (or (and (live-context? t) (tree->string (tree-ref t 1)))
    (and (live-view-context? t) (live-id (tree-up t)))
  ) ;or
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Manage views
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define live-views (make-ahash-table))

(define (live-view-table lid)
  (when (not (ahash-ref live-views lid))
    (ahash-set! live-views lid (make-ahash-table))
  ) ;when
  (ahash-ref live-views lid)
) ;define

(tm-define (live-exists? lid)
  (with t (ahash-ref live-views lid) (and t (!= (ahash-size t) 0)))
) ;tm-define

(tm-define (live-view-set-state lid vid state)
  (with t (live-view-table lid) (ahash-set! t vid state))
) ;tm-define

(tm-define (live-view-get-state lid vid)
  (with t (live-view-table lid) (ahash-ref t vid))
) ;tm-define

(tm-define (live-view-remove lid vid)
  (with t
    (live-view-table lid)
    (ahash-remove! t vid)
    (when (== (ahash-size t) 0)
      (ahash-remove! live-views lid)
    ) ;when
  ) ;with
) ;tm-define

(tm-define (live-view-clean lid)
  (with t
    (live-view-table lid)
    (for (vid (map car (ahash-table->list t)))
      (when (null? (id->trees vid))
        (live-view-remove lid vid)
      ) ;when
    ) ;for
  ) ;with
) ;tm-define

(tm-define (live-states-in-use lid)
  (live-view-clean lid)
  (let* ((t (live-view-table lid))
         (l1 (former lid))
         (l2 (if (live-exists? lid) (map cdr (ahash-table->list t)) (list)))
        ) ;
    (append l1 l2)
  ) ;let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Updating views to match the current live document
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define live-author (new-author))

(define live-updating? #f)

(tm-define (live-view-update lid vid)
  (with-author live-author
    (with-global live-updating?
      #t
      (let* ((old-state (live-view-get-state lid vid))
             (new-state (live-current-state lid))
             (p (live-get-inverse-patch lid old-state))
             (cur (live-current-document lid))
             (vts (id->trees vid))
            ) ;
        (when (!= new-state old-state)
          (for (vt vts)
            (if (and p (patch-applicable? p vt))
              (begin
                (when (debug-get "live")
                  (display* "]] Apply " (patch->scheme p) "\n")
                ) ;when
                (patch-apply! vt p)
              ) ;begin
              (begin
                (when (debug-get "live")
                  (display* "]] Reset view " (tm->stree cur) "\n")
                ) ;when
                (tree-set! vt cur)
              ) ;begin
            ) ;if
          ) ;for
          (live-view-set-state lid vid new-state)
        ) ;when
        (when (null? vts)
          (live-view-remove lid vid)
        ) ;when
      ) ;let*
    ) ;with-global
  ) ;with-author
) ;tm-define

(tm-define (live-update-views lid)
  (with vids
    (map car (ahash-table->list (live-view-table lid)))
    (for (vid vids) (live-view-update lid vid))
  ) ;with
  (live-forget-obsolete lid)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Restoring views to match the current live document in case of panic
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (live-view-restore lid vid)
  ;; (display* "live-view-restore " lid ", " vid "\n")
  (with-author live-author
    (with-global live-updating?
      #t
      (let* ((cur (live-current-document lid)) (vts (id->trees vid)))
        (for (vt vts)
          (when (debug-get "live")
            (display* "]] Restore view " (tm->stree cur) "\n")
          ) ;when
          (tree-set! vt cur)
        ) ;for
        (live-view-set-state lid vid (live-current-state lid))
        (when (null? vts)
          (live-view-remove lid vid)
        ) ;when
      ) ;let*
    ) ;with-global
  ) ;with-author
) ;tm-define

(tm-define (live-restore-views lid)
  (with vids
    (map car (ahash-table->list (live-view-table lid)))
    (for (vid vids) (live-view-restore lid vid))
  ) ;with
  (live-forget-obsolete lid)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Retracting recent changes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (live-view-retract lid vid new-state p t)
  (with-author live-author
    (with-global live-updating?
      #t
      (let* ((old-state (live-view-get-state lid vid)) (vts (id->trees vid)))
        (when (!= new-state old-state)
          (for (vt vts)
            (if (and p (patch-applicable? p vt))
              (begin
                (when (debug-get "live")
                  (display* "]] Apply " (patch->scheme p) "\n")
                ) ;when
                (patch-apply! vt p)
              ) ;begin
              (begin
                (when (debug-get "live")
                  (display* "]] Reset view " (tm->stree t) "\n")
                ) ;when
                (tree-set! vt t)
              ) ;begin
            ) ;if
          ) ;for
          (live-view-set-state lid vid new-state)
        ) ;when
        (when (null? vts)
          (live-view-remove lid vid)
        ) ;when
      ) ;let*
    ) ;with-global
  ) ;with-author
) ;define

(tm-define (live-retract lid state)
  (let* ((vids (map car (ahash-table->list (live-view-table lid))))
         (p (live-get-patch lid state))
         (t (patch-apply (live-current-document lid) p))
        ) ;
    (for (vid vids) (live-view-retract lid vid state p t))
  ) ;let*
  (former lid state)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Initializing views
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (live-retrieve t)
  ;; to be redefined for particular tags
  (let* ((lid (live-id t))
         (vid (live-view-id t))
         (vt (tree-ref t :last))
         (initial-state (live-create lid (tm->stree vt)))
        ) ;
    ;; (display* "live-retrieve " lid ", " vid "\n")
    (live-view-set-state lid vid initial-state)
  ) ;let*
) ;tm-define

(tm-define (live-view-separate lid vid)
  (with-author live-author
    (with-global live-updating?
      #t
      (with vts
        (id->trees vid)
        (when (>= (length vts) 2)
          (for (vt (cdr vts)) (tree-set (tree-ref vt :up 0) (create-unique-id)))
        ) ;when
      ) ;with
    ) ;with-global
  ) ;with-author
) ;tm-define

(tm-define (live-initialize t)
  (:secure #t)
  (when (live-view-context? t)
    (let* ((lid (live-id t)) (vid (live-view-id t)))
      (when (not (live-view-get-state lid vid))
        ;; (display* "live-initialize " lid ", " vid "\n")
        (if (live-exists? lid)
          (delayed (:idle 1) (live-view-restore lid vid))
          (live-retrieve (tree-up t))
        ) ;if
      ) ;when
      (when (>= (length (id->trees vid)) 2)
        (delayed (:idle 1) (live-view-separate lid vid))
      ) ;when
    ) ;let*
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Monitor changes in views and update live document accordingly
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define live-pending (make-ahash-table))

(tm-define (live-treat-pending lid)
  (when (ahash-ref live-pending lid)
    (let* ((pend (ahash-ref live-pending lid))
           (vids (map car (ahash-table->list pend)))
           (vid (car vids))
           (l (ahash-ref pend vid))
           (p (patch-compound (reverse l)))
           (new-state (live-apply-patch lid p))
           (ts (id->trees vid))
          ) ;
      (if (and new-state (== (length ts) 1))
        (live-view-set-state lid vid new-state)
        (live-view-restore lid vid)
      ) ;if
      (for (vid* (cdr vids)) (live-view-restore lid vid*))
    ) ;let*
    (live-update-views lid)
    (ahash-remove! live-pending lid)
  ) ;when
) ;tm-define

(define (live-notify-patch lid vid p)
  (when (not (ahash-ref live-pending lid))
    (delayed (:idle 1) (live-treat-pending lid))
    (ahash-set! live-pending lid (make-ahash-table))
  ) ;when
  (with t
    (ahash-ref live-pending lid)
    (with old (or (ahash-ref t vid) (list)) (ahash-set! t vid (cons p old)))
  ) ;with
) ;define

(tm-define (live-notify event t mod)
  (:secure #t)
  (when (and (not live-updating?)
          (== event 'announce)
          (!= (modification-type mod) 'set-cursor)
          (live-view-context? t)
        ) ;and
    (let* ((lid (live-id t)) (vid (live-view-id t)))
      (when (live-view-get-state lid vid)
        ;; (display* event ", " (tree->path t) ", "
        ;; (modification->scheme mod) "\n")
        (set! mod (modification-copy mod))
        (with p
          (patch-pair mod (modification-invert mod t))
          (live-notify-patch (live-id t) (live-view-id t) p)
        ) ;with
      ) ;when
    ) ;let*
  ) ;when
) ;tm-define
