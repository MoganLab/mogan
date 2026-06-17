
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : client-chat.scm
;; DESCRIPTION : Sending messages and chatting, client side
;; COPYRIGHT   : (C) 2020  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (client client-chat) (:use (client client-tmfs)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Chat room urls
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (chat-room-url? u) (string-starts? (url->string u) "tmfs://chat/"))

(tm-define (chat-rooms-url? u)
  (string-starts? (url->string u) "tmfs://chat-rooms/")
) ;tm-define

(tm-define (mail-box-url? u)
  (and (chat-room-url? u) (string-starts? (url->string (url-tail u)) "mail-"))
) ;tm-define

(define (chat-room-name u)
  (cAr (tmfs->list (url->string u)))
) ;define

(define (chat-room-server u)
  (and (chat-room-url? u)
    (and-let* ((name (string-drop (url->string u) 12)) (sname (tmfs-car name)))
      (client-find-server sname)
    ) ;and-let*
  ) ;and
) ;define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Receiving and sending messages
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (message->share u)
  (let* ((name (url->string (url-tail u))) (h `(hlink ,name ,(url->string u))))
    (if (chat-room-url? u)
      `(concat ,"You were invited to join the chat room \x10;" ,h ,"\x11;.")
      `(concat ,"The resource \x10;" ,h ,"\x11; has been shared with you.")
    ) ;if
  ) ;let*
) ;define

(define (message->document msg)
  (with (action pseudo full-name date doc)
    msg
    (let* ((date* (pretty-time (string->number date)))
           (full-name* (utf8->cork full-name))
          ) ;
      (cond ((== action "share")
             (with doc*
               `(document ,(message->share doc))
               `(chat-output ,full-name* ,pseudo ,"" ,date* ,doc*)
             ) ;with
            ) ;
            (else `(chat-output ,full-name* ,pseudo ,"" ,date* ,doc))
      ) ;cond
    ) ;let*
  ) ;with
) ;define

(define (messages->document msgs name)
  `(document (section* "Messages")
     ,@(map message->document msgs)
     ,@(if (string-starts? name "mail-") (list) (list '(chat-input ""))))
) ;define

(define (chat-document doc)
  `(document (TeXmacs ,(texmacs-version))
     (style (tuple "generic" "chat-room"))
     (body ,doc))
) ;define

