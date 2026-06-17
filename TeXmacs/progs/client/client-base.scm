
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : client-base.scm
;; DESCRIPTION : clients of TeXmacs servers
;; COPYRIGHT   : (C) 2007, 2013  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (client client-base) (:use (database db-users)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Declaration of call backs
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define call-back-dispatch-table (make-ahash-table))

(tm-define-macro (tm-call-back proto . body)
  (if (npair? proto)
    '(noop)
    (with (fun . args)
      proto
      `(begin
         (tm-define (,fun envelope ,@args)
           (catch ,#t
             (lambda ,() ,@body)
             (lambda err
               (display* "Client error: " err "\n")
               (client-error envelope err))))
         (ahash-set! call-back-dispatch-table (quote ,fun) ,fun))
    ) ;with
  ) ;if
) ;tm-define-macro

(tm-define (client-eval envelope cmd)
  (when (debug-get "remote")
    (display* "client-eval " envelope ", " cmd "\n")
  ) ;when
  (cond ((and (pair? cmd) (ahash-ref call-back-dispatch-table (car cmd)))
         (with (name . args)
           cmd
           (with fun
             (ahash-ref call-back-dispatch-table name)
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
        (else (client-error envelope "invalid command"))
  ) ;cond
) ;tm-define

(tm-define (client-return envelope ret-val)
  (with (server msg-id)
    envelope
    (client-send server `(server-remote-result ,msg-id ,ret-val))
  ) ;with
) ;tm-define

(tm-define (client-error envelope error-msg)
  (with (server msg-id)
    envelope
    (client-send server `(server-remote-error ,msg-id ,error-msg))
  ) ;with
) ;tm-define

(tm-call-back (local-eval cmd)
  (when #f
    ;; only set to #t for debugging purposes
    (with ret
      (eval cmd)
      (when (debug-get "remote")
        (display* "local-eval " cmd " -> " ret "\n")
      ) ;when
      (client-return envelope ret)
    ) ;with
  ) ;when
) ;tm-call-back

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Establishing and finishing connections with servers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define client-server-active? (make-ahash-table))

(define client-serial 0)

(tm-define (active-servers) (ahash-set->list client-server-active?))

(tm-define (client-send server cmd)
  (client-write server (object->string* (list client-serial cmd)))
  (set! client-serial (+ client-serial 1))
) ;tm-define

(tm-define (client-add server)
  (ahash-set! client-server-active? server #t)
  (with wait
    1
    (delayed (:while (ahash-ref client-server-active? server))
      (:pause ((lambda () (inexact->exact (round wait)))))
      (:do (set! wait (min (* 1.01 wait) 2500)))
      ;; (display* "client-wait= " wait "\n")
      (with msg
        (client-read server)
        (when (!= msg "")
          (with (msg-id msg-cmd)
            (string->object msg)
            (client-eval (list server msg-id) msg-cmd)
            (set! wait 1)
          ) ;with
        ) ;when
      ) ;with
    ) ;delayed
  ) ;with
) ;tm-define

(tm-define (client-remove server) (ahash-remove! client-server-active? server))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Sending asynchroneous commands to servers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define client-continuations (make-ahash-table))

(define client-error-handlers (make-ahash-table))

(define (std-client-error msg)
  ;; (texmacs-error "client-remote-error" "remote error ~S" msg)
  (display-err* "Remote error: " msg "\n")
) ;define

(tm-define (client-remote-eval server cmd cont . opt-err-handler)
  (when (debug-get "remote")
    (display* "client-remote-eval " (list server client-serial) ", " cmd "\n")
  ) ;when
  (with err-handler
    std-client-error
    (if (nnull? opt-err-handler) (set! err-handler (car opt-err-handler)))
    (ahash-set! client-continuations client-serial (list server cont))
    (ahash-set! client-error-handlers client-serial (list server err-handler))
    (client-send server cmd)
  ) ;with
) ;tm-define

(tm-define (client-remote-eval* server cmd cont)
  (client-remote-eval server cmd cont cont)
) ;tm-define

(tm-call-back (client-remote-result msg-id ret)
  (with server
    (car envelope)
    (and-with val
      (ahash-ref client-continuations msg-id)
      (ahash-remove! client-continuations msg-id)
      (ahash-remove! client-error-handlers msg-id)
      (with (orig-server cont) val (when (== server orig-server) (cont ret)))
    ) ;and-with
  ) ;with
) ;tm-call-back

(tm-call-back (client-remote-error msg-id err-msg)
  (with server
    (car envelope)
    (and-with val
      (ahash-ref client-error-handlers msg-id)
      (ahash-remove! client-continuations msg-id)
      (ahash-remove! client-error-handlers msg-id)
      (with (orig-server err-handler)
        val
        (when (== server orig-server)
          (err-handler err-msg)
        ) ;when
      ) ;with
    ) ;and-with
  ) ;with
) ;tm-call-back

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Logging in
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define client-active-connections (make-ahash-table))

(tm-define (client-find-server server-name)
  (ahash-ref client-active-connections server-name)
) ;tm-define

(tm-define (client-find-server-name server)
  (and-with p (ahash-ref client-active-connections server) (car p))
) ;tm-define

(tm-define (client-find-server-pseudo server)
  (and-with p (ahash-ref client-active-connections server) (cadr p))
) ;tm-define

(tm-define (client-active-servers)
  (list-filter (active-servers) client-find-server-name)
) ;tm-define

(define (notify-account server-name pseudo passwd)
  (with-database (user-database "remote")
    (with l
      `(("type" "account") (,"server" ,server-name) (,"pseudo" ,pseudo))
      (when (null? (db-search l))
        (db-create-entry l)
      ) ;when
    ) ;with
  ) ;with-database
) ;define

