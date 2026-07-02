
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : server-base.scm
;; DESCRIPTION : TeXmacs servers
;; COPYRIGHT   : (C) 2007, 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (server server-base) (:use (database db-version)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Declaration of services
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (server-database) (global-database))

(tm-define service-dispatch-table (make-ahash-table))

(tm-define-macro (tm-service proto . body)
  (if (npair? proto)
    '(noop)
    (with (fun . args)
      proto
      `(begin
         (tm-define (,(symbol-append 'service- fun) envelope ,@args)
           (with-database (server-database)
             (catch ,#t
               (lambda ,() ,@body)
               (lambda err
                 (display* "Server error: " err "\n")
                 (server-error envelope err)))))
         (ahash-set! service-dispatch-table
           (quote ,fun)
           ,(symbol-append 'service- fun)))
    ) ;with
  ) ;if
) ;tm-define-macro

(tm-define (server-eval envelope cmd)
  (when (debug-get "remote")
    (display* "server-eval " envelope ", " cmd "\n")
  ) ;when
  (cond ((and (pair? cmd) (ahash-ref service-dispatch-table (car cmd)))
         (with (name . args)
           cmd
           (with fun
             (ahash-ref service-dispatch-table name)
             (apply fun (cons envelope args))
           ) ;with
         ) ;with
        ) ;
        ((symbol? (car cmd))
         (with s
           (symbol->string (car cmd))
           (server-error envelope (string-append "invalid command '" s "'"))
         ) ;with
        ) ;
        (else (server-error envelope "invalid command"))
  ) ;cond
) ;tm-define

(tm-define (server-return envelope ret-val)
  (with (client msg-id)
    envelope
    (server-send client `(client-remote-result ,msg-id ,ret-val))
  ) ;with
) ;tm-define

