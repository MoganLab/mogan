;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : chat-render.scm
;; DESCRIPTION : render chat session model to message document
;; COPYRIGHT   : (C) 2026  Mogan Contributors
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (generic chat-render)
  (:use (generic chat-model)))

(define (chat-append-reverse front back)
  (let loop ((rest front) (result back))
    (if (null? rest)
        result
        (loop (cdr rest) (cons (car rest) result)))))

(define (chat-render-actor-label actor)
  (cond ((== actor 'user) "User")
        ((== actor 'agent) "Agent")
        ((== actor 'system) "System")
        ((symbol? actor) (symbol->string actor))
        (else actor)))

(define (chat-render-entry-children entry indent)
  (let* ((label (assoc-ref entry "label"))
         (children (or (assoc-ref entry "children") #()))
         (prefix (make-string indent #\space))
         (line `(concat ,prefix ,(or label ""))))
    (cons line
          (chat-render-entry-list-children children (+ indent 2)))))

(define (chat-render-entry-list-children entries indent)
  (let loop ((i 0) (n (vector-length entries)) (result '()))
    (if (>= i n)
        (reverse result)
        (loop (+ i 1) n
              (chat-append-reverse
               (chat-render-entry-children (vector-ref entries i) indent)
               result)))))

(define (chat-render-tool-call-lines payload)
  (let* ((title (or (assoc-ref payload "title") "Tool call"))
         (entries (or (assoc-ref payload "entries") #())))
    (cons `(concat (with "font-series" "bold" ,title))
          (chat-render-entry-list-children entries 2))))

(define (chat-render-tool-permission-lines payload)
  (let ((question (or (assoc-ref payload "question") "Permission required")))
    (list `(concat (with "font-series" "bold" "Permission"))
          `(concat "Permission request: " ,question))))

(define (chat-render-file-diff-lines payload)
  (let* ((title (or (assoc-ref payload "title") "File diff"))
         (files (or (assoc-ref payload "files") #()))
         (lines (or (assoc-ref payload "lines") #())))
    (append
     (list `(concat (with "font-series" "bold" ,title)))
     (let loop ((i 0) (n (vector-length files)) (result '()))
       (if (>= i n)
           (reverse result)
           (let* ((file (vector-ref files i))
                  (path (or (assoc-ref file "path") ""))
                  (summary (assoc-ref file "summary")))
             (loop (+ i 1) n
                   (cons (if summary
                             `(concat ,path ": " ,summary)
                             `(concat ,path))
                         result)))))
     (let loop ((i 0) (n (vector-length lines)) (result '()))
       (if (>= i n)
           (reverse result)
           (let* ((line (vector-ref lines i))
                  (kind (assoc-ref line "kind"))
                  (text (or (assoc-ref line "text") "")))
             (loop (+ i 1) n
                   (cons
                    (cond ((== kind 'add)
                           `(with "color" "green" (concat "+ " ,text)))
                          ((== kind 'delete)
                           `(with "color" "red" (concat "- " ,text)))
                          (else
                           `(concat "  " ,text)))
                    result))))))))

(define (chat-render-item-lines item)
  (let ((type (chat-item-type item))
        (content (chat-item-content item))
        (payload (chat-item-payload item)))
    (cond ((== type 'thinking)
           (list `(concat (with "font-shape" "italic" "Thinking: ") ,(or content ""))))
          ((== type 'text)
           (list (or content "")))
          ((== type 'tool-call)
           (chat-render-tool-call-lines payload))
          ((== type 'tool-permission)
           (chat-render-tool-permission-lines payload))
          ((== type 'file-diff)
           (chat-render-file-diff-lines payload))
          (else
           (list `(concat (with "font-series" "bold" "Unknown item: ")
                          ,(if (symbol? type) (symbol->string type) type)))))))

(define (chat-render-block-lines block)
  (let loop ((i 0)
             (n (chat-block-item-count block))
             (result (list `(concat (with "font-series" "bold"
                                          ,(chat-render-actor-label
                                            (chat-block-actor block)))))))
    (if (>= i n)
        (reverse result)
        (loop (+ i 1) n
              (chat-append-reverse
               (chat-render-item-lines (chat-block-item-ref block i))
               result)))))

(tm-define (chat-session->message-document session)
  (let loop ((i 0)
             (n (chat-session-block-count session))
             (children '()))
    (if (>= i n)
        (if (null? children)
            '(document "")
            `(document ,@(reverse children)))
        (loop (+ i 1) n
              (chat-append-reverse
               (append (chat-render-block-lines
                        (chat-session-block-ref session i))
                       (if (< (+ i 1) n) (list "") '()))
               children)))))
