
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; MODULE      : file-menu.scm
;; DESCRIPTION : the file menus
;; COPYRIGHT   : (C) 1999  Joris van der Hoeven
;;
;; This software falls under the GNU general public license version 3 or later.
;; It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
;; in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(texmacs-module (texmacs menus file-menu)
  (:use (utils library cursor)
    (network url)
    (texmacs texmacs tm-server)
    (texmacs texmacs tm-files)
    (texmacs menus print-widgets)
  ) ;:use
) ;texmacs-module

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dynamic menu for existing buffers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (buffer-list-menu l)
  (for (name l)
    (let* ((abbr (buffer-get-title name))
           (abbr* (if (== abbr "") (url->system (url-tail name)) abbr))
           (mod? (buffer-modified? name))
           (short-name `(verbatim ,(string-append abbr* (if mod? " *" ""))))
           (long-name `(verbatim ,(url->system name)))
          ) ;
      ((check (balloon (eval short-name) (eval long-name))
         "v"
         (== (current-buffer) name)
       ) ;check
       (switch-to-buffer* name)
      ) ;
    ) ;let*
  ) ;for
) ;tm-menu

(tm-define (buffer-more-recent? b1 b2)
  (>= (buffer-last-visited b1) (buffer-last-visited b2))
) ;tm-define

(tm-define (buffer-sorted-list)
  (with l
    (list-filter (buffer-list) buffer-in-menu?)
    (list-sort l buffer-more-recent?)
  ) ;with
) ;tm-define

(tm-define (buffer-menu-list nr)
  (let* ((l1 (list-filter (buffer-list) buffer-in-menu?))
         (l2 (list-sort l1 buffer-more-recent?))
        ) ;
    (sublist l2 0 (min (length l2) nr))
  ) ;let*
) ;tm-define

(tm-define (buffer-menu-unsorted-list nr)
  (let* ((l1 (list-filter (buffer-list) buffer-in-menu?)))
    (sublist l1 0 (min (length l1) nr))
  ) ;let*
) ;tm-define

(tm-define (buffer-go-menu)
  (let* ((l1 (list-difference (buffer-menu-list 15) (linked-file-list))))
    (buffer-list-menu l1)
  ) ;let*
) ;tm-define

(tm-define (buffer-windows-menu)
  (let* ((l1 (map window->buffer (window-list))))
    (buffer-list-menu l1)
  ) ;let*
) ;tm-define

(tm-define (buffer-invisible-list n)
  (let* ((l1 (list-difference (buffer-menu-list n) (linked-file-list)))
         (l2 (map window->buffer (window-list)))
        ) ;
    (list-difference l1 l2)
  ) ;let*
) ;tm-define

