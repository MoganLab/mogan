
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : db-users.scm
;; DESCRIPTION : Users and permissions
;; COPYRIGHT   : (C) 2015  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (database db-users) (:use (database db-format)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The current user
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define db-current-user #t)

(tm-define-macro (with-user uid . body)
  `(with-global db-current-user ,uid ,@body)
) ;tm-define-macro

(tm-define (db-reset) (former) (set! db-current-user #t))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The database for user management
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define users-dir "$TEXMACS_HOME_PATH/users")

(define users-master (url->url (string-append users-dir "/users-master.tmdb")))

(tm-define (add-user pseudo name)
  (with-database users-master
    (db-set-entry pseudo `(("type" "user") (,"pseudo" ,pseudo) (,"name" ,name)))
    pseudo
  ) ;with-database
) ;tm-define

;; (tm-define (add-user pseudo name)
;;   (with-database users-master
;;     (db-create-entry `(("type" "user")
;;                        ("pseudo" ,pseudo)
;;                        ("name" ,name)))))

(tm-define (remove-user)
  (let* ((del (get-default-user)) (rem (list-filter (get-users-list) (cut != <> del))))
    (when (nnull? rem)
      (set-default-user (car rem))
      (with-database users-master (db-remove-entry del))
    ) ;when
  ) ;let*
) ;tm-define

(tm-define (pseudo->user pseudo)
  (with-database users-master
    (with ids
      (db-search `(("type" "user") (,"pseudo" ,pseudo)))
      (and (nnull? ids) (car ids))
    ) ;with
  ) ;with-database
) ;tm-define