(tm-define (server-error envelope error-msg)
  (with (client msg-id)
    envelope
    (server-send client `(client-remote-error ,msg-id ,error-msg))
  ) ;with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Establishing and finishing connections with clients
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define server-client-active? (make-ahash-table))

(define server-serial 0)

(tm-define (active-client? client) (ahash-ref server-client-active? client))

(tm-define (active-clients) (ahash-set->list server-client-active?))

(tm-define (server-send client cmd)
  (server-write client (object->string* (list server-serial cmd)))
  (set! server-serial (+ server-serial 1))
) ;tm-define

(tm-define (server-add client)
  (ahash-set! server-client-active? client #t)
  (with wait
    1
    (delayed (:while (ahash-ref server-client-active? client))
      (:pause ((lambda () (inexact->exact (round wait)))))
      (:do (set! wait (min (* 1.01 wait) 2500)))
      ;; (display* "server-wait= " wait "\n")
      (with msg
        (server-read client)
        ;; (when (!= msg "")
        ;;  (display* "wait  = " wait "\n")
        ;;  (display* "client= " client "\n")
        ;;  (display* "msg   = " msg "\n"))
        (when (!= msg "")
          (with (msg-id msg-cmd)
            (string->object msg)
            (server-eval (list client msg-id) msg-cmd)
            (set! wait 1)
          ) ;with
        ) ;when
      ) ;with
    ) ;delayed
  ) ;with
) ;tm-define

(tm-define (server-remove client) (ahash-remove! server-client-active? client))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Sending asynchroneous commands to clients
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define server-continuations (make-ahash-table))

(define server-error-handlers (make-ahash-table))

(define (std-server-error msg)
  ;; (texmacs-error "server-remote-error" "remote error ~S" msg)
  (display-err* "Remote error: " msg "\n")
) ;define

(tm-define (server-remote-eval client cmd cont . opt-err-handler)
  (with err-handler
    std-server-error
    (if (nnull? opt-err-handler) (set! err-handler (car opt-err-handler)))
    (ahash-set! server-continuations server-serial (list client cont))
    (ahash-set! server-error-handlers server-serial (list client err-handler))
    (server-send client cmd)
  ) ;with
) ;tm-define

(tm-define (server-remote-eval* client cmd cont)
  (server-remote-eval client cmd cont cont)
) ;tm-define

(tm-service (server-remote-result msg-id ret)
  (with client
    (car envelope)
    (when (debug-get "remote")
      (display* "server-remote-result " (list client msg-id) "\n")
    ) ;when
    (and-with val
      (ahash-ref server-continuations msg-id)
      (ahash-remove! server-continuations msg-id)
      (ahash-remove! server-error-handlers msg-id)
      (with (orig-client cont) val (when (== client orig-client) (cont ret)))
    ) ;and-with
  ) ;with
) ;tm-service

(tm-service (server-remote-error msg-id err-msg)
  (with client
    (car envelope)
    (and-with val
      (ahash-ref server-error-handlers msg-id)
      (ahash-remove! server-continuations msg-id)
      (ahash-remove! server-error-handlers msg-id)
      (with (orig-client err-handler)
        val
        (when (== client orig-client)
          (err-handler err-msg)
        ) ;when
      ) ;with
    ) ;and-with
  ) ;with
) ;tm-service

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Users
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define server-users (make-ahash-table))

(define (server-load-users)
  (when (== (ahash-size server-users) 0)
    (with f
      "$TEXMACS_HOME_PATH/server/users.scm"
      (set! server-users
        (if (url-exists? f) (list->ahash-table (load-object f)) (make-ahash-table))
      ) ;set!
    ) ;with
  ) ;when
) ;define

(define (server-save-users)
  (with f
    "$TEXMACS_HOME_PATH/server/users.scm"
    (save-object f (ahash-table->list server-users))
  ) ;with
) ;define

(define (server-find-user pseudo)
  (server-load-users)
  (with l
    (ahash-table->list server-users)
    (with ok?
      (lambda (x) (== (cadr x) pseudo))
      (and-with i (list-find-index l ok?) (car (list-ref l i)))
    ) ;with
  ) ;with
) ;define

(define (server-lookup-user pseudo)
  (with-database (server-database)
    (with uids
      (db-search (list (list "type" "user") (list "pseudo" pseudo)))
      (and (nnull? uids) (car uids))
    ) ;with
  ) ;with-database
) ;define

(define (server-set-user-info uid pseudo name passwd email admin)
  (with-database (server-database)
    (with-user #t
      (when (not uid)
        (set! uid pseudo)
      ) ;when
      ;; (when (not uid) (set! uid (db-create-entry (list))))
      (db-set-entry uid
        (list (list "type" "user")
          (list "pseudo" pseudo)
          (list "name" name)
          (list "email" email)
          (list "owner" uid)
        ) ;list
      ) ;db-set-entry
      (server-load-users)
      (ahash-set! server-users uid (list pseudo name passwd email admin))
      (server-save-users)
      (let* ((home (string-append "~" pseudo))
             (q (list (list "name" home) (list "type" "dir")))
            ) ;
        (when (null? (db-search q))
          (db-create-entry (rcons q (list "owner" uid)))
        ) ;when
      ) ;let*
    ) ;with-user
  ) ;with-database
) ;define

(tm-define (server-set-user-information pseudo name passwd email admin)
  (:argument pseudo "User pseudo")
  (:argument name "Full name")
  (:argument passwd "password" "Password")
  (:argument email "Email address")
  (:argument admin "Administrive rights?")
  (:proposals admin '("no" "yes"))
  (with uid
    (or (server-find-user pseudo) (pseudo->user pseudo))
    (server-set-user-info uid pseudo name passwd email (== admin "yes"))
  ) ;with
) ;tm-define

(define (server-create-user pseudo name passwd email admin)
  (with uid
    (or (server-find-user pseudo) (pseudo->user pseudo))
    (server-set-user-info uid pseudo name passwd email admin)
  ) ;with
) ;define

(tm-service (new-account pseudo name passwd email agreed)
  (if (server-find-user pseudo)
    (server-error envelope "user already exists")
    (with ret
      (server-create-user pseudo name passwd email #f)
      (server-return envelope "done")
    ) ;with
  ) ;if
) ;tm-service

(tm-service (server-licence)
  (with f
    "$TEXMACS_HOME_PATH/server/licence.tm"
    (with s
      (and (url-exists? f) (string-load f))
      (with doc
        (and s (convert s "texmacs-document" "texmacs-stree"))
        (server-return envelope doc)
      ) ;with
    ) ;with
  ) ;with
) ;tm-service

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Logging in
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define server-logged-table (make-ahash-table))

(tm-define (server-get-user envelope)
  (with client (car envelope) (and client (ahash-ref server-logged-table client)))
) ;tm-define

(tm-define (server-check-admin? envelope)
  (and-with uid
    (server-get-user envelope)
    (with (pseudo name passwd email admin) (ahash-ref server-users uid) admin)
  ) ;and-with
) ;tm-define

(tm-service (remote-login pseudo passwd)
  (with uid
    (server-find-user pseudo)
    (if (not uid)
      (server-error envelope "user not found")
      (with (pseudo2 name2 passwd2 email2 admin2)
        (ahash-ref server-users uid)
        (if (!= passwd2 passwd)
          (with client
            (car envelope)
            (ahash-remove! server-logged-table client)
            (server-error envelope "invalid password")
          ) ;with
          (with client
            (car envelope)
            (ahash-set! server-logged-table client uid)
            (server-return envelope "ready")
          ) ;with
        ) ;if
      ) ;with
    ) ;if
  ) ;with
) ;tm-service

(tm-service (remote-eval cmd)
  (if (server-check-admin? envelope)
    (with ret
      (eval cmd)
      (when (debug-get "remote")
        (display* "server-remote-eval " cmd " -> " ret "\n")
      ) ;when
      (server-return envelope ret)
    ) ;with
    (server-error envelope "execution of commands is not allowed")
  ) ;if
) ;tm-service
