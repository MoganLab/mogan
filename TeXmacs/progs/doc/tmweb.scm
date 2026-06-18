
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : tmweb.scm
;; DESCRIPTION : automatic generation of web sites
;; COPYRIGHT   : (C) 2003  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (doc tmweb) (:use (texmacs texmacs tm-files)))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Selecting a video
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (collect-thumbnails t)
  (cond ((tm-atomic? t) (list))
        ((tm-is? t 'youtube-thumbnail-legend) (list t))
        (else (append-map collect-thumbnails (tm-children t)))
  ) ;cond
) ;define

(define (transform-thumbnail t id ref)
  (with lab
    (if (tm-equal? (tm-ref t 0) id) 'shown 'hidden)
    `(,lab
      (youtube-video-legend ,(tm-ref t 0)
        ,(tm-ref t 1)
        ,(tm-ref ref 2)
        ,(tm-ref ref 3)))
  ) ;with
) ;define

(define (select-thumbnail t l id)
  (cond ((tm-atomic? t) (noop))
        ((and (tm-is? t 'tiny-switch)
           (tm-in? (tm-ref t 0) '(shown hidden))
           (tm-is? (tm-ref t 0 0) 'youtube-video-legend)
         ) ;and
         (with r
           (map (cut transform-thumbnail <> id (tm-ref t 0 0)) l)
           (tree-set! t `(tiny-switch ,@r))
         ) ;with
        ) ;
        (else (for-each (cut select-thumbnail <> l id) (tm-children t)))
  ) ;cond
) ;define