(tm-define (buffer-invisible-menu)
  (buffer-list-menu (buffer-invisible-list 25))
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dynamic menu for recent files
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (short-menu-name u)
  (cond ((url-rooted-tmfs? u) (tmfs-title u '(document "")))
        ((url-rooted-web? u)
         (string-append (url->system (url-tail u)) " @ " (url-host u))
        ) ;
        (else (url->system (url-tail u)))
  ) ;cond
) ;define

(define (long-menu-name u)
  (url->system u)
) ;define

(tm-menu (file-list-menu l win?)
  (for (name l)
    (let* ((short-name `(verbatim ,(short-menu-name name)))
           (long-name `(verbatim ,(long-menu-name name)))
          ) ;
      ((balloon (eval short-name) (eval long-name))
       (begin
         (if win? (load-document name) (load-buffer name))
         (when (not (url-exists? (url->system name)))
           (recent-files-remove-by-path (url->system name))
         ) ;when
       ) ;begin
      ) ;
    ) ;let*
  ) ;for
) ;tm-menu

(tm-define (recent-file-list nr)
  (let* ((l1 (map cdar (learned-interactive "recent-buffer")))
         (l2 (map system->url l1))
         (l3 (list-filter l2 buffer-in-recent-menu?))
        ) ;
    (sublist l3 0 (min (length l3) nr))
  ) ;let*
) ;tm-define

(tm-define (recent-unloaded-file-list nr)
  (let* ((l1 (map cdar (learned-interactive "recent-buffer")))
         (l2 (map system->url l1))
         (l3 (list-filter l2 buffer-in-recent-menu?))
         (dl (list-difference l3 (buffer-list)))
        ) ;
    (sublist dl 0 (min (length dl) nr))
  ) ;let*
) ;tm-define

(tm-define (recent-directory-list nr)
  (let* ((l1 (recent-file-list nr))
         (l2 (map url-head l1))
         (l3 (list-remove-duplicates l2))
        ) ;
    (list-filter l3 (cut url-rooted-protocol? <> "default"))
  ) ;let*
) ;tm-define

(tm-define (recent-file-menu) (file-list-menu (recent-file-list 25) #t))

(tm-define (recent-unloaded-file-menu)
  (with l
    (list-difference (recent-unloaded-file-list 15) (linked-file-list))
    (file-list-menu l #f)
  ) ;with
) ;tm-define

(tm-define (linked-file-menu)
  (file-list-menu (list-remove-duplicates (linked-file-list)) #f)
) ;tm-define

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Dynamic menus for formats
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(tm-menu (import-menu flag?)
  (with l
    (filter (lambda (x)
              (and (not (in? x (image-formats)))
                (or (with-developer-tool?)
                  (and (not (string=? x "mgs")) (not (string=? x "stm")))
                ) ;or
              ) ;and
            ) ;lambda
      (converters-to-special "texmacs-file" "-file" #f)
    ) ;filter
    (for (fm l)
      (let* ((name (format-get-name fm))
             (load-text (string-append "Load " (string-downcase name) " file"))
             (import-text `(concat ,"Import " ,name))
             (text (if flag? import-text name))
             (format (if (== fm "verbatim") "text" fm))
            ) ;
        ((eval text) (choose-file (buffer-importer fm) load-text format))
      ) ;let*
    ) ;for
  ) ;with
) ;tm-menu

(tm-define (import-top-menu) (import-menu #t))
(tm-define (import-import-menu) (import-menu #f))

(define (export-latex-file dest)
  (with opts
    '(("texmacs->latex:progress" . "on"))
    (with s
      (texmacs->latex-document (buffer-get (current-buffer)) opts)
      (string-save s dest)
      (save-buffer-save (current-buffer) (list) "latex_export")
      (set-message `(concat ,"Exported " ,(url->system dest)) "Export LaTeX")
    ) ;with
  ) ;with
) ;define

(tm-menu (export-menu flag?)
  (with l
    (converters-from-special "texmacs-file" "-file" #f)
    (with l2
      (filter (lambda (x)
                (and (not (string=? x "tmu"))
                  (not (string=? x "latex"))
                  (not (string=? x "latex-class"))
                  (or (with-developer-tool?)
                    (and (not (string=? x "mgs")) (not (string=? x "stm")))
                  ) ;or
                ) ;and
              ) ;lambda
        l
      ) ;filter
      (for (fm l2)
        (let* ((name (format-get-name fm))
               (save-text (string-append "Save " (string-downcase name) " file"))
               (export-text `(concat ,"Export as " ,name))
               (text (if flag? export-text name))
               (format (if (== fm "verbatim") "text" fm))
              ) ;
          ((eval text) (choose-file (buffer-exporter fm) save-text format))
        ) ;let*
      ) ;for
    ) ;with
  ) ;with
) ;tm-menu

(tm-define (export-top-menu) (export-menu #t))
(tm-define (export-export-menu) (export-menu #f))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Submenus of the File menu and for the icon bar
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind new-file-menu
  (if (window-per-buffer?) ("New window" (new-document)))
  (if (not (window-per-buffer?))
   ("New document" (new-document))
   ("New window" (new-document*))
  ) ;if
  ;; ("Clone window" (clone-window))
) ;menu-bind

(menu-bind load-menu
 ("Load" (open-document))
 ("Revert" (revert-buffer))
 (if (not (window-per-buffer?)) ("Load in new window" (open-document*)))
 ---
 (link import-top-menu)
 (if (nnull? (recent-file-list 1)) --- (link recent-file-menu))
) ;menu-bind

(menu-bind export-as-image-menu
  (for (fm (filter (lambda (x) (file-converter-exists? "x.pdf" (string-append "y." x)))
             (image-formats)
           ) ;filter
       ) ;fm
   ((eval (upcase-first fm))
    (choose-file export-selection-as-graphics "Export selection as image" fm)
   ) ;
  ) ;for
) ;menu-bind

(menu-bind save-menu
 ("Save" (save-buffer))
 ("Save as" (choose-file save-buffer-as "Save TeXmacs file" "action_save_as"))
 ---
 (link export-top-menu)
 ---
 ((eval '(concat "Export as " "Pdf"))
  (choose-file wrapped-print-to-file "Save pdf file" "pdf")
 ) ;
 ((eval '(concat "Export as " "PostScript"))
  (choose-file wrapped-print-to-file "Save postscript file" "postscript")
 ) ;
 (when (selection-active-any?)
   (=> "Export selection as image" (link export-as-image-menu))
 ) ;when
) ;menu-bind

(menu-bind print-menu-sub
  (if (has-printing-cmd?)
   ("Print buffer" (print-buffer))
   ("Print page selection" (interactive print-pages))
  ) ;if
  ("Print buffer to file"
    (choose-file print-to-file "Print all to file" "postscript")
  ) ;
  ("Print page selection to file"
    (interactive choose-file-and-print-page-selection)
  ) ;
) ;menu-bind

(menu-bind print-menu
 ("Preview" (preview-buffer))
 (if (use-print-dialog?)
   (if (has-printing-cmd?) ("Print" (print-buffer)))
   ("Print to file" (choose-file print-to-file "Print all to file" "postscript"))
 ) ;if
 (if (not (use-print-dialog?)) (-> "Print" (link print-menu-sub)))
 (if (use-menus?) (-> "Page setup" (link page-setup-menu)))
 (if (use-popups?) ("Page setup" (open-page-setup)))
) ;menu-bind

(menu-bind print-menu-inline
 ("Preview" (preview-buffer))
 (if (use-print-dialog?)
   (if (has-printing-cmd?) ("Print" (print-buffer)))
   ("Print to file" (choose-file print-to-file "Print all to file" "postscript"))
 ) ;if
 (if (not (use-print-dialog?)) --- (link print-menu-sub) ---)
 (if (use-menus?) (-> "Page setup" (link page-setup-menu)))
 (if (use-popups?) ("Page setup" (open-page-setup)))
) ;menu-bind

(menu-bind close-menu
  (if (window-per-buffer?) ("Close window" (close-document)))
  (if (not (window-per-buffer?))
   ("Close document" (close-document))
   ("Close window" (close-document*))
  ) ;if
  ("Close TeXmacs" (safely-quit-TeXmacs))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The File menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(define (wrapped-import-pdf-embeded-with-tm tem-pdf)
  (let* ((tem-dir (url-temp-dir))
         (tem-tm (url-append tem-dir "tem.tm"))
         (tem-tm2 (url-append tem-dir "extracted.tm"))
        ) ;
    (if (extract-attachments tem-pdf)
      (begin
        (string-save (serialize-texmacs (pdf-replace-linked-path (tree-import (url-relative tem-tm (pdf-get-attached-main-tm tem-pdf)) "texmacs")
                                          tem-pdf
                                        ) ;pdf-replace-linked-path
                     ) ;serialize-texmacs
          tem-tm2
        ) ;string-save
        (load-buffer tem-tm2)
      ) ;begin
      (begin
        (notify-now "Can not extract attachments from PDF")
        (texmacs-error "pdf" "Can not extract attachments from PDF")
      ) ;begin
    ) ;if
  ) ;let*
) ;define

(define (wrapped-import-pdf-embeded-with-tmu tem-pdf)
  (let* ((tem-dir (url-temp-dir))
         (tem-tmu (url-append tem-dir "tem.tmu"))
         (tem-tmu2 (url-append tem-dir "extracted.tmu"))
        ) ;
    (if (extract-attachments tem-pdf)
      (begin
        (string-save (serialize-tmu (pdf-replace-linked-path (tree-import (url-relative tem-tmu (pdf-get-attached-main-tm tem-pdf)) "tmu")
                                      tem-pdf
                                    ) ;pdf-replace-linked-path
                     ) ;serialize-tmu
          tem-tmu2
        ) ;string-save
        (load-buffer tem-tmu2)
      ) ;begin
      (begin
        (notify-now "Can not extract attachments from PDF")
        (texmacs-error "pdf" "Can not extract attachments from PDF")
      ) ;begin
    ) ;if
  ) ;let*
) ;define

(menu-bind file-menu
 ("New" (new-document))
 ("Load" (open-document))
 ("Revert" (revert-buffer))
 (-> "Recent"
   (link recent-file-menu)
   (if (nnull? (recent-file-list 1)) ---)
   (when (nnull? (recent-file-list 1))
     ("Clear menu" (forget-interactive "recent-buffer"))
   ) ;when
 ) ;->
 ---
 ("Save" (save-buffer))
 ("Save as" (choose-file save-buffer-as "Save TeXmacs file" "action_save_as"))
 ---
 (link print-menu)
 ---
 (-> "Import"
   (link import-import-menu)
   ---
   ("Pdf with embedded document"
     (choose-file wrapped-import-pdf-embeded-with-tmu "Import pdf file" "tmu.pdf")
   ) ;
 ) ;->
 (-> "Export"
   (link export-export-menu)
   ---
   (when (defined? 'texmacs->latex-document)
     ("LaTeX" (choose-file export-latex-file "Save LaTeX file" "latex"))
   ) ;when
   ("TM document" (choose-file save-buffer-as "Save TeXmacs file" "texmacs"))
   ("Pdf" (choose-file wrapped-print-to-file "Save pdf file" "pdf"))
   ("Pdf with embedded document"
     (choose-file wrapped-print-to-pdf-embeded-with-tmu
       "Save tmu.pdf file"
       "tmu.pdf"
     ) ;choose-file
   ) ;
   ("Postscript"
     (choose-file wrapped-print-to-file "Save postscript file" "postscript")
   ) ;
   (when (selection-active-any?)
     (=> "Export selection as image" (link export-as-image-menu))
   ) ;when
 ) ;->
 ---
 (if (window-per-buffer?) ("Close window" (close-document)))
 (if (not (window-per-buffer?)) ("Close document" (close-document)))
 ("Close TeXmacs" (safely-quit-TeXmacs))
) ;menu-bind

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The Go menu
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

(menu-bind go-menu
  (when (cursor-has-history?)
    ("Back" (cursor-history-backward))
  ) ;when
  (when (cursor-has-future?)
    ("Forward" (cursor-history-forward))
  ) ;when
  ("Save position" (cursor-history-add (cursor-path)))
  ---
  (if (not (window-per-buffer?))
    (link buffer-go-menu)
    (if (nnull? (linked-file-list)) --- (link linked-file-menu))
    (if (nnull? (recent-unloaded-file-list 1)) --- (link recent-unloaded-file-menu))
    (if (nnull? (bookmarks-menu)) --- (link bookmarks-menu))
  ) ;if
  (if (window-per-buffer?)
    (group "Windows")
    (link buffer-windows-menu)
    ---
    (group "Buffer in this window")
    ("New" (new-document*))
    ("Load" (open-document*))
    (if (nnull? (buffer-invisible-list 25))
      (-> "Hidden" --- (link buffer-invisible-menu))
    ) ;if
    (if (nnull? (linked-file-list)) (-> "Linked" --- (link linked-file-menu)))
    (if (nnull? (recent-unloaded-file-list 1))
      (-> "Recent" --- (link recent-unloaded-file-menu))
    ) ;if
    (if (nnull? (bookmarks-menu)) (-> "Bookmarks" --- (link bookmarks-menu)))
    ("Close" (close-document*))
  ) ;if
) ;menu-bind