(tm-define (user->pseudo uid)
  (with-database users-master (db-get-field-first uid "pseudo" #f))
) ;tm-define

(tm-define (set-default-user uid)
  (with-database users-master
    (when (!= db-the-default-user uid)
      (db-set-field "root" "default-user" (list uid))
      (set! db-the-default-user uid)
    ) ;when
  ) ;with-database
) ;tm-define

(define (search-default-user)
  (with-database users-master
    (with l (db-get-field "root" "default-user") (and (pair? l) (car l)))
  ) ;with-database
) ;define

(tm-define (set-user-info attr val)
  (with-database users-master
    (when (!= val (get-user-info attr))
      (db-set-field (get-default-user) attr (list val))
    ) ;when
  ) ;with-database
) ;tm-define

(tm-define (get-user-info attr)
  (with-database users-master (db-get-field-first (get-default-user) attr ""))
) ;tm-define

(tm-define (get-users-list)
  (with-database users-master (db-search '(("type" "user"))))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The default user
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define db-the-default-user #f)

(define (create-default-user)
  (let* ((pseudo (get-user-login)) (name (get-user-name)))
    ;; (display* "pseudo= " pseudo "\n")
    ;; (display* "name= " name "\n")
    (when (== pseudo "")
      (set! pseudo "default")
    ) ;when
    (when (== name "")
      (set! name "Default User")
    ) ;when
    (list pseudo name)
  ) ;let*
) ;define

(tm-define (get-default-user)
  (when (not db-the-default-user)
    (and-with uid (search-default-user) (set! db-the-default-user uid))
  ) ;when
  (when (not db-the-default-user)
    (with info
      (create-default-user)
      (with (pseudo name)
        info
        (with me
          (add-user pseudo name)
          (set-default-user me)
          (set! db-the-default-user me)
        ) ;with
      ) ;with
    ) ;with
  ) ;when
  db-the-default-user
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; User databases
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define dir-created-table (make-ahash-table))

(define (get-user-dir uid)
  (with dir
    (string-append users-dir "/" uid)
    (when (not (ahash-ref dir-created-table dir))
      (system-mkdir dir)
      (ahash-set! dir-created-table dir #t)
    ) ;when
    dir
  ) ;with
) ;define

(define (search-preferred-database-id uid kind)
  (with ids
    (db-search `(("type" "preference")
                 (,"user" ,uid)
                 (,"key" ,(string-append "database-" kind)))
    ) ;db-search
    (and (nnull? ids) (car ids))
  ) ;with
) ;define

(tm-define (get-preferred-database uid kind)
  (with-database users-master
    (let* ((id (search-preferred-database-id uid kind))
           (val (and id (db-get-field-first id "value" #f)))
          ) ;
      (if val
        (system->url val)
        (let* ((dir (get-user-dir uid))
               (pseudo (user->pseudo uid))
               (db* (string-append dir "/" pseudo "-" kind ".tmdb"))
               (db (string->url db*))
              ) ;
          (set-preferred-database uid kind db)
          db
        ) ;let*
      ) ;if
    ) ;let*
  ) ;with-database
) ;tm-define

(tm-define (set-preferred-database uid kind db)
  (with-database users-master
    (with-time-stamp #t
      (with id
        (or (search-preferred-database-id uid kind) (db-create-id))
        (db-set-entry id
          `(("type" "preference")
            (,"user" ,uid)
            (,"key" ,(string-append "database-" kind))
            (,"value" ,(url->system db)))
        ) ;db-set-entry
      ) ;with
    ) ;with-time-stamp
  ) ;with-database
) ;tm-define

(tm-define (recent-preferred-databases uid kind)
  (with-database users-master
    (with id
      (or (search-preferred-database-id uid kind) (db-create-id))
      (with-time :always
        (with vals
          (db-get-field id "value")
          (list-remove-duplicates (map system->url (reverse vals)))
        ) ;with
      ) ;with-time
    ) ;with
  ) ;with-database
) ;tm-define

(tm-define (db-get-kind) "general")

(tm-define (user-database . opt-kind)
  (with kind
    (if (null? opt-kind) (db-get-kind) (car opt-kind))
    (get-preferred-database (get-default-user) kind)
  ) ;with
) ;tm-define

(tm-define (use-database db)
  (set-preferred-database (get-default-user) (db-get-kind) db)
  (revert-buffer-revert)
) ;tm-define

(tm-define (recent-databases)
  (recent-preferred-databases (get-default-user) (db-get-kind))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Important tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(smart-table db-encoding-table
 (("owner" * :pseudos) :users)
 (("readable" * :pseudos) :users)
 (("writable" * :pseudos) :users)
 (("delegate-owner" * :pseudos) :users)
 (("delegate-readable" * :pseudos) :users)
 (("delegate-writable" * :pseudos) :users)
) ;smart-table

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Encoding and decoding of lists of users
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (db-encode-user user)
  (if (== user "all")
    user
    (with l
      (db-search (list (list "type" "user") (list "pseudo" user)))
      (if (pair? l) (car l) "all")
    ) ;with
  ) ;if
) ;define

(define (db-encode-users users)
  ;; (display* "Encode users " users "\n")
  (list-filter (map db-encode-user users) identity)
) ;define

(define (db-decode-user id)
  (if (== id "all") id (db-get-field-first id "pseudo" "nobody"))
) ;define

(define (db-decode-users ids)
  ;; (display* "Decode users " ids "\n")
  (list-filter (map db-decode-user ids) identity)
) ;define

(smart-table db-encoder-table (,:users ,db-encode-users))

(smart-table db-decoder-table (,:users ,db-decode-users))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Expand user list according to group membership
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (db-expand accu todo attr)
  (with-user #t
    (with added
      (make-ahash-table)
      (for (uid (ahash-set->list todo))
        (with q
          (list (list "type" "group") (list (string-append "delegate-" attr) uid))
          (for (x (db-search q))
            (when (not (ahash-ref accu x))
              (ahash-set! accu x #t)
              (ahash-set! added x #t)
            ) ;when
          ) ;for
        ) ;with
      ) ;for
      (if (== (ahash-size added) 0) accu (db-expand accu added attr))
    ) ;with
  ) ;with-user
) ;define

(tm-define (db-expand-user uid attr)
  (cond ((string? uid) (db-expand-user (list uid) attr))
        ((list? uid)
         (let* ((accu (list->ahash-set uid)) (todo accu) (done (db-expand accu todo attr)))
           (rcons (sort (ahash-set->list done) string<=?) "all")
         ) ;let*
        ) ;
        (else (list "all"))
  ) ;cond
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Access rights
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (db-allow? id uid attr)
  ;; (display* "Allow " id ", " uid ", " attr "\n")
  (or (== uid #t)
    (let* ((ids (db-get-field id attr)) (exp (db-expand-user uid attr)))
      ;; (display* "Expanded " uid " -> " exp "\n")
      ;; (display* "Test " ids " -> " (nnull? (list-intersection ids exp)) "\n")
      (or (nnull? (list-intersection ids exp))
        (and (!= attr "owner") (db-allow? id uid "owner"))
      ) ;or
    ) ;let*
  ) ;or
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Wrap basic interface to databases
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (db-get-field id attr)
  (if (or (== db-current-user #t)
        (db-allow? id db-current-user "owner")
        (db-allow? id db-current-user "readable")
      ) ;or
    (with-user #t (former id attr))
    (list)
  ) ;if
) ;tm-define

(tm-define (db-set-field id attr vals)
  (when (or (== db-current-user #t) (db-allow? id db-current-user "owner"))
    (with-user #t (former id attr vals))
  ) ;when
) ;tm-define

(tm-define (db-get-entry id)
  (if (or (== db-current-user #t)
        (db-allow? id db-current-user "owner")
        (db-allow? id db-current-user "readable")
      ) ;or
    (with-user #t (former id))
    (list)
  ) ;if
) ;tm-define

(tm-define (db-set-entry id l)
  (when (or (== db-current-user #t) (db-allow? id db-current-user "owner"))
    (with-user #t (former id l))
  ) ;when
) ;tm-define

(tm-define (db-create-entry l)
  (if (== db-current-user #t)
    (former l)
    (let* ((old-owners (or (assoc-ref l "owner") (list)))
           (new-owners (cond ((string? db-current-user) (list db-current-user))
                             ((list? db-current-user) db-current-user)
                             (else (list))
                       ) ;cond
           ) ;new-owners
           (all-owners (list-union old-owners new-owners))
          ) ;
      (and (nnull? all-owners)
        (with-user #t (set! l (assoc-set! (list-copy l) "owner" all-owners)) (former l))
      ) ;and
    ) ;let*
  ) ;if
) ;tm-define

(tm-define (db-remove-entry id)
  (when (or (== db-current-user #t) (db-allow? id db-current-user "owner"))
    (with-user #t (former id))
  ) ;when
) ;tm-define

(tm-define (db-search l)
  (if (== db-current-user #t)
    (former l)
    (let* ((users (db-expand-user db-current-user "readable"))
           (lo (rcons l (cons "owner" users)))
           (lr (rcons l (cons "readable" users)))
          ) ;
      (with-user #t (list-union (former lo) (former lr)))
    ) ;let*
  ) ;if
) ;tm-define