(tm-define (youtube-select t)
  (:type (-> void))
  (:synopsis "Select a video")
  (:secure #t)
  (when (tree->path t)
    (select-thumbnail (buffer-tree) (collect-thumbnails (buffer-tree)) t)
  ) ;when
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Building a web site
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (tmweb-make-dir dir html-dir)
  (when (and (!= dir html-dir) (!= dir (system->url ".")))
    (tmweb-make-dir (url-expand (url-append dir (url-parent))) html-dir)
  ) ;when
  (when (not (url-exists? dir))
    (display* "TeXmacs] Creating directory " (url->system dir) "\n")
    (system-mkdir dir)
    (system-1 "chmod a+x" dir)
  ) ;when
) ;define

(define (tmweb-convert-file tm-file html-file)
  (with-aux tm-file
    (if (url? html-file) (set! current-save-target html-file))
    (export-buffer-main (current-buffer) html-file "html" (list :overwrite))
  ) ;with-aux
) ;define

(define (needs-update? src dest update?)
  (or (not update?) (not (url-exists? dest)) (url-newer? src dest))
) ;define

(define (tmweb-convert-file-dir file tm-dir html-dir update?)
  (let* ((m? (== (get-preference "texmacs->html:mathml") "on"))
         (u1 (url-delta (url-append tm-dir "dummy") file))
         (u2 (url-glue (url-unglue u1 2) (if m? "xhtml" "html")))
         (u3 (url-append html-dir u2))
         (dir (url-expand (url-append u3 (url-parent))))
         (dir-name (url->system (url-tail dir)))
        ) ;
    (when (and (!= dir-name "CVS")
            (!= dir-name ".svn")
            (!= dir-name "prop-base")
            (!= dir-name "text-base")
          ) ;and
      (tmweb-make-dir dir (url-expand html-dir))
      (when (needs-update? file u3 update?)
        (system-wait "Converting" (url->system u1))
        (display* "TeXmacs] Converting " (url->system u1) "\n")
        (tmweb-convert-file file u3)
      ) ;when
    ) ;when
  ) ;let*
) ;define

(define (tmweb-copy-file-dir file tm-dir html-dir update?)
  (let* ((u1 (url-delta (url-append tm-dir "dummy") file))
         (u2 (url-append html-dir u1))
         (name (url->system (url-tail u2)))
         (dir (url-expand (url-append u2 (url-parent))))
         (dir-name (url->system (url-tail dir)))
        ) ;
    (when (and (!= dir-name "CVS")
            (!= dir-name "prop-base")
            (!= dir-name "text-base")
            (not (string-occurs? "/." (url->system u2)))
            (not (string-ends? name "~"))
            (not (string-ends? name "#"))
          ) ;and
      (tmweb-make-dir dir (url-expand html-dir))
      (when (needs-update? file u2 update?)
        (system-wait "Copying" (url->system u1))
        (display* "TeXmacs] Copying " (url->system u1) "\n")
        (system-copy file u2)
      ) ;when
    ) ;when
  ) ;let*
) ;define

(define (tmweb-convert-directory tm-dir html-dir update? keep?)
  (let* ((u1 (url-append tm-dir (url-any)))
         (u2 (url-expand (url-complete u1 "dr")))
         (u3 (url-append u2 (url-wildcard "*.tm")))
         (u4 (url-expand (url-complete u3 "fr")))
         (u5 (url-expand (url-complete u1 "fr")))
        ) ;
    (when (!= html-dir tm-dir)
      (for-each (lambda (x) (tmweb-copy-file-dir x tm-dir html-dir update?))
        (if keep? (url->list u5) (list-difference (url->list u5) (url->list u4)))
      ) ;for-each
    ) ;when
    (for-each (lambda (x) (tmweb-convert-file-dir x tm-dir html-dir update?))
      (url->list u4)
    ) ;for-each
  ) ;let*
) ;define

(tm-define (tmweb-convert-dir tm-dir html-dir)
  (tmweb-convert-directory tm-dir html-dir #f #f)
) ;tm-define

(tm-define (tmweb-update-dir tm-dir html-dir)
  (tmweb-convert-directory tm-dir html-dir #t #f)
) ;tm-define

(tm-define (tmweb-convert-dir-keep-texmacs tm-dir html-dir)
  (tmweb-convert-directory tm-dir html-dir #f #t)
) ;tm-define

(tm-define (tmweb-update-dir-keep-texmacs tm-dir html-dir)
  (tmweb-convert-directory tm-dir html-dir #t #t)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Interface
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-define (tmweb-interactive-build)
  (:interactive #t)
  (user-url "Source directory"
    "directory"
    (lambda (src)
      (user-url "Destination directory"
        "directory"
        (lambda (dest) (tmweb-convert-directory src dest #f #f))
      ) ;user-url
    ) ;lambda
  ) ;user-url
) ;tm-define

(tm-define (tmweb-interactive-update)
  (:interactive #t)
  (user-url "Source directory"
    "directory"
    (lambda (src)
      (user-url "Destination directory"
        "directory"
        (lambda (dest) (tmweb-convert-directory src dest #t #f))
      ) ;user-url
    ) ;lambda
  ) ;user-url
) ;tm-define

(tm-widget ((website-widget src-dir dest-dir) cmd)
  (padded ===
    ===
    (refreshable "website-tool-directories"
      (aligned (item (text "Source directory:")
                 (hlist (input (when answer (set! src-dir answer)) "file" (list src-dir) "30em")
                   //
                   //
                   (explicit-buttons ((balloon (icon "tm_find.xpm") "Choose source directory")
                                      (cpp-choose-file (lambda (u)
                                                         (set! src-dir (url->string u))
                                                         (refresh-now "website-tool-directories")
                                                       ) ;lambda
                                        "Choose source dir"
                                        "directory"
                                        ""
                                        (string->url src-dir)
                                      ) ;cpp-choose-file
                                     ) ;
                   ) ;explicit-buttons
                 ) ;hlist
               ) ;item
        (item (text "Destination directory:")
          (hlist (input (when answer (set! dest-dir answer)) "file" (list dest-dir) "30em")
            //
            //
            (explicit-buttons ((balloon (icon "tm_find.xpm") "Choose destination directory")
                               (cpp-choose-file (lambda (u)
                                                  (set! dest-dir (url->string u))
                                                  (refresh-now "website-tool-directories")
                                                ) ;lambda
                                 "Choose dest dir"
                                 "directory"
                                 ""
                                 (string->url dest-dir)
                               ) ;cpp-choose-file
                              ) ;
            ) ;explicit-buttons
          ) ;hlist
        ) ;item
      ) ;aligned
    ) ;refreshable
    ===
    ===
    (refreshable "website-tool-dialog-buttons"
      (bottom-buttons >>>
       ("Cancel" (cmd #f src-dir dest-dir #f))
       //
       //
       ("Create" (cmd #t src-dir dest-dir #f))
       //
       //
       ("Update" (cmd #t src-dir dest-dir #t))
      ) ;bottom-buttons
    ) ;refreshable
  ) ;padded
) ;tm-widget

(tm-define (open-website-builder)
  (:interactive #t)
  (let ((src (get-preference "website:src-dir"))
        (dest (get-preference "website:dest-dir"))
       ) ;
    (dialogue-window (website-widget src dest)
      (lambda (flag? src dest update?)
        (when flag?
          (set-preference "website:src-dir" src)
          (set-preference "website:dest-dir" dest)
          (if update? (tmweb-update-dir src dest) (tmweb-convert-dir src dest))
        ) ;when
      ) ;lambda
      "Create web site"
    ) ;dialogue-window
  ) ;let
) ;tm-define
