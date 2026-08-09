
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : comment-widgets.scm
;; DESCRIPTION : special widgets for editing comments
;; COPYRIGHT   : (C) 2020  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (various comment-widgets)
  (:use (various comment-edit)
        (utils library cursor)
        (generic document-style)
        (generic generic-edit)
        (kernel gui menu-widget)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Editing a simple comment in a separate widget
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define comment-quit-command ignore)
(define comment-window-table (make-ahash-table))
(define comment-text "comment")


;; 设置comment编辑器窗口状态
(tm-define (set-comment-window-state opened?)
  (set-auxiliary-widget-state opened? 'comment-editor))

;; 获取当前窗口的comment编辑器状态
(tm-define (get-comment-window-state)
  (let ((state (get-auxiliary-widget-state)))
    (if state
        (let ((opened? (car state))
              (widget-type (cadr state)))
          (and (== widget-type 'comment-editor) opened?))
        #f)))

(define (comment-editor-done u b)
  (buffer-focus u #t)
  (and-let* ((pc (tree-innermost any-comment-context? #t))
             (mid    (and pc (tm->string (tm-ref pc 1))))
             (body   (and pc (tm-ref pc :last))))
    (with-buffer b
      (let* ((mb (buffer-get-body b))
             (l  (tree-search
                   mb
                   (lambda (t)
                     (and (any-comment-context? t)
                          (== (tm->string (tm-ref t 1)) mid))))))
        (when (nnull? l)
          (tree-set! (car l) :last (tree-copy body))))))
  (when (defined? 'mirror-treat-pending)
    (mirror-treat-pending))
  (when (defined? 'mirror-synchronize)
    (mirror-synchronize))
  (set-comment-window-state #f)
  (show-auxiliary-widget #f)
  (comment-quit-command))

;; 用于auxiliary-widget的comment编辑器widget
(tm-widget ((comment-aux-widget u packs doc b) quit)
  (padded
    (resize "480px" "300px"
      (texmacs-input doc `(style (tuple ,@packs)) u))
    ===
    (hlist
      >>
      (explicit-buttons
        ("Done" (comment-editor-done u b))))))

;; Comment编辑器取消函数
(tm-define ((comment-cancel u) . args)
  (set-comment-window-state #f))

(define (allow-init? init)
  (and (string? (car init))
       (not (string-starts? (car init) "page-"))))

(tm-define (open-comment-editor-aux)
  (and-let* ((c (tm->stree (tree-innermost any-comment-context? #t)))
             (b (current-buffer-url))
             (u (string->url (string-append "tmfs://aux/edit-comment" 
                                            "/" (url->string 
                                                  (url-tail (get-auxiliary-widget-parent-url))))))
             (packs (embedded-style-list))
             (pre (document-get-preamble (buffer-tree)))
             (inits* (map cdr (cdr (tm->stree (get-all-inits)))))
             (inits (list-filter inits* allow-init?))
             (env (apply append inits))
             (com (mirror-comment c 'carbon-comment))
             (doc `(with ,@env (document (hide-preamble ,pre) ,com))))
    (buffer-set-master u b)
    (auxiliary-widget (comment-aux-widget u packs doc b)
                      (comment-cancel b)
                      (translate comment-text) u)
    (buffer-focus u #t)
    (go-end)))

(tm-define (open-comment-editor)
  (:applicable (behind-folded-comment?))
  (:interactive #t)
  (set-comment-window-state #t)
  (open-comment-editor-aux))

(tm-define (kbd-control-return)
  (:require (behind-folded-comment?))
  (let ((state (get-comment-window-state)))
    (if state
        (begin
          (set-comment-window-state #f)
          (show-auxiliary-widget #f))
        (open-comment-editor))))

(tm-define (kbd-control-return)
  (:require (inside? 'carbon-comment))
  (comment-editor-done))

(tm-define (make-folded-comment type)
  (:applicable (not (inside-comment?)))
  (make-comment 'folded-comment type (list 1) open-comment-editor))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Open comments editor
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tmfs-permission-handler (comments name type)
  (in? type (list "read")))

(tmfs-title-handler (comments name doc)
  (with u (tmfs-string->url name)
    (string-append (url->system (url-tail u)) " - Comments")))

(define (mirror-comment t . opt-lab)
  (let* ((uid' (if (tm-atomic? (tm-ref t 0))
                   (string-append (tm->string (tm-ref t 0)) "-edit")
                   (create-unique-id)))
         (mid  (tm->string (tm-ref t 1)))
         (typ  (tm->string (tm-ref t 2)))
         (by   (tm->string (tm-ref t 3)))
         (tim  (tm->string (tm-ref t 4)))
         (bod  (tm-ref t :last))
         (lab  (if (null? opt-lab) 'mirror-comment (car opt-lab))))
    `(,lab ,uid' ,mid ,typ ,by ,tim "" ,bod)))

(tmfs-load-handler (comments name)
  (let* ((u (tmfs-string->url name))
         (doc (tree->stree (buffer-get u))))
    (tm-replace doc (cut tm-func? <> 'body 1)
                (lambda (t)
                  (let* ((l (tm-search t comment-context?))
                         (r (map mirror-comment l)))
                    (if (null? r)
                        `(body (document ""))
                        `(body (document ,@r))))))))

(tm-define (open-comments-editor)
  (:applicable (comments-in-buffer))
  (let* ((l (comments-in-buffer))
         (u (current-buffer))
         (cu (string-append "tmfs://comments/" (url->tmfs-string u))))
    (when (not (tree-innermost any-comment-context? #t))
      (list-go-to l :next))
    (when (not (tree-innermost any-comment-context? #t))
      (list-go-to l :previous))
    (when (not (tree-innermost any-comment-context? #t))
      (list-go-to l :first))
    (let* ((t (tree-innermost any-comment-context? #t))
           (id (comment-id t)))
      (when t (tree-select t))
      (load-buffer-in-new-window cu)
      (buffer-set-master cu u)
      (delayed
        (:pause 50)
        (and-let* ((b (buffer-get-body cu))
                   (c (search-comment b id)))
          (with-buffer cu
            (tree-go-to c :start)))))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Pairing cursor positions: comments -> commented
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (in-comments-editor?)
  (string-starts? (url->string (current-buffer)) "tmfs://comments/"))

(define ((comment-by-id? id) t)
  (and (any-comment-context? t)
       (== (comment-id t) id)))

(define (search-comment t id)
  (with l (tree-search t (comment-by-id? id))
    (and (nnull? l) (car l))))

(define (sync-master-cursor)
  (and-let* ((c (tree-innermost 'mirror-comment))
             (m (buffer-get-master (current-buffer)))
             (b (buffer-get-body m))
             (i (comment-id c))
             (t (search-comment b i))
             (inv? (not (notified-change? 4))))
    (with-buffer m
      (when inv? (tree-select t))
      (tree-go-to t :end)
      (when (and (not (cursor-accessible?)) (not (in-source?)))
        (cursor-show-hidden)))))

(tm-define (mouse-event key x y mods time data)
  (former key x y mods time data)
  (when (and (in-comments-editor?) (!= key "move"))
    (sync-master-cursor)))

(tm-define (keyboard-press key time)
  (former key time)
  (when (in-comments-editor?)
    (sync-master-cursor)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Pairing cursor positions: commented -> comments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (has-comments-editor?)
  (let* ((u (current-buffer))
         (cu (string-append "tmfs://comments/" (url->tmfs-string u))))
    (and (buffer-exists? cu)
         (in? (string->url cu) (map window->buffer (window-list))))))

(define (sync-comments-cursor)
  (let* ((m (current-buffer))
         (u (string-append "tmfs://comments/" (url->tmfs-string m)))
         (b (buffer-get-body u))
         (t (tree-innermost any-comment-context? #t))
         (inv? (not (notified-change? 4))))
    (with-buffer u
      (if t
          (and-let* ((i (comment-id t))
                     (c (search-comment b i)))
            (when inv? (tree-select (tm-ref c :last)))
            (tree-go-to c :start))
          (selection-cancel)))))

(tm-define (mouse-event key x y mods time data)
  (former key x y mods time data)
  (when (and (has-comments-editor?) (!= key "move"))
    (sync-comments-cursor)))

(tm-define (keyboard-press key time)
  (former key time)
  (when (has-comments-editor?)
    (sync-comments-cursor)))

;; 注册comment编辑器widget类型
(register-auxiliary-widget-type 'comment-editor (list open-comment-editor-aux))