(define (empty-document)
  (chat-document '(document ""))
) ;define

(define (chat-room-modified fname)
  ;; (display* "Received message in " fname "\n")
  (noop)
) ;define

(define chat-room-writable-table (make-ahash-table))

(define (chat-room-set-writable fname w?)
  (ahash-set! chat-room-writable-table fname w?)
) ;define

(define (chat-room-writable? fname)
  (ahash-ref chat-room-writable-table fname)
) ;define

(define (chat-room-set fname msgs)
  (with doc
    (messages->document msgs (chat-room-name fname))
    (buffer-set fname (chat-document doc))
    (buffer-pretend-saved fname)
    (chat-room-modified fname)
  ) ;with
) ;define

(define (chat-room-insert fname msg)
  (and-let* ((doc (and (buffer-exists? fname) (buffer-get-body fname)))
             (outl (tree-search doc (cut tree-is? <> 'chat-output)))
             (inl (tree-search doc (cut tree-is? <> 'chat-input)))
             (pos (cond ((nnull? inl) (tree-index (car inl)))
                        ((nnull? outl) (+ (tree-index (cAr outl)) 1))
                        (else #f)
                  ) ;cond
             ) ;pos
             (p (if (nnull? inl) (tree-up (car inl)) (tree-up (cAr outl))))
             (ok? (tree-is? p 'document))
            ) ;
    (tree-insert p pos (list (message->document msg)))
  ) ;and-let*
) ;define

(tm-call-back (chat-room-receive name msg)
  (with (server msg-id)
    envelope
    (and-let* ((sname (client-find-server-name server))
               (fname (string-append "tmfs://chat/" sname "/" name))
              ) ;
      (chat-room-insert fname msg)
      (chat-room-modified fname)
      #t
    ) ;and-let*
  ) ;with
) ;tm-call-back

(tm-define (chat-room-send)
  (and-let* ((t (tree-innermost 'chat-input))
             (mt (tm-ref t 0))
             (msg (tm->stree mt))
             (ok? (chat-room-url? (current-buffer)))
             (room (chat-room-name (current-buffer)))
             (server (chat-room-server (current-buffer)))
             (cmd `(remote-send-message ,room ,"send-document" ,msg))
             (sname (client-find-server-name server))
             (fname (string-append "tmfs://chat/" sname "/" room))
            ) ;
    (if (chat-room-writable? fname)
      (begin
        (tree-set! mt '(document ""))
        (client-remote-eval server cmd ignore)
      ) ;begin
      (set-message "this chat room is read only" "send message")
    ) ;if
  ) ;and-let*
) ;tm-define

(tm-define (kbd-control-return)
  (:require (inside? 'chat-input))
  (chat-room-send)
) ;tm-define

(tm-define (button-chat-send t)
  (:secure #t)
  (tree-go-to t :end)
  (when (inside? 'chat-input)
    (chat-room-send)
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Creating and joining chat rooms
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (chat-room-create server name)
  ;; (display* "chat-room-create " server ", " name "\n")
  (let* ((sname (client-find-server-name server))
         (fname (string-append "tmfs://chat/" sname "/" name))
        ) ;
    (client-remote-eval server
      `(remote-chat-room-create ,name)
      (lambda (msg) (load-document fname))
      (lambda (err) (set-message err "create chat room"))
    ) ;client-remote-eval
  ) ;let*
) ;tm-define

(tm-define (chat-room-create-interactive server)
  (:interactive #t)
  (interactive (lambda (name) (chat-room-create server name))
    (list "Name of the chat room" "string" '())
  ) ;interactive
) ;tm-define

(tm-define (chat-room-join server name)
  ;; (display* "chat-room-join " server ", " name "\n")
  (and-with sname
    (client-find-server-name server)
    (load-document (string-append "tmfs://chat/" sname "/" name))
  ) ;and-with
) ;tm-define

(tm-define (chat-room-join-interactive server)
  (:interactive #t)
  (interactive (lambda (name) (chat-room-join server name))
    (list "Join chat room" "string" '())
  ) ;interactive
) ;tm-define

(tm-define (mail-box-open server)
  ;; (display* "remote-mail-box-open " server "\n")
  (and-let* ((pseudo (client-find-server-pseudo server))
             (mbox (string-append "mail-" pseudo))
            ) ;
    (chat-room-join server mbox)
  ) ;and-let*
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; List of chat rooms
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (fix-link sname u)
  (if (and (string? u)
        (or (string-starts? u "tmfs://remote-file/")
          (string-starts? u "tmfs://remote-dir/")
          (string-starts? u "tmfs://chat/")
          (string-starts? u "tmfs://live/")
        ) ;or
      ) ;and
    (let* ((v (tmfs-cdr (tmfs-cdr (tmfs-cdr u))))
           (v* (string-append sname "/" (tmfs-cdr v)))
           (b (string-drop-right u (string-length v)))
          ) ;
      (string-append b v*)
    ) ;let*
    u
  ) ;if
) ;define

(define (fix-links sname doc)
  (tm-replace doc
    (cut tm-func? <> 'hlink 2)
    (lambda (h) `(hlink ,(tm-ref h 0) ,(fix-link sname (tm-ref h 1))))
  ) ;tm-replace
) ;define

(tmfs-permission-handler (chat-rooms name type) (in? type (list "read")))

(tmfs-load-handler (chat-rooms sname)
  (let* ((u (string-append "tmfs://chat-rooms/" sname))
         (base (string-append "tmfs://chat/" sname))
         (server (client-find-server sname))
        ) ;
    (client-remote-eval server
      '(remote-list-chat-rooms)
      (lambda (l)
        (with hyp
          (lambda (c) `(hlink ,c ,(string-append base "/" c)))
          (with doc
            `(document (section* "My chat rooms") ,@(map hyp l))
            (buffer-set-body u (fix-links sname doc))
            (buffer-pretend-saved u)
            (set-message "retrieved contents" "list of chat rooms")
          ) ;with
        ) ;with
      ) ;lambda
      (lambda (err) (set-message err "list of chat rooms"))
    ) ;client-remote-eval
    (set-message "loading..." "list of chat rooms")
    (empty-document)
  ) ;let*
) ;tmfs-load-handler

(tm-define (list-chat-rooms server)
  (and-with sname
    (client-find-server-name server)
    (string-append "tmfs://chat-rooms/" sname)
  ) ;and-with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; List of shared documents
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tmfs-permission-handler (shared sname type) (in? type (list "read")))

(define (list-shared? msg)
  (with (action pseudo full-name date doc) msg (== action "share"))
) ;define

(define (list-shared-name msg)
  (with (action pseudo full-name date doc) msg (utf8->cork full-name))
) ;define

(define (list-shared-links l name)
  (with f
    (list-filter l (lambda (m) (== (list-shared-name m) name)))
    `((subsection* ,name)
      ,@(map (lambda (m)
               (with (action pseudo full-name date u)
                 m
                 `(hlink ,(url->string (url-tail u)) ,(url->string u))))
          f))
  ) ;with
) ;define

(define (list-shared-document l)
  (let* ((f (list-filter l list-shared?))
         (names (list-remove-duplicates (map list-shared-name f)))
        ) ;
    `(document (section* "Shared resources")
       ,@(append-map (cut list-shared-links f <>) names))
  ) ;let*
) ;define

(tmfs-load-handler (shared sname)
  (let* ((u (string-append "tmfs://shared/" sname)) (server (client-find-server sname)))
    (client-remote-eval server
      '(remote-mail-open)
      (lambda (l)
        (with doc
          (list-shared-document l)
          (buffer-set-body u (fix-links sname doc))
          (buffer-pretend-saved u)
          (set-message "retrieved contents" "list of shared resources")
        ) ;with
      ) ;lambda
      (lambda (err) (set-message err "list of chat rooms"))
    ) ;client-remote-eval
    (set-message "loading..." "list of shared resources")
    (empty-document)
  ) ;let*
) ;tmfs-load-handler

(tm-define (list-shared server)
  (and-with sname
    (client-find-server-name server)
    (string-append "tmfs://shared/" sname)
  ) ;and-with
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Chat rooms as files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tmfs-permission-handler (chat name type) (in? type (list "read")))

(tmfs-title-handler (chat name doc)
  (let* ((fname (string-append "tmfs://chat/" name))
         (room (chat-room-name fname))
         (title (string-append "Chat room - " room))
        ) ;
    title
  ) ;let*
) ;tmfs-title-handler

(tmfs-load-handler (chat name)
  (let* ((fname (string-append "tmfs://chat/" name))
         (server (chat-room-server fname))
         (room (chat-room-name fname))
        ) ;
    (cond ((not server)
           ;; FIXME: better error handling
           (texmacs-error "chat" "invalid server")
          ) ;
          ((not (string-starts? room "mail-"))
           (client-remote-eval server
             `(remote-chat-room-open ,room)
             (lambda (ret)
               (chat-room-set fname (cadr ret))
               (chat-room-set-writable fname (car ret))
               (if (car ret)
                 (set-message "retrieved contents" "join chat room")
                 (set-message "joined in read only mode" "join chat room")
               ) ;if
             ) ;lambda
             (lambda (err) (set-message err "join chat room"))
           ) ;client-remote-eval
           (set-message "loading..." "joining chat room")
           (empty-document)
          ) ;
          ((string-starts? room "mail-")
           (client-remote-eval server
             '(remote-mail-open)
             (lambda (msgs)
               (chat-room-set fname msgs)
               (chat-room-set-writable fname #t)
               (set-message "retrieved contents" "open mail box")
             ) ;lambda
             (lambda (err) (set-message err "join chat room"))
           ) ;client-remote-eval
           (set-message "loading..." "opening mail box")
           (empty-document)
          ) ;
    ) ;cond
  ) ;let*
) ;tmfs-load-handler
