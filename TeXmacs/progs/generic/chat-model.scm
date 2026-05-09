;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-model.scm
;; DESCRIPTION : chat session / block / item model
;; COPYRIGHT   : (C) 2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-model))

(import (liii flexvector))

(define chat-model-version 1)

(define (chat-error message . rest)
  (apply error (cons message rest)))

(tm-define (make-chat-session id json-path created-at)
  `(("id" . ,id)
    ("version" . ,chat-model-version)
    ("blocks" . ,(flexvector))
    ("current_block_id" . #f)
    ("json_path" . ,json-path)
    ("dirty?" . #f)
    ("created_at" . ,created-at)
    ("updated_at" . ,created-at)))

(tm-define (make-chat-block id actor started-at)
  `(("id" . ,id)
    ("actor" . ,actor)
    ("items" . ,(flexvector))
    ("sealed?" . #f)
    ("close_reason" . #f)
    ("started_at" . ,started-at)
    ("ended_at" . #f)))

(tm-define (make-chat-item id type content payload timestamp status)
  `(("id" . ,id)
    ("type" . ,type)
    ("status" . ,status)
    ("timestamp" . ,timestamp)
    ("content" . ,content)
    ("payload" . ,payload)))

(define (chat-alist-set! obj key value)
  (let ((cell (assoc key obj)))
    (if cell
        (set-cdr! cell value)
        (chat-error "missing chat model key" key))))

(tm-define (chat-session-id session) (assoc-ref session "id"))
(tm-define (chat-session-json-path session) (assoc-ref session "json_path"))
(tm-define (chat-session-created-at session) (assoc-ref session "created_at"))
(tm-define (chat-session-updated-at session) (assoc-ref session "updated_at"))
(tm-define (chat-session-dirty? session) (assoc-ref session "dirty?"))
(tm-define (chat-session-current-block-id session) (assoc-ref session "current_block_id"))
(define (chat-session-blocks session) (assoc-ref session "blocks"))
(tm-define (chat-session-block-count session)
  (flexvector-length (chat-session-blocks session)))

(tm-define (chat-block-id block) (assoc-ref block "id"))
(tm-define (chat-block-actor block) (assoc-ref block "actor"))
(define (chat-block-items block) (assoc-ref block "items"))
(tm-define (chat-block-sealed? block) (assoc-ref block "sealed?"))
(tm-define (chat-block-close-reason block) (assoc-ref block "close_reason"))
(tm-define (chat-block-started-at block) (assoc-ref block "started_at"))
(tm-define (chat-block-ended-at block) (assoc-ref block "ended_at"))
(tm-define (chat-block-item-count block)
  (flexvector-length (chat-block-items block)))

(tm-define (chat-item-id item) (assoc-ref item "id"))
(tm-define (chat-item-type item) (assoc-ref item "type"))
(tm-define (chat-item-status item) (assoc-ref item "status"))
(tm-define (chat-item-timestamp item) (assoc-ref item "timestamp"))
(tm-define (chat-item-content item) (assoc-ref item "content"))
(tm-define (chat-item-payload item) (assoc-ref item "payload"))

(define (chat-session-set-dirty! session dirty? updated-at)
  (chat-alist-set! session "dirty?" dirty?)
  (chat-alist-set! session "updated_at" updated-at))

(tm-define (chat-session-block-ref session index)
  (flexvector-ref (chat-session-blocks session) index))

(tm-define (chat-session-current-block session)
  (let ((block-id (chat-session-current-block-id session)))
    (and block-id
         (let loop ((i 0) (n (chat-session-block-count session)))
           (and (< i n)
                (let ((block (chat-session-block-ref session i)))
                  (if (== (chat-block-id block) block-id)
                      block
                      (loop (+ i 1) n))))))))

(tm-define (chat-block-item-ref block index)
  (flexvector-ref (chat-block-items block) index))

(tm-define (chat-block-last-item block)
  (and (> (chat-block-item-count block) 0)
       (chat-block-item-ref block (- (chat-block-item-count block) 1))))

(define (chat-mergeable-item-type? type)
  (not (not (memq type '(thinking text file-diff)))))

(define (chat-item-mergeable-with? item type payload)
  (and item
       (chat-mergeable-item-type? type)
       (== (chat-item-type item) type)
       (equal? (chat-item-payload item) payload)))

(define (chat-symbol->json-string sym)
  (if (symbol? sym) (symbol->string sym) sym))

(define (chat-item->json item)
  `(("id" . ,(chat-item-id item))
    ("type" . ,(chat-symbol->json-string (chat-item-type item)))
    ("status" . ,(chat-symbol->json-string (chat-item-status item)))
    ("timestamp" . ,(chat-item-timestamp item))
    ("content" . ,(chat-item-content item))
    ("payload" . ,(chat-item-payload item))))

(define (chat-block-items->json block)
  (list->vector
   (map chat-item->json
        (flexvector->list (chat-block-items block)))))

(define (chat-block->json block)
  `(("id" . ,(chat-block-id block))
    ("actor" . ,(chat-symbol->json-string (chat-block-actor block)))
    ("sealed" . ,(chat-block-sealed? block))
    ("close_reason" . ,(and (chat-block-close-reason block)
                            (chat-symbol->json-string
                             (chat-block-close-reason block))))
    ("started_at" . ,(chat-block-started-at block))
    ("ended_at" . ,(chat-block-ended-at block))
    ("items" . ,(chat-block-items->json block))))

(tm-define (chat-session->json session)
  `(("id" . ,(chat-session-id session))
    ("meta" . (("version" . ,chat-model-version)
               ("current_block_id" . ,(chat-session-current-block-id session))
               ("json_path" . ,(chat-session-json-path session))
               ("dirty" . ,(chat-session-dirty? session))
               ("created_at" . ,(chat-session-created-at session))
               ("updated_at" . ,(chat-session-updated-at session))))
    ("blocks" . ,(list->vector
                  (map chat-block->json
                       (flexvector->list (chat-session-blocks session)))))))

(tm-define (chat-session-open-block! session block-id actor started-at)
  (if (chat-session-current-block-id session)
      (chat-error "chat session already has an open block" block-id)
      (begin
        (flexvector-add-back! (chat-session-blocks session)
                              (make-chat-block block-id actor started-at))
        (chat-alist-set! session "current_block_id" block-id)
        (chat-session-set-dirty! session #t started-at)
        session)))

(tm-define (chat-session-append-item! session item-id type content payload timestamp status)
  (let ((block (chat-session-current-block session)))
    (if (not block)
        (chat-error "chat session has no open block" item-id)
        (if (chat-block-sealed? block)
            (chat-error "cannot append item to sealed block" item-id)
            (let ((item (chat-block-last-item block)))
              (if (chat-item-mergeable-with? item type payload)
                  (begin
                    (chat-alist-set! item "content"
                                     (string-append
                                      (or (chat-item-content item) "")
                                      (or content "")))
                    (chat-alist-set! item "status" status)
                    (chat-alist-set! item "timestamp" timestamp))
                  (flexvector-add-back! (chat-block-items block)
                                        (make-chat-item item-id type content payload
                                                        timestamp status)))
              (chat-session-set-dirty! session #t timestamp)
              session)))))

(tm-define (chat-session-seal-current-block! session close-reason ended-at)
  (let ((block (chat-session-current-block session)))
    (if (not block)
        (chat-error "chat session has no open block to seal")
        (begin
          (chat-alist-set! block "sealed?" #t)
          (chat-alist-set! block "close_reason" close-reason)
          (chat-alist-set! block "ended_at" ended-at)
          (chat-alist-set! session "current_block_id" #f)
          (chat-session-set-dirty! session #t ended-at)
          session))))