(tm-define (client-accounts)
  (with-database (user-database "remote")
    (let* ((ids (db-search '(("type" "account"))))
           (get (lambda (id)
                  (list (db-get-field-first id "server" "") (db-get-field-first id "pseudo" ""))
                ) ;lambda
           ) ;get
          ) ;
      (map get ids)
    ) ;let*
  ) ;with-database
) ;tm-define

(tm-define (client-new-account server-name pseudo name passwd email agreed)
  (:argument server-name "Server")
  (:argument pseudo "User pseudo")
  (:argument name "Full name")
  (:argument passwd "password" "Password")
  (:argument email "Email address")
  (:argument agreed "Licence agreement")
  (with uid
    (pseudo->user pseudo)
    (when (== uid (get-default-user))
      (when (== (get-user-info "name") "")
        (set-user-info "name" name)
      ) ;when
      (when (== (get-user-info "email") "")
        (set-user-info "email" email)
      ) ;when
    ) ;when
  ) ;with
  (with server
    (client-start server-name)
    (when (!= server -1)
      (client-remote-eval* server
        `(new-account ,pseudo ,name ,passwd ,email ,agreed)
        (lambda (msg)
          (set-message msg "creating new account")
          (when (== msg "done")
            (notify-account server-name pseudo passwd)
          ) ;when
          (client-stop server)
        ) ;lambda
      ) ;client-remote-eval*
    ) ;when
  ) ;with
) ;tm-define

(tm-define (client-login-then server-name pseudo passwd cb)
  (with wcb
    (lambda (ret)
      (when (== ret "ready")
        (notify-account server-name pseudo passwd)
      ) ;when
      (cb ret)
    ) ;lambda
    (with server
      (client-start server-name)
      (when (!= server -1)
        (ahash-set! client-active-connections server (list server-name pseudo))
        (ahash-set! client-active-connections server-name server)
        (set! remote-client-list (client-active-servers))
        (client-remote-eval* server `(remote-login ,pseudo ,passwd) wcb)
      ) ;when
    ) ;with
  ) ;with
) ;tm-define

(tm-define (client-login server-name pseudo passwd)
  (:argument server-name "Server")
  (:argument pseudo "User pseudo")
  (:argument passwd "password" "Password")
  (client-login-then server-name
    pseudo
    passwd
    (lambda (ret) (set-message ret "logging in"))
  ) ;client-login-then
) ;tm-define

(tm-define (client-logout server)
  (and-with server-con
    (ahash-ref client-active-connections server)
    (with (server-name server-pseudo)
      server-con
      (client-stop server)
      (ahash-remove! client-active-connections server)
      (ahash-remove! client-active-connections server-name)
      (for (sv (active-servers))
        (and-with sv-conn
          (ahash-ref client-active-connections sv)
          (with (sv-name sv-pseudo)
            sv-conn
            (ahash-set! client-active-connections sv-name sv)
          ) ;with
        ) ;and-with
      ) ;for
      (set! remote-client-list (client-active-servers))
    ) ;with
  ) ;and-with
) ;tm-define
